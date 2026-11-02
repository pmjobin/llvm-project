; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O0 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O2 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O3 -verify-machineinstrs -filetype=obj %s -o /dev/null
; RUN: llvm-as %s -o %t.bc
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=shle-unknown-linux-gnu -mcpu=sh2 -O1 -verify-machineinstrs -filetype=obj %t.bc -o /dev/null
; RUN: llc -mtriple=sh-unknown-linux-gnu -mcpu=sh2 -O1 -verify-machineinstrs -filetype=asm %s -o - | FileCheck %s

; CHECK-LABEL: clang_frame_512:
; The first two comparisons need inverted short branches over a long branch;
; the remaining two comparisons can reach cleanup directly.
; CHECK:      bt{{(/s)?}} [[NEXT1:.LBB[0-9]+_[0-9]+]]
; The delayed form may contain a useful instruction before the long branch.
; CHECK:      bra [[CLEANUP:.LBB[0-9]+_[0-9]+]]
; CHECK-NEXT: nop
; CHECK:      [[NEXT1]]:
; CHECK:      bt [[NEXT2:.LBB[0-9]+_[0-9]+]]
; CHECK-NEXT: bra [[CLEANUP]]
; CHECK-NEXT: nop
; CHECK:      [[NEXT2]]:
; CHECK:      bf{{(/s)?}} [[CLEANUP]]
; CHECK:      bf{{(/s)?}} [[CLEANUP]]

source_filename = "serialized-ir-branch-relaxation.c"
target datalayout = "E-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "sh-unknown-linux-gnu"

%struct.TLSRecord = type <{ i8, ptr, i16 }>
@clang_anchor = external global i32, align 4
@clang_tls_scalar = external thread_local local_unnamed_addr global i32, align 4
@clang_tls_record = external thread_local local_unnamed_addr global %struct.TLSRecord, align 1

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(ptr captures(none)) #0

; Function Attrs: noimplicitfloat
declare i32 @gcc_mix(i32 noundef, i32 noundef, i32 noundef, i32 noundef, i32 noundef) local_unnamed_addr #1

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare nonnull ptr @llvm.threadlocal.address.p0(ptr nonnull) #2

; Function Attrs: mustprogress nofree noimplicitfloat noinline norecurse nosync nounwind willreturn memory(none) uwtable(sync)
define internal fastcc noundef i32 @internal_helper(i32 noundef %value) unnamed_addr #3 {
entry:
  %shl = shl i32 %value, 7
  %shr = lshr i32 %value, 3
  %0 = xor i32 %shr, %shl
  %xor1 = xor i32 %0, 324508639
  ret i32 %xor1
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(ptr captures(none)) #0

; Function Attrs: noimplicitfloat noinline nounwind uwtable(sync)
define i32 @clang_frame_512(i32 noundef %a, i32 noundef %b, i32 noundef %c, i32 noundef %tail) local_unnamed_addr #4 {
entry:
  %frame = alloca [464 x i8], align 1
  %value0 = trunc i32 %a to i8
  %value1 = trunc i32 %b to i16
  %value3 = trunc i32 %tail to i8
  call void @llvm.lifetime.start.p0(ptr nonnull %frame) #6
  %call = tail call i32 @gcc_mix(i32 noundef %a, i32 noundef %b, i32 noundef %c, i32 noundef %c, i32 noundef %tail) #7
  %or.i = tail call noundef i32 @llvm.fshl.i32(i32 %call, i32 %call, i32 5)
  %mul = mul i32 %a, 17
  %add2 = add i32 %mul, %b
  %rem = urem i32 %add2, 400
  store volatile i8 %value0, ptr %frame, align 1, !tbaa !12
  %conv = trunc i16 %value1 to i8
  %arrayidx4 = getelementptr inbounds nuw i8, ptr %frame, i32 232
  store volatile i8 %conv, ptr %arrayidx4, align 1, !tbaa !12
  %arrayidx6 = getelementptr inbounds nuw i8, ptr %frame, i32 463
  store volatile i8 %value3, ptr %arrayidx6, align 1, !tbaa !12
  %xor = xor i32 %tail, %a
  %conv7 = trunc i32 %xor to i8
  %4 = getelementptr inbounds nuw i8, ptr %frame, i32 %rem
  %arrayidx8 = getelementptr inbounds nuw i8, ptr %4, i32 32
  store volatile i8 %conv7, ptr %arrayidx8, align 1, !tbaa !12
  %arrayidx9 = getelementptr inbounds nuw i8, ptr %frame, i32 8
  store i64 4340410370284600380, ptr %arrayidx9, align 1
  %arrayidx10 = getelementptr inbounds nuw i8, ptr %frame, i32 24
  store i64 4340410370284600380, ptr %arrayidx10, align 1
  %5 = load volatile i8, ptr %frame, align 1, !tbaa !12
  %6 = xor i32 %call, -1
  %cmp = icmp ugt i32 %tail, %6
  %conv14 = zext i1 %cmp to i32
  %7 = load volatile i8, ptr %arrayidx6, align 1, !tbaa !12
  %8 = load volatile i8, ptr %arrayidx9, align 1, !tbaa !12
  %cmp21.not = icmp eq i8 %8, 60
  br i1 %cmp21.not, label %lor.lhs.false, label %cleanup

lor.lhs.false:                                    ; preds = %entry
  %arrayidx23 = getelementptr inbounds nuw i8, ptr %frame, i32 15
  %9 = load volatile i8, ptr %arrayidx23, align 1, !tbaa !12
  %cmp25.not = icmp eq i8 %9, 60
  br i1 %cmp25.not, label %lor.lhs.false27, label %cleanup

