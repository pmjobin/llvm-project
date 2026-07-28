//===- SelectionDAGSelectSupportTest.cpp ----------------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "SelectionDAGTestBase.h"

namespace {

class MockTargetLowering final : public TargetLowering {
public:
  using TargetLowering::TargetLowering;

  bool isSelectSupported(SelectSupportKind Kind) const override {
    return Kind != ScalarValSelect;
  }
};

class SelectionDAGSelectSupportTest : public SelectionDAGTestBase {};

TEST_F(SelectionDAGSelectSupportTest, DivRemSelectFoldIsVectorIndependent) {
  const TargetSubtargetInfo *STI = TM->getSubtargetImpl(*F);
  ASSERT_NE(nullptr, STI);
  MockTargetLowering TLI(*TM, *STI);

  EXPECT_FALSE(TLI.isDivRemSelectFoldSupported(MVT::i32));
  EXPECT_TRUE(TLI.isDivRemSelectFoldSupported(MVT::v4i32));
}

} // end anonymous namespace
