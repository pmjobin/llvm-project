//===-- SHConstantPoolValue.h - SH constant-pool values --------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_SHCONSTANTPOOLVALUE_H
#define LLVM_LIB_TARGET_SH_SHCONSTANTPOOLVALUE_H

#include "llvm/ADT/StringRef.h"
#include "llvm/CodeGen/MachineConstantPool.h"
#include <cstdint>
#include <string>

namespace llvm {

class BlockAddress;
class GlobalValue;
class LLVMContext;

class SHConstantPoolValue : public MachineConstantPoolValue {
public:
  enum class SymbolKind {
    GlobalValue,
    ExternalSymbol,
    JumpTable,
    BlockAddress
  };
  enum class Modifier { None };

private:
  SymbolKind Kind;
  Modifier TargetModifier;
  const GlobalValue *GV = nullptr;
  std::string Symbol;
  unsigned JTI = 0;
  const BlockAddress *BA = nullptr;
  int32_t Addend;

  SHConstantPoolValue(Type *Ty, const GlobalValue *GV, int32_t Addend);
  SHConstantPoolValue(Type *Ty, StringRef Symbol, int32_t Addend);
  SHConstantPoolValue(Type *Ty, unsigned JTI, int32_t Addend);
  SHConstantPoolValue(Type *Ty, const BlockAddress *BA, int32_t Addend);

public:
  static SHConstantPoolValue *create(const GlobalValue *GV, int32_t Addend);
  static SHConstantPoolValue *create(LLVMContext &Ctx, StringRef Symbol,
                                     int32_t Addend);
  static SHConstantPoolValue *create(LLVMContext &Ctx, unsigned JTI,
                                     int32_t Addend);
  static SHConstantPoolValue *create(const BlockAddress *BA, int32_t Addend);

  SymbolKind getKind() const { return Kind; }
  Modifier getModifier() const { return TargetModifier; }
  bool isGlobalValue() const { return Kind == SymbolKind::GlobalValue; }
  bool isExternalSymbol() const { return Kind == SymbolKind::ExternalSymbol; }
  bool isJumpTable() const { return Kind == SymbolKind::JumpTable; }
  bool isBlockAddress() const { return Kind == SymbolKind::BlockAddress; }
  const GlobalValue *getGlobalValue() const;
  StringRef getExternalSymbol() const;
  unsigned getJumpTableIndex() const;
  const BlockAddress *getBlockAddress() const;
  int32_t getAddend() const { return Addend; }

  bool equals(const SHConstantPoolValue &Other) const;
  int getExistingMachineCPValue(MachineConstantPool *CP,
                                Align Alignment) override;
  void addSelectionDAGCSEId(FoldingSetNodeID &ID) override;
  void print(raw_ostream &OS) const override;
};

} // namespace llvm

#endif
