/*
 * Copyright (c) 2017, 2018, Red Hat, Inc. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 *
 */

#include "precompiled.hpp"
#include "gc/test/testArguments.hpp"
#include "gc/shared/gcArguments.hpp"
#include "gc/shared/tlab_globals.hpp"
#include "logging/log.hpp"
#include "runtime/globals.hpp"
#include "runtime/globals_extension.hpp"
#include "runtime/os.hpp"
#include <dlfcn.h>
#include "runtime/java.hpp"

size_t TestArguments::conservative_max_heap_alignment() {
  return UseLargePages ? os::large_page_size() : os::vm_page_size();
}

void TestArguments::initialize() {
  GCArguments::initialize();

  assert(UseTestGC, "Sanity");

  // Forcefully exit when OOME is detected. Nothing we can do at that point.
  if (FLAG_IS_DEFAULT(ExitOnOutOfMemoryError)) {
    FLAG_SET_DEFAULT(ExitOnOutOfMemoryError, true);
  }

  if (TestMaxTLABSize < MinTLABSize) {
    log_warning(gc)("TestMaxTLABSize < MinTLABSize, adjusting it to " SIZE_FORMAT, MinTLABSize);
    TestMaxTLABSize = MinTLABSize;
  }

  if (!TestElasticTLAB && TestElasticTLABDecay) {
    log_warning(gc)("Disabling TestElasticTLABDecay because TestElasticTLAB is disabled");
    FLAG_SET_DEFAULT(TestElasticTLABDecay, false);
  }

#ifdef COMPILER2
  // Enable loop strip mining: there are still non-GC safepoints, no need to make it worse
  if (FLAG_IS_DEFAULT(UseCountedLoopSafepoints)) {
    FLAG_SET_DEFAULT(UseCountedLoopSafepoints, true);
    if (FLAG_IS_DEFAULT(LoopStripMiningIter)) {
      FLAG_SET_DEFAULT(LoopStripMiningIter, 1000);
    }
  }
#endif
}

void TestArguments::initialize_alignments() {
  size_t page_size = UseLargePages ? os::large_page_size() : os::vm_page_size();
  size_t align = MAX2(os::vm_allocation_granularity(), page_size);
  SpaceAlignment = align;
  HeapAlignment  = align;
}
static void* _test_gc_handle = NULL;
static CollectedHeap* (*_test_gc_factory_func)() = NULL;
CollectedHeap* TestArguments::create_heap() {
  log_info(gc)("create test heap ");
    // Try to load the shared library
    char ebuf[1024];
    const char* error_msg = NULL;
    // Load the test GC library
    _test_gc_handle = os::dll_load("./libgc.so", ebuf, sizeof ebuf);
    if (_test_gc_handle == NULL) {
      vm_exit_during_initialization("Failed to load TestGC library", ebuf);
    }

    // Get the factory function
    _test_gc_factory_func = CAST_TO_FN_PTR(CollectedHeap*(*)(),
                            os::dll_lookup(_test_gc_handle, "create_test_collected_heap"));
    if (_test_gc_factory_func == NULL) {

      vm_exit_during_initialization("Test GC factory function not found", error_msg);
    }

  log_info(gc)("About to call factory function at " PTR_FORMAT, p2i((void*)_test_gc_factory_func));
  CollectedHeap* heap = NULL;

    heap = _test_gc_factory_func();
    if (heap != nullptr) {
      log_info(gc)("Factory function returned heap: " PTR_FORMAT, p2i(heap));
    } else {
      log_error(gc)("Factory function returned null heap pointer");
      vm_exit(1); // or any error handling logic
    }

  return heap;
}
