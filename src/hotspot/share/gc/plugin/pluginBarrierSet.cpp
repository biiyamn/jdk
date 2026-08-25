/*
 * Copyright (c) 2026, Biiyamn. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 */

#include "precompiled.hpp"
#include "gc/plugin/pluginBarrierSet.hpp"
#include "gc/shared/barrierSetAssembler.hpp"
#ifdef COMPILER1
#include "gc/shared/c1/barrierSetC1.hpp"
#endif
#ifdef COMPILER2
#include "gc/shared/c2/barrierSetC2.hpp"
#endif

PluginBarrierSet::PluginBarrierSet() : BarrierSet(
    make_barrier_set_assembler<BarrierSetAssembler>(),
    make_barrier_set_c1<BarrierSetC1>(),
    make_barrier_set_c2<BarrierSetC2>(),
    nullptr,
    nullptr,
    BarrierSet::FakeRtti(BarrierSet::PluginBarrierSet)) {}
