//===-- SHIRValidation.h - SH IR invariants -------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHIRVALIDATION_H
#define LLVM_LIB_TARGET_SH_SHIRVALIDATION_H

#include "llvm/IR/CallingConv.h"
#include "llvm/Support/Alignment.h"
#include <cstdint>

namespace llvm {

namespace SH {
enum class ABIKind : unsigned;
}

class CallBase;
class DataLayout;
class Function;
class SHTargetMachine;
class Type;

enum class SHAtomicRuntimeCallKind {
  None,
  Operation,
  CompareExchange,
};

bool validateSHIR(const Function &F, SH::ABIKind ABI);
bool validateSHAtomics(const Function &F, bool AllowFloating);
bool validateSHConfiguration(const Function &F, const SHTargetMachine &TM);
void replaceSHFunctionWithUnreachable(Function &F);
SHAtomicRuntimeCallKind getSHAtomicRuntimeCallKind(const CallBase &Call);
bool isSupportedSHCallingConvention(CallingConv::ID CallConv);
void requireSupportedSHCallingConvention(CallingConv::ID CallConv);
bool isSupportedSHAggregateType(Type *Ty);
bool isSupportedSHValueType(Type *Ty);
bool isSupportedSHVectorType(Type *Ty);
bool isSupportedSHVarArgType(Type *Ty);
uint64_t getFixedSHTypeAllocSize(const DataLayout &DL, Type *Ty);
bool isDirectSHAggregateReturn(const DataLayout &DL, Type *Ty);
void requireSupportedSHByValType(const DataLayout &DL, Type *Ty,
                                 Align Alignment);

} // namespace llvm

#endif
