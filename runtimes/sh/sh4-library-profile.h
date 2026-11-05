/* Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
 * See https://llvm.org/LICENSE.txt for license information.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#ifndef LLVM_SH4_LIBRARY_PROFILE_H
#define LLVM_SH4_LIBRARY_PROFILE_H

#if !defined(__sh__) || !defined(__LITTLE_ENDIAN__) ||                         \
    !defined(__SH4__) || defined(__SH4A__) ||                                  \
    !defined(__SH_FLOAT_ABI_SOFT__) || defined(__SH_FDPIC__)
#error "This library profile requires little-endian SH4, soft float, static ELF"
#endif

#if defined(__PIC__) || defined(__PIE__)
#error "This library profile requires static non-PIC, non-PIE code"
#endif

#if defined(__cplusplus) && (defined(__EXCEPTIONS) || defined(__GXX_RTTI))
#error "This libc++ profile requires -fno-exceptions and -fno-rtti"
#endif

#endif
