/*
 * Copyright (c) 2026, Biiyamn. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#ifndef SHARE_GC_PLUGIN_PLUGINBARRIERSET_HPP
#define SHARE_GC_PLUGIN_PLUGINBARRIERSET_HPP

#include "gc/shared/barrierSet.hpp"

// Generic raw-access barrier set for version-1 external collectors.
// External collectors may derive from this class to add thread lifecycle
// hooks, but cannot customize compiler or interpreter barriers in API v1.
class PluginBarrierSet : public BarrierSet {
 public:
  PluginBarrierSet();

  virtual void print_on(outputStream* st) const {}

  template <DecoratorSet decorators, typename BarrierSetT = PluginBarrierSet>
  class AccessBarrier : public BarrierSet::AccessBarrier<decorators, BarrierSetT> {};
};

template<>
struct BarrierSet::GetName<PluginBarrierSet> {
  static const BarrierSet::Name value = BarrierSet::PluginBarrierSet;
};

template<>
struct BarrierSet::GetType<BarrierSet::PluginBarrierSet> {
  typedef ::PluginBarrierSet type;
};

#endif // SHARE_GC_PLUGIN_PLUGINBARRIERSET_HPP
