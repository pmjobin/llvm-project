//===-- SHRintTest.cpp - Public rounding libcall ABI tests ----------------===//
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include "SHSubtarget.h"
#include "llvm/Analysis/LibcallLoweringInfo.h"
#include "llvm/IR/RuntimeLibcalls.h"
#include "llvm/MC/TargetRegistry.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Target/TargetMachine.h"
#include "gtest/gtest.h"

using namespace llvm;

TEST(SHRint, PublicLibmTypesAndOtherRoundingFamilies) {
  LLVMInitializeSHTargetInfo();
  LLVMInitializeSHTarget();
  LLVMInitializeSHTargetMC();
  for (StringRef TripleName :
       {"shle-unknown-linux-gnu", "sh-unknown-linux-gnu"}) {
    Triple TT(TripleName);
    std::string Error;
    const Target *T = TargetRegistry::lookupTarget(TT, Error);
    ASSERT_NE(nullptr, T) << Error;
    std::unique_ptr<TargetMachine> TM(
        T->createTargetMachine(TT, "sh4", "", TargetOptions(), std::nullopt));
    ASSERT_NE(nullptr, TM);
    RTLIB::RuntimeLibcallsInfo RI(TT);
    // Confirm that the generic table does not already provide these entries.
    LibcallLoweringInfo Generic(RI);
    EXPECT_EQ(RTLIB::Unsupported, Generic.getLibcallImpl(RTLIB::RINT_F32));
    EXPECT_EQ(RTLIB::Unsupported, Generic.getLibcallImpl(RTLIB::RINT_F64));
    for (SH::ABIKind ABI : {SH::ABIKind::GNU, SH::ABIKind::GNUHardFloat,
                            SH::ABIKind::GNUEFloat}) {
      SHSubtarget ST(TT, "sh4", "sh4", "", ABI, false, *TM);
      LibcallLoweringInfo Info(
          RI, [&](LibcallLoweringInfo &I) { ST.initLibcallLoweringInfo(I); });
      EXPECT_EQ(RTLIB::impl_rintf, Info.getLibcallImpl(RTLIB::RINT_F32));
      EXPECT_EQ(ABI == SH::ABIKind::GNUEFloat ? RTLIB::Unsupported
                                              : RTLIB::impl_rint,
                Info.getLibcallImpl(RTLIB::RINT_F64));
      EXPECT_EQ(RTLIB::impl_ceilf, Info.getLibcallImpl(RTLIB::CEIL_F32));
      EXPECT_EQ(RTLIB::impl_floorf, Info.getLibcallImpl(RTLIB::FLOOR_F32));
      EXPECT_EQ(RTLIB::impl_roundf, Info.getLibcallImpl(RTLIB::ROUND_F32));
      EXPECT_EQ(RTLIB::impl_fminf, Info.getLibcallImpl(RTLIB::FMIN_F32));
      EXPECT_EQ(RTLIB::impl_fmaxf, Info.getLibcallImpl(RTLIB::FMAX_F32));
      const bool HasDoubleABI = ABI != SH::ABIKind::GNUEFloat;
      EXPECT_EQ(HasDoubleABI ? RTLIB::impl_round : RTLIB::Unsupported,
                Info.getLibcallImpl(RTLIB::ROUND_F64));
      EXPECT_EQ(HasDoubleABI ? RTLIB::impl_fmin : RTLIB::Unsupported,
                Info.getLibcallImpl(RTLIB::FMIN_F64));
      EXPECT_EQ(HasDoubleABI ? RTLIB::impl_fmax : RTLIB::Unsupported,
                Info.getLibcallImpl(RTLIB::FMAX_F64));
      for (RTLIB::Libcall Call :
           {RTLIB::NEARBYINT_F32, RTLIB::NEARBYINT_F64, RTLIB::LRINT_F32,
            RTLIB::LRINT_F64, RTLIB::LLRINT_F32, RTLIB::LLRINT_F64})
        EXPECT_EQ(RTLIB::Unsupported, Info.getLibcallImpl(Call));
    }
  }
}
