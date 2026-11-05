// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef LLVM_SH4_FREESTANDING_H
#define LLVM_SH4_FREESTANDING_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Application hooks for the optional single-threaded C++ support archive.
// Allocation must return storage aligned to alignment; deallocation must accept
// null. Failure to allocate calls the application's non-returning panic hook.
void *sh4_allocate(size_t bytes, size_t alignment);
void sh4_deallocate(void *pointer);
__attribute__((noreturn)) void sh4_panic(void);

// Startup must call preinit/init arrays before main, and this function after
// main to run registered C++ destructors. No threads, DSO loading or unwinding.
void __cxa_finalize(void *dso);

// Raises SH trap 255, then stops if a board trap handler returns.
__attribute__((noreturn)) void abort(void);

#ifdef __cplusplus
}
#endif

#endif
