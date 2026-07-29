//===-- SHConstantPoolValue.cpp - SH constant-pool values ----------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SHConstantPoolValue.h"
#include "llvm/ADT/FoldingSet.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/IR/Type.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

SHConstantPoolValue::SHConstantPoolValue(Type *Ty, const GlobalValue *GV,
                                         int32_t Addend)
    : MachineConstantPoolValue(Ty), Kind(SymbolKind::GlobalValue),
      TargetModifier(Modifier::None), GV(GV), Addend(Addend) {}

SHConstantPoolValue::SHConstantPoolValue(Type *Ty, StringRef Symbol,
                                         int32_t Addend)
    : MachineConstantPoolValue(Ty), Kind(SymbolKind::ExternalSymbol),
      TargetModifier(Modifier::None), Symbol(Symbol.str()), Addend(Addend) {}

SHConstantPoolValue::SHConstantPoolValue(Type *Ty, unsigned JTI, int32_t Addend)
    : MachineConstantPoolValue(Ty), Kind(SymbolKind::JumpTable),
      TargetModifier(Modifier::None), JTI(JTI), Addend(Addend) {}

SHConstantPoolValue::SHConstantPoolValue(Type *Ty, const BlockAddress *BA,
                                         int32_t Addend)
    : MachineConstantPoolValue(Ty), Kind(SymbolKind::BlockAddress),
      TargetModifier(Modifier::None), BA(BA), Addend(Addend) {}

SHConstantPoolValue *SHConstantPoolValue::create(const GlobalValue *GV,
                                                 int32_t Addend) {
  return new SHConstantPoolValue(Type::getInt32Ty(GV->getContext()), GV,
                                 Addend);
}

SHConstantPoolValue *SHConstantPoolValue::create(LLVMContext &Ctx,
                                                 StringRef Symbol,
                                                 int32_t Addend) {
  return new SHConstantPoolValue(Type::getInt32Ty(Ctx), Symbol, Addend);
}

SHConstantPoolValue *SHConstantPoolValue::create(LLVMContext &Ctx, unsigned JTI,
                                                 int32_t Addend) {
  return new SHConstantPoolValue(Type::getInt32Ty(Ctx), JTI, Addend);
}

SHConstantPoolValue *SHConstantPoolValue::create(const BlockAddress *BA,
                                                 int32_t Addend) {
  return new SHConstantPoolValue(Type::getInt32Ty(BA->getContext()), BA,
                                 Addend);
}

const GlobalValue *SHConstantPoolValue::getGlobalValue() const {
  assert(isGlobalValue() && "SH constant-pool value is not a GlobalValue");
  return GV;
}

StringRef SHConstantPoolValue::getExternalSymbol() const {
  assert(isExternalSymbol() &&
         "SH constant-pool value is not an external symbol");
  return Symbol;
}

unsigned SHConstantPoolValue::getJumpTableIndex() const {
  assert(isJumpTable() && "SH constant-pool value is not a jump table");
  return JTI;
}

const BlockAddress *SHConstantPoolValue::getBlockAddress() const {
  assert(isBlockAddress() && "SH constant-pool value is not a block address");
  return BA;
}

bool SHConstantPoolValue::equals(const SHConstantPoolValue &Other) const {
  if (Kind != Other.Kind || TargetModifier != Other.TargetModifier ||
      Addend != Other.Addend)
    return false;
  if (isGlobalValue())
    return GV == Other.GV;
  if (isExternalSymbol())
    return Symbol == Other.Symbol;
  if (isJumpTable())
    return JTI == Other.JTI;
  return BA == Other.BA;
}

int SHConstantPoolValue::getExistingMachineCPValue(MachineConstantPool *CP,
                                                   Align Alignment) {
  const std::vector<MachineConstantPoolEntry> &Constants = CP->getConstants();
  for (unsigned I = 0, E = Constants.size(); I != E; ++I) {
    const MachineConstantPoolEntry &Entry = Constants[I];
    if (!Entry.isMachineConstantPoolEntry() || Entry.getAlign() != Alignment)
      continue;
    const auto *Other =
        static_cast<const SHConstantPoolValue *>(Entry.Val.MachineCPVal);
    if (equals(*Other))
      return I;
  }
  return -1;
}

void SHConstantPoolValue::addSelectionDAGCSEId(FoldingSetNodeID &ID) {
  ID.AddInteger(static_cast<unsigned>(Kind));
  ID.AddInteger(static_cast<unsigned>(TargetModifier));
  ID.AddInteger(static_cast<uint32_t>(Addend));
  if (isGlobalValue())
    ID.AddPointer(GV);
  else if (isExternalSymbol())
    ID.AddString(Symbol);
  else if (isJumpTable())
    ID.AddInteger(JTI);
  else
    ID.AddPointer(BA);
}

void SHConstantPoolValue::print(raw_ostream &OS) const {
  if (isGlobalValue())
    OS << GV->getName();
  else if (isExternalSymbol())
    OS << Symbol;
  else if (isJumpTable())
    OS << "jump-table." << JTI;
  else
    BA->printAsOperand(OS, false);
  if (Addend > 0)
    OS << '+' << Addend;
  else if (Addend < 0)
    OS << Addend;
}
