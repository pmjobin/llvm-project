//===-- SHTargetStreamer.h - SH target streamer ---------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_SH_MCTARGETDESC_SHTARGETSTREAMER_H
#define LLVM_LIB_TARGET_SH_MCTARGETDESC_SHTARGETSTREAMER_H

#include "llvm/MC/MCStreamer.h"

namespace llvm {

class MCExpr;
class MCSubtargetInfo;
class formatted_raw_ostream;

class SHTargetStreamer : public MCTargetStreamer {
public:
  explicit SHTargetStreamer(MCStreamer &S);
  virtual void emitMachine(const MCSubtargetInfo &STI);
  virtual void emitUnalignedDataValue(const MCExpr *Value, unsigned Size,
                                      SMLoc Loc = SMLoc());
};

class SHTargetAsmStreamer : public SHTargetStreamer {
  formatted_raw_ostream &OS;

public:
  SHTargetAsmStreamer(MCStreamer &S, formatted_raw_ostream &OS);
  const char *getDataDirective(const MCSection *Section,
                               unsigned Size) const override;
  void emitUnalignedDataValue(const MCExpr *Value, unsigned Size,
                              SMLoc Loc = SMLoc()) override;
};

} // namespace llvm

#endif
