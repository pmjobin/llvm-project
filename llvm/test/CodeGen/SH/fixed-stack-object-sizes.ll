; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=prolog-epilog %s -o - | FileCheck %s --check-prefix=FRAME
; RUN: llc -mtriple=shle-unknown-elf -mcpu=sh2 -O0 -verify-machineinstrs -stop-after=prolog-epilog %s -o - | FileCheck %s --check-prefix=FRAME

declare void @llvm.memset.p0.i32(ptr captures(none), i8, i32, i1 immarg)

define i32 @fixed_size_1() {
	%object = alloca [1 x i8], align 1
	call void @llvm.memset.p0.i32(ptr align 1 %object, i8 17, i32 1, i1 false)
	%last = getelementptr inbounds [1 x i8], ptr %object, i32 0, i32 0
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_2() {
	%object = alloca [2 x i8], align 2
	call void @llvm.memset.p0.i32(ptr align 2 %object, i8 18, i32 2, i1 false)
	%last = getelementptr inbounds [2 x i8], ptr %object, i32 0, i32 1
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_3() {
	%object = alloca [3 x i8], align 1
	call void @llvm.memset.p0.i32(ptr align 1 %object, i8 19, i32 3, i1 false)
	%last = getelementptr inbounds [3 x i8], ptr %object, i32 0, i32 2
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_4() {
	%object = alloca [4 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 20, i32 4, i1 false)
	%last = getelementptr inbounds [4 x i8], ptr %object, i32 0, i32 3
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_8() {
	%object = alloca [8 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 21, i32 8, i1 false)
	%last = getelementptr inbounds [8 x i8], ptr %object, i32 0, i32 7
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_16() {
	%object = alloca [16 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 22, i32 16, i1 false)
	%last = getelementptr inbounds [16 x i8], ptr %object, i32 0, i32 15
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_32() {
	%object = alloca [32 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 23, i32 32, i1 false)
	%last = getelementptr inbounds [32 x i8], ptr %object, i32 0, i32 31
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_64() {
	%object = alloca [64 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 24, i32 64, i1 false)
	%last = getelementptr inbounds [64 x i8], ptr %object, i32 0, i32 63
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_256() {
	%object = alloca [256 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 25, i32 256, i1 false)
	%last = getelementptr inbounds [256 x i8], ptr %object, i32 0, i32 255
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_512() {
	%object = alloca [512 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 26, i32 512, i1 false)
	%last = getelementptr inbounds [512 x i8], ptr %object, i32 0, i32 511
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_1024() {
	%object = alloca [1024 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 27, i32 1024, i1 false)
	%last = getelementptr inbounds [1024 x i8], ptr %object, i32 0, i32 1023
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_2048() {
	%object = alloca [2048 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 28, i32 2048, i1 false)
	%last = getelementptr inbounds [2048 x i8], ptr %object, i32 0, i32 2047
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_4096() {
	%object = alloca [4096 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 29, i32 4096, i1 false)
	%last = getelementptr inbounds [4096 x i8], ptr %object, i32 0, i32 4095
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_8192() {
	%object = alloca [8192 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 30, i32 8192, i1 false)
	%last = getelementptr inbounds [8192 x i8], ptr %object, i32 0, i32 8191
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_16384() {
	%object = alloca [16384 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 31, i32 16384, i1 false)
	%last = getelementptr inbounds [16384 x i8], ptr %object, i32 0, i32 16383
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_32768() {
	%object = alloca [32768 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 32, i32 32768, i1 false)
	%last = getelementptr inbounds [32768 x i8], ptr %object, i32 0, i32 32767
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_60000() {
	%object = alloca [60000 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 33, i32 60000, i1 false)
	%last = getelementptr inbounds [60000 x i8], ptr %object, i32 0, i32 59999
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

define i32 @fixed_size_65524() {
	%object = alloca [65524 x i8], align 4
	call void @llvm.memset.p0.i32(ptr align 4 %object, i8 34, i32 65524, i1 false)
	%last = getelementptr inbounds [65524 x i8], ptr %object, i32 0, i32 65523
	%value = load volatile i8, ptr %last, align 1
	%result = zext i8 %value to i32
	ret i32 %result
}

; FRAME-LABEL: name:            fixed_size_1
; FRAME: stackSize:       8
; FRAME-LABEL: name:            fixed_size_2
; FRAME: stackSize:       8
; FRAME-LABEL: name:            fixed_size_3
; FRAME: stackSize:       8
; FRAME-LABEL: name:            fixed_size_4
; FRAME: stackSize:       8
; FRAME-LABEL: name:            fixed_size_8
; FRAME: stackSize:       12
; FRAME-LABEL: name:            fixed_size_16
; FRAME: stackSize:       20
; FRAME-LABEL: name:            fixed_size_32
; FRAME: stackSize:       36
; FRAME-LABEL: name:            fixed_size_64
; FRAME: stackSize:       76
; FRAME-LABEL: name:            fixed_size_256
; FRAME: stackSize:       268
; FRAME-LABEL: name:            fixed_size_512
; FRAME: stackSize:       524
; FRAME-LABEL: name:            fixed_size_1024
; FRAME: stackSize:       1036
; FRAME-LABEL: name:            fixed_size_2048
; FRAME: stackSize:       2060
; FRAME-LABEL: name:            fixed_size_4096
; FRAME: stackSize:       4108
; FRAME-LABEL: name:            fixed_size_8192
; FRAME: stackSize:       8204
; FRAME-LABEL: name:            fixed_size_16384
; FRAME: stackSize:       16396
; FRAME-LABEL: name:            fixed_size_32768
; FRAME: stackSize:       32780
; FRAME-LABEL: name:            fixed_size_60000
; FRAME: stackSize:       60012
; FRAME-LABEL: name:            fixed_size_65524
; FRAME: stackSize:       65536
