/*
 * Copyright (c) 2026, Biiyamn. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#include "precompiled.hpp"
#include "gc/plugin/gcPlugin.hpp"
#include "gc/plugin/gcPluginLoader.hpp"
#include "logging/logConfiguration.hpp"
#if INCLUDE_G1GC
#include "gc/g1/heapRegion.hpp"
#endif
#include "logging/log.hpp"
#include "runtime/globals_extension.hpp"
#include "runtime/java.hpp"
#include "runtime/os.hpp"
#include "utilities/globalDefinitions.hpp"

void* GCPluginLoader::_library = nullptr;
const GCPluginDescriptor* GCPluginLoader::_descriptor = nullptr;
GCArguments* GCPluginLoader::_arguments = nullptr;

static const JVMGCServices gc_plugin_services = {
  HOTSPOT_GC_PLUGIN_API_VERSION,
  sizeof(JVMGCServices),
  HOTSPOT_GC_PLUGIN_HOTSPOT_ABI,
  gc_plugin_build_flags()
};

GCArguments* GCPluginLoader::load(const char* path, const char* options) {
  assert(_library == nullptr && _descriptor == nullptr && _arguments == nullptr,
         "GC plugin already loaded");

  if (path == nullptr || path[0] == '\0') {
    vm_exit_during_initialization("GC plugin path is empty");
  }

  char library_error[1024];
  _library = os::dll_load(path, library_error, sizeof(library_error));
  if (_library == nullptr) {
    vm_exit_during_initialization("Unable to load GC plugin", library_error);
  }

  // dlopen runs the plugin's static constructors. Any unified-log tag sets
  // registered there missed the earlier command-line logging configuration.
  LogConfiguration::reconfigure_after_dynamic_load();

  void* entry = os::dll_lookup(_library, HOTSPOT_GC_PLUGIN_ENTRY_POINT);
  if (entry == nullptr) {
    vm_exit_during_initialization(
        "GC plugin entry point '" HOTSPOT_GC_PLUGIN_ENTRY_POINT "' was not found", path);
  }

  GCPluginInitFunction initialize = CAST_TO_FN_PTR(GCPluginInitFunction, entry);
  _descriptor = initialize(&gc_plugin_services);
  if (_descriptor == nullptr) {
    vm_exit_during_initialization("GC plugin initialization returned no descriptor", path);
  }

  if (_descriptor->struct_size < sizeof(GCPluginDescriptor)) {
    vm_exit_during_initialization("GC plugin descriptor is too small", path);
  }

  if (_descriptor->api_version != HOTSPOT_GC_PLUGIN_API_VERSION) {
    char version_error[256];
    jio_snprintf(version_error, sizeof(version_error),
                 "Plugin API version %u is not supported; this JVM supports version %u",
                 _descriptor->api_version, HOTSPOT_GC_PLUGIN_API_VERSION);
    vm_exit_during_initialization("Incompatible GC plugin", version_error);
  }

  if (_descriptor->name == nullptr || _descriptor->name[0] == '\0') {
    vm_exit_during_initialization("GC plugin has no name", path);
  }

  if (_descriptor->required_hotspot_abi != gc_plugin_services.hotspot_abi) {
    vm_exit_during_initialization("GC plugin was built for an incompatible HotSpot ABI", path);
  }

  if (_descriptor->required_build_flags != gc_plugin_services.build_flags) {
    vm_exit_during_initialization("GC plugin build configuration does not match this JVM", path);
  }

  uint64_t supported_capabilities =
      GCPluginCapabilityNoBarrier |
      GCPluginCapabilityCardTableBarrier |
      GCPluginCapabilityGenerational;
#if INCLUDE_G1GC
  supported_capabilities |= GCPluginCapabilityG1Barrier;
#endif
  if ((_descriptor->capabilities & ~supported_capabilities) != 0) {
    vm_exit_during_initialization(
        "GC plugin declares unsupported capabilities", path);
  }

  const uint64_t barrier_capabilities =
      _descriptor->capabilities &
      (GCPluginCapabilityNoBarrier |
       GCPluginCapabilityCardTableBarrier |
       GCPluginCapabilityG1Barrier);
  if (barrier_capabilities != GCPluginCapabilityNoBarrier &&
      barrier_capabilities != GCPluginCapabilityCardTableBarrier &&
      barrier_capabilities != GCPluginCapabilityG1Barrier) {
    vm_exit_during_initialization(
        "GC plugin must select exactly one supported barrier kind",
        "supported kinds are no-barrier, HotSpot card-table, and HotSpot G1 barrier");
  }

  // G1 support in shared HotSpot code is historically selected through
  // UseG1GC. Activate that existing integration profile only after the
  // external plugin has passed protocol and capability validation. The
  // plugin still supplies GCArguments and the heap implementation.
#if INCLUDE_G1GC
  if (has_capability(GCPluginCapabilityG1Barrier)) {
    FLAG_SET_ERGO(UseG1GC, true);
  }
#endif

  if (_descriptor->create_arguments == nullptr) {
    vm_exit_during_initialization("GC plugin has no GCArguments factory", path);
  }

  char plugin_error[1024] = { 0 };
  _arguments = _descriptor->create_arguments(
      &gc_plugin_services, options, plugin_error, sizeof(plugin_error));
  if (_arguments == nullptr) {
    vm_exit_during_initialization(
        "GC plugin failed to create GCArguments",
        plugin_error[0] == '\0' ? path : plugin_error);
  }

  log_info(gc)("Loaded external GC plugin '%s'%s%s",
               _descriptor->name,
               _descriptor->vendor == nullptr ? "" : " by ",
               _descriptor->vendor == nullptr ? "" : _descriptor->vendor);
  return _arguments;
}

const char* GCPluginLoader::name() {
  return _descriptor == nullptr ? "external gc" : _descriptor->name;
}

bool GCPluginLoader::has_capability(uint64_t capability) {
  return _descriptor != nullptr && (_descriptor->capabilities & capability) != 0;
}

void GCPluginLoader::initialize_runtime() {
#if INCLUDE_G1GC
  if (has_capability(GCPluginCapabilityG1Barrier)) {
    // The external G1 implementation has its own copy of HeapRegion's static
    // geometry. Existing HotSpot G1 barrier, CDS, C1, C2, and serviceability
    // code use the libjvm copy, so initialize that matching runtime profile
    // after the plugin has computed the heap size and alignments.
    HeapRegion::setup_heap_region_size(MaxHeapSize);
  }
#endif
}
