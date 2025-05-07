//
// Created by jvm on 07/05/25.
//
#include "gcService.hpp"
#include "precompiled.hpp"
#include "gc/shared/tlab_globals.hpp"
#include "logging/log.hpp"
#include "runtime/globals.hpp"
#include "runtime/globals_extension.hpp"
#include "runtime/os.hpp"
#include <dlfcn.h>

#include "runtime/java.hpp"


/*
CollectedHeap* GCService::create_heap() {

}*/
//static CollectedHeap* (*_test_gc_factory_func)() = NULL;
//static void* _test_arguments_handle = NULL;
static void* _test_gc_handle = NULL;
static GCArguments& (*_test_arguments_factory_func)() = NULL;

GCArguments& GCService::create_arguments()
{
    log_info(gc)("create test heap ");
    // Try to load the shared library

    char ebuf[1024];
    _test_gc_handle = os::dll_load("./libgc.so", ebuf, sizeof ebuf);
    if (_test_gc_handle == NULL)
    {
        vm_exit_during_initialization("Failed to load TestGC library", ebuf);
    }
    log_info(gc)("Successfully loaded library");
    // Look up the function
    using FactoryFunc = GCArguments& (*)();
    FactoryFunc create_test_arguments = CAST_TO_FN_PTR(FactoryFunc, os::dll_lookup(_test_gc_handle, "create_test_arguments"));
     log_info(gc)("Successfully found symbol 'create_test_arguments'");





     GCArguments& args = create_test_arguments();
    return args;
}