lor.lhs.false27:                                  ; preds = %lor.lhs.false
  %10 = load volatile i8, ptr %arrayidx10, align 1, !tbaa !12
  %cmp30.not = icmp eq i8 %10, 60
  br i1 %cmp30.not, label %lor.lhs.false32, label %cleanup

lor.lhs.false32:                                  ; preds = %lor.lhs.false27
  %arrayidx33 = getelementptr inbounds nuw i8, ptr %frame, i32 31
  %11 = load volatile i8, ptr %arrayidx33, align 1, !tbaa !12
  %cmp35.not = icmp eq i8 %11, 60
  br i1 %cmp35.not, label %if.end, label %cleanup

if.end:                                           ; preds = %lor.lhs.false32
  %12 = tail call align 1 ptr @llvm.threadlocal.address.p0(ptr align 1 @clang_tls_record)
  %pointer = getelementptr inbounds nuw i8, ptr %12, i32 1
  %13 = load ptr, ptr %pointer, align 1, !tbaa !13
  %cmp37 = icmp ne ptr %13, @clang_anchor
  %14 = load i8, ptr %12, align 1
  %cmp42 = icmp ne i8 %14, 90
  %or.cond = select i1 %cmp37, i1 true, i1 %cmp42
  br i1 %or.cond, label %cleanup, label %lor.lhs.false44

lor.lhs.false44:                                  ; preds = %if.end
  %tail45 = getelementptr inbounds nuw i8, ptr %12, i32 5
  %15 = load i16, ptr %tail45, align 1, !tbaa !18
  %cmp47 = icmp ne i16 %15, 27516
  %16 = tail call align 4 ptr @llvm.threadlocal.address.p0(ptr align 4 @clang_tls_scalar)
  %17 = load i32, ptr %16, align 4
  %cmp50 = icmp ne i32 %17, 655894552
  %or.cond79 = select i1 %cmp47, i1 true, i1 %cmp50
  br i1 %or.cond79, label %cleanup, label %if.end53

if.end53:                                         ; preds = %lor.lhs.false44
  %call56 = tail call fastcc i32 @internal_helper(i32 noundef %tail) #8
  %18 = load volatile i8, ptr %frame, align 1, !tbaa !12
  %conv59 = zext i8 %18 to i32
  %19 = load volatile i8, ptr %arrayidx4, align 1, !tbaa !12
  %conv62 = zext i8 %19 to i32
  %20 = load volatile i8, ptr %arrayidx6, align 1, !tbaa !12
  %conv65 = zext i8 %20 to i32
  %21 = load volatile i8, ptr %arrayidx8, align 1, !tbaa !12
  %conv68 = zext i8 %21 to i32
  %22 = load i32, ptr @clang_anchor, align 4, !tbaa !19
  %23 = xor i32 %or.i, %conv14
  %24 = xor i32 %23, %call56
  %25 = xor i32 %24, %conv59
  %26 = xor i32 %25, %conv62
  %27 = xor i32 %26, %conv65
  %28 = xor i32 %27, %conv68
  %29 = xor i32 %28, %22
  %xor71 = xor i32 %29, %call
  br label %cleanup

cleanup:                                          ; preds = %if.end53, %lor.lhs.false44, %if.end, %lor.lhs.false32, %lor.lhs.false27, %lor.lhs.false, %entry
  %retval.0 = phi i32 [ %xor71, %if.end53 ], [ 57005, %entry ], [ 57005, %lor.lhs.false32 ], [ 57005, %lor.lhs.false27 ], [ 57005, %lor.lhs.false ], [ 48879, %lor.lhs.false44 ], [ 48879, %if.end ]
  call void @llvm.lifetime.end.p0(ptr nonnull %frame) #6
  ret i32 %retval.0
}

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.fshl.i32(i32, i32, i32) #5

attributes #0 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #1 = { noimplicitfloat "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="sh2" "target-features"="+sh2" "use-soft-float"="true" }
attributes #2 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #3 = { mustprogress nofree noimplicitfloat noinline norecurse nosync nounwind willreturn memory(none) uwtable(sync) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="sh2" "target-features"="+sh2" "use-soft-float"="true" }
attributes #4 = { noimplicitfloat noinline nounwind uwtable(sync) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="sh2" "target-features"="+sh2" "use-soft-float"="true" }
attributes #5 = { nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }
attributes #6 = { nounwind }
attributes #7 = { noimplicitfloat nounwind }
attributes #8 = { noimplicitfloat }

!llvm.module.flags = !{!0, !1, !2, !3, !4, !5}
!llvm.ident = !{!6}
!llvm.errno.tbaa = !{!7}

!0 = !{i32 7, !"Dwarf Version", i32 5}
!1 = !{i32 2, !"Debug Info Version", i32 3}
!2 = !{i32 8, !"PIC Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 1}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{i32 7, !"debug-info-assignment-tracking", i1 true}
!6 = !{!"clang version 24.0.0git"}
!7 = !{!8, !9, i64 0}
!8 = !{!"__libc_errno", !9, i64 0}
!9 = !{!"int", !10, i64 0}
!10 = !{!"omnipotent char", !11, i64 0}
!11 = !{!"Simple C/C++ TBAA"}
!12 = !{!10, !10, i64 0}
!13 = !{!14, !15, i64 1}
!14 = !{!"TLSRecord", !10, i64 0, !15, i64 1, !17, i64 5}
!15 = !{!"p1 int", !16, i64 0}
!16 = !{!"any pointer", !10, i64 0}
!17 = !{!"short", !10, i64 0}
!18 = !{!14, !17, i64 5}
!19 = !{!9, !9, i64 0}
