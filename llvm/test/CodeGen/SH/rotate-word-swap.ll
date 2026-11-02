; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O1 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefixes=SPEED,MODES
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefixes=SPEED,MODES
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefixes=SPEED,MODES
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh3 -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefixes=SPEED,MODES
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh4 -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefixes=SPEED,MODES
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh4a -O2 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefixes=SPEED,MODES
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=finalize-isel < %s | FileCheck %s --check-prefixes=OLD,MODES

declare i32 @llvm.fshl.i32(i32, i32, i32)
declare i32 @llvm.fshr.i32(i32, i32, i32)

define i32 @left7(i32 %x) {
; SPEED-LABEL: name: left7
; SPEED-DAG: SHLL8
; SPEED-DAG: SWAP_W
; SPEED-DAG: SHLR8
; SPEED: ORrr
; SPEED: ROTR {{%[0-9]+}}, implicit-def dead $tbit
; SPEED-NOT: ROTL
; OLD-LABEL: name: left7
; OLD-COUNT-7: ROTL
	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 7)
	ret i32 %r
}

define i32 @right7(i32 %x) {
; SPEED-LABEL: name: right7
; SPEED-DAG: SHLR8
; SPEED-DAG: SWAP_W
; SPEED-DAG: SHLL8
; SPEED: ORrr
; SPEED: ROTL {{%[0-9]+}}, implicit-def dead $tbit
; OLD-LABEL: name: right7
; OLD-COUNT-7: ROTR
	%r = call i32 @llvm.fshr.i32(i32 %x, i32 %x, i32 7)
	ret i32 %r
}

define i32 @left8(i32 %x) {
; SPEED-LABEL: name: left8
; SPEED-DAG: SHLL8
; SPEED-DAG: SWAP_W
; SPEED-DAG: SHLR8
; SPEED: ORrr
; SPEED-NOT: $tbit
; SPEED: RTS
	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 8)
	ret i32 %r
}

define i32 @left10_disposable(i32 %x) {
; SPEED-LABEL: name: left10_disposable
; SPEED-DAG: SHLL8
; SPEED-DAG: SWAP_W
; SPEED-DAG: SHLR8
; SPEED: ORrr
; SPEED-COUNT-2: ROTL
	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 10)
	ret i32 %r
}

; A preservation copy would make the byte composition tie the old word plan.
define i32 @left10_live(i32 %x) {
; SPEED-LABEL: name: left10_live
; SPEED: SWAP_W
; SPEED-COUNT-6: ROTR
; SPEED-NOT: SHLL8
; SPEED-NOT: SHLR8
; SPEED: ADDrr
	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 10)
	%sum = add i32 %r, %x
	ret i32 %sum
}

define i32 @left39(i32 %x) {
; SPEED-LABEL: name: left39
; SPEED-DAG: SHLL8
; SPEED-DAG: SWAP_W
; SPEED-DAG: SHLR8
; SPEED: ORrr
; SPEED: ROTR
	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 39)
	ret i32 %r
}

define i32 @right_minus8(i32 %x) {
; SPEED-LABEL: name: right_minus8
; SPEED-DAG: SHLL8
; SPEED-DAG: SWAP_W
; SPEED-DAG: SHLR8
; SPEED: ORrr
; SPEED-NOT: $tbit
; SPEED: RTS
	%r = call i32 @llvm.fshr.i32(i32 %x, i32 %x, i32 -8)
	ret i32 %r
}

define i32 @zero_modulo(i32 %x) {
; SPEED-LABEL: name: zero_modulo
; SPEED-NOT: SWAP_W
; SPEED-NOT: ROTL
; SPEED-NOT: ROTR
; SPEED: RTS
	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 -32)
	ret i32 %r
}

define i32 @none(i32 %x) noinline optnone {
; MODES-LABEL: name: none
; MODES-NOT: SWAP_W
; MODES-COUNT-7: ROTL
	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 7)
	ret i32 %r
}

define i32 @size(i32 %x) optsize {
; MODES-LABEL: name: size
; MODES-NOT: SWAP_W
; MODES-COUNT-7: ROTL
	%r = call i32 @llvm.fshl.i32(i32 %x, i32 %x, i32 7)
	ret i32 %r
}

define i32 @minimum(i32 %x) minsize {
; MODES-LABEL: name: minimum
; MODES-NOT: SWAP_W
; MODES-COUNT-7: ROTR
	%r = call i32 @llvm.fshr.i32(i32 %x, i32 %x, i32 7)
	ret i32 %r
}
