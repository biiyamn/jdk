/*
 * Copyright (c) 2026, Biiyamn. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#ifndef SHARE_GC_PLUGIN_GCPLUGIN_HPP
#define SHARE_GC_PLUGIN_GCPLUGIN_HPP

#include <stddef.h>
#include <stdint.h>

class GCArguments;

#define HOTSPOT_GC_PLUGIN_API_VERSION 1u
#define HOTSPOT_GC_PLUGIN_HOTSPOT_ABI 0x4a444b3231474350ULL
#define HOTSPOT_GC_PLUGIN_ENTRY_POINT "JVM_GCPluginInit"

#if defined(_WIN32)
#define HOTSPOT_GC_PLUGIN_EXPORT __declspec(dllexport)
#else
#define HOTSPOT_GC_PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

enum GCPluginCapabilities : uint64_t {
  GCPluginCapabilityNone = 0,
  GCPluginCapabilityNoBarrier = 1u << 0
};

enum GCPluginBuildFlags : uint64_t {
  GCPluginBuildDebug = 1u << 0,
  GCPluginBuildC1 = 1u << 1,
  GCPluginBuildC2 = 1u << 2,
  GCPluginBuildLP64 = 1u << 3
};

static constexpr uint64_t gc_plugin_build_flags() {
  return
#ifdef ASSERT
      GCPluginBuildDebug |
#endif
#ifdef COMPILER1
      GCPluginBuildC1 |
#endif
#ifdef COMPILER2
      GCPluginBuildC2 |
#endif
#ifdef _LP64
      GCPluginBuildLP64 |
#endif
      0;
}

// Services supplied by HotSpot. Version 1 intentionally contains only
// protocol metadata. Runtime wrappers can be appended in later versions.
struct JVMGCServices {
  uint32_t api_version;
  uint32_t struct_size;
  uint64_t hotspot_abi;
  uint64_t build_flags;
};

typedef GCArguments* (*GCPluginCreateArguments)(
    const JVMGCServices* services,
    const char* options,
    char* error_buffer,
    size_t error_buffer_size);

struct GCPluginDescriptor {
  uint32_t api_version;
  uint32_t struct_size;
  const char* name;
  const char* vendor;
  uint64_t capabilities;
  uint64_t required_hotspot_abi;
  uint64_t required_build_flags;
  GCPluginCreateArguments create_arguments;
};

typedef const GCPluginDescriptor* (*GCPluginInitFunction)(
    const JVMGCServices* services);

// A plugin exports this function with C linkage:
// extern "C" HOTSPOT_GC_PLUGIN_EXPORT
// const GCPluginDescriptor* JVM_GCPluginInit(const JVMGCServices* services);

#endif // SHARE_GC_PLUGIN_GCPLUGIN_HPP
