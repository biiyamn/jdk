/*
 * Copyright (c) 2026, Biiyamn. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#ifndef SHARE_GC_PLUGIN_GCPLUGINLOADER_HPP
#define SHARE_GC_PLUGIN_GCPLUGINLOADER_HPP

#include "gc/plugin/gcPlugin.hpp"
#include "memory/allocation.hpp"

class GCArguments;

class GCPluginLoader : public AllStatic {
 private:
  static void* _library;
  static const GCPluginDescriptor* _descriptor;
  static GCArguments* _arguments;

 public:
  static GCArguments* load(const char* path, const char* options);
  static bool is_loaded() { return _descriptor != nullptr; }
  static bool has_capability(uint64_t capability);
  static const char* name();
};

#endif // SHARE_GC_PLUGIN_GCPLUGINLOADER_HPP
