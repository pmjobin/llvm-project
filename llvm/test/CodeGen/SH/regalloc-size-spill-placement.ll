; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O2 < %s | FileCheck %s --check-prefix=SIZE
; RUN: llc -mtriple=shle-unknown-none-elf -mcpu=sh3 -O2 -split-spill-mode=speed < %s | FileCheck %s --check-prefix=SPEED
;
; SIZE-LABEL: qtmd_decompress:
; SIZE:       add     #-120,r15
; SIZE-NEXT:  mov     r5,r0
; SIZE-NEXT:  mov     r4,r11
; SIZE-NEXT:  mov     r15,r1
;
; SPEED-LABEL: qtmd_decompress:
; SPEED:       add     #-120,r15
; SPEED-NEXT:  mov     r5,r0
; SPEED-NEXT:  mov     r4,r11
; SPEED-NEXT:  mov     r15,r1
;
source_filename = "regalloc-size-spill-placement.ll"
target datalayout = "e-m:e-p:32:32-i64:32:32-f64:32:32-a:0:32-n32-S32"
target triple = "shle-unknown-none-elf"

; Function Attrs: noimplicitfloat nounwind optsize
define i32 @qtmd_decompress(ptr nofree noundef %qtm, i64 noundef %out_bytes, ptr %o_ptr, ptr %0, i32 %sub.ptr.rhs.cast, i32 %sub.ptr.sub, ptr %1, ptr %output, ptr %2, i32 %H.2, ptr %syms, ptr %3, ptr %cumfreq, i16 %4, ptr %entries, i32 %5, ptr %6, i16 %L.2, i16 %7, i32 %mul190, i32 %div192, i16 %8, ptr %model7, i16 %shl244, ptr %i_ptr25, ptr %i_ptr.6, ptr %i_ptr.7, ptr %add.ptr279, i16 %9, ptr %model0, ptr %model1, ptr %cond314, ptr %entries335, i32 %smax3113, ptr %10, ptr %.phi.trans.insert3128, ptr %cumfreq378.phi.trans.insert, i32 %sub318, i32 %and319, i32 %add320, i16 %11, i32 %i.3.lcssa, ptr %12, ptr %arrayidx353, ptr %cumfreq367, ptr %cumfreq389, i16 %13, i32 %conv368, i32 %mul369, i16 %14, i32 %conv379, i32 %conv328, ptr %cumfreq327, i16 %15, i1 %cmp401, i16 %conv384, i16 %16, i1 %tobool416.not, ptr %17, i32 %window_posn.1, ptr %arrayidx488) #0 {
entry:
  %o_ptr1 = getelementptr inbounds nuw i8, ptr %qtm, i32 52
  %18 = load ptr, ptr %o_ptr, align 4
  %sub.ptr.rhs.cast2 = ptrtoaddr ptr %0 to i32
  %sub.ptr.sub3 = sub i32 0, %sub.ptr.rhs.cast
  %conv = sext i32 %sub.ptr.sub to i64
  %spec.select2403 = tail call i64 @llvm.smin.i64(i64 %out_bytes, i64 %conv)
  %spec.select = trunc nsw i64 %spec.select2403 to i32
  %19 = load ptr, ptr %1, align 4
  %output4 = getelementptr inbounds nuw i8, ptr %qtm, i32 8
  %20 = load ptr, ptr %output, align 4
  %call = tail call i32 %19(ptr noundef %2, ptr noundef %0, i32 noundef %spec.select) #4
  %i_ptr255 = getelementptr inbounds nuw i8, ptr %qtm, i32 44
  %i_end26 = getelementptr inbounds nuw i8, ptr %qtm, i32 48
  %window29 = getelementptr inbounds nuw i8, ptr %qtm, i32 12
  %21 = load ptr, ptr %window29, align 4
  br label %while.cond122

while.cond122:                                    ; preds = %cleanup.thread, %entry
  %window_posn.16 = phi i32 [ 0, %entry ], [ 1, %cleanup.thread ]
  %i_ptr.5 = phi ptr [ null, %entry ], [ %i_ptr.615, %cleanup.thread ]
  %i_end.5 = phi ptr [ null, %entry ], [ %i_end.6, %cleanup.thread ]
  %H.27 = phi i32 [ 0, %entry ], [ %div371, %cleanup.thread ]
  %L.28 = phi i16 [ 0, %entry ], [ %conv38438, %cleanup.thread ]
  %C.3 = phi i16 [ 0, %entry ], [ %C.4, %cleanup.thread ]
  %bit_buffer.4 = phi i32 [ 0, %entry ], [ %bit_buffer.5, %cleanup.thread ]
  %bits_left.4 = phi i8 [ 0, %entry ], [ %bits_left.5, %cleanup.thread ]
  %syms9 = getelementptr inbounds nuw i8, ptr %qtm, i32 176
  %entries10 = getelementptr inbounds nuw i8, ptr %qtm, i32 172
  %conv127 = and i32 %H.2, 65535
  %22 = load ptr, ptr %syms, align 4
  %cumfreq11 = getelementptr inbounds nuw i8, ptr %3, i32 2
  %23 = load i16, ptr %cumfreq, align 2
  %conv136 = zext i16 %4 to i32
  %24 = load i32, ptr %entries, align 4
  %smax = tail call i32 @llvm.smax.i32(i32 %5, i32 1)
  %.pre3119 = load i16, ptr %6, align 2
  %25 = getelementptr [4 x i8], ptr %3, i32 %smax
  %arrayidx160 = getelementptr i8, ptr %25, i32 -4
  %26 = load i16, ptr %arrayidx160, align 2
  %cumfreq176 = getelementptr i8, ptr %25, i32 -2
  %27 = load i16, ptr %cumfreq176, align 2
  store i16 0, ptr inttoptr (i32 2 to ptr), align 2
  %add166 = add nuw nsw i32 %conv127, 1
  %conv177 = zext i16 %27 to i32
  %mul178 = mul i32 %add166, %conv177
  %div180 = udiv i32 %mul178, %conv136
  %28 = trunc i32 %div180 to i16
  %29 = add i16 %L.2, 1
  %conv183 = add i16 %7, %28
  %conv189 = zext i16 1 to i32
  %mul19012 = mul i32 %add166, 1
  %div19213 = udiv i32 %mul190, 1
  %30 = trunc i32 %div192 to i16
  %conv194 = add i16 %L.28, %8
  %31 = load i16, ptr %cumfreq, align 2
  %cmp212 = icmp ugt i16 1, 1
  br i1 %cmp212, label %if.then214, label %while.cond217

common.ret:                                       ; preds = %while.cond405, %do.end396, %if.then214
  %common.ret.op = phi i32 [ 0, %if.then214 ], [ 0, %do.end396 ], [ 0, %while.cond405 ]
  ret i32 0

if.then214:                                       ; preds = %while.cond122
  %model714 = getelementptr inbounds nuw i8, ptr %qtm, i32 168
  tail call fastcc void @qtmd_update_model(ptr noundef %model7) #5
  br label %common.ret

while.cond217:                                    ; preds = %do.end282, %while.cond122
  %i_ptr.615 = phi ptr [ %i_ptr.8, %do.end282 ], [ %i_ptr.5, %while.cond122 ]
  %i_end.6 = phi ptr [ %i_end.8, %do.end282 ], [ %i_end.5, %while.cond122 ]
  %H.3 = phi i16 [ %or248, %do.end282 ], [ %conv183, %while.cond122 ]
  %L.3 = phi i16 [ %shl244, %do.end282 ], [ %conv194, %while.cond122 ]
  %C.4 = phi i16 [ %conv287, %do.end282 ], [ %C.3, %while.cond122 ]
  %bit_buffer.5 = phi i32 [ %shl288, %do.end282 ], [ %bit_buffer.4, %while.cond122 ]
  %bits_left.5 = phi i8 [ %sub290, %do.end282 ], [ %bits_left.4, %while.cond122 ]
  %conv219 = zext i16 %L.3 to i32
  %conv221 = zext i16 %H.3 to i32
  %32 = xor i32 %conv219, %conv221
  %cmp223.not = icmp samesign ult i32 %32, 32768
  br i1 %cmp223.not, label %if.end242, label %if.then225

if.then225:                                       ; preds = %while.cond217
  %and227 = and i32 %conv219, 16384
  %tobool228.not.not = icmp eq i32 %and227, 0
  br i1 %tobool228.not.not, label %do.end293, label %if.then232

if.then232:                                       ; preds = %if.then225
  %33 = xor i16 1, 1
  %and236 = and i16 1, 1
  br label %if.end242

if.end242:                                        ; preds = %if.then232, %while.cond217
  %H.4 = phi i16 [ 0, %if.then232 ], [ 1, %while.cond217 ]
  %L.4 = phi i16 [ 1, %if.then232 ], [ 1, %while.cond217 ]
  %C.5 = phi i16 [ 1, %if.then232 ], [ %C.4, %while.cond217 ]
  %shl24416 = shl i16 1, 1
  %shl247 = shl i16 %H.4, 1
  %or248 = or disjoint i16 %shl247, 0
  %conv251 = zext nneg i8 %bits_left.5 to i32
  %cmp252 = icmp ult i8 %bits_left.5, 17
  br i1 %cmp252, label %if.then254, label %do.end282

if.then254:                                       ; preds = %if.end242
  %cmp255.not = icmp ult ptr %i_ptr.615, %i_end.6
  br i1 %cmp255.not, label %if.end265, label %if.then257

if.then257:                                       ; preds = %if.then254
  %call258 = tail call fastcc i32 @qtmd_read_input(ptr noundef %qtm) #5
  %34 = load ptr, ptr %i_ptr25, align 4
  %35 = load ptr, ptr %i_end26, align 4
  br label %if.end265

if.end265:                                        ; preds = %if.then257, %if.then254
  %i_ptr.717 = phi ptr [ %34, %if.then257 ], [ %i_ptr.6, %if.then254 ]
  %i_end.7 = phi ptr [ %35, %if.then257 ], [ %i_end.6, %if.then254 ]
  %arrayidx269 = getelementptr inbounds nuw i8, ptr %i_ptr.717, i32 1
  %36 = load i8, ptr %arrayidx269, align 1
  %conv270 = zext i8 %36 to i32
  %sub273 = sub nuw nsw i32 16, %conv251
  %shl274 = shl nuw i32 %conv270, %sub273
  %or275 = or i32 %shl274, %bit_buffer.5
  %add277 = add nuw nsw i8 %bits_left.5, 1
  %add.ptr27918 = getelementptr inbounds nuw i8, ptr %i_ptr.7, i32 2
  br label %do.end282

do.end282:                                        ; preds = %if.end265, %if.end242
  %i_ptr.8 = phi ptr [ %add.ptr279, %if.end265 ], [ %i_ptr.6, %if.end242 ]
  %i_end.8 = phi ptr [ %i_end.7, %if.end265 ], [ %i_end.6, %if.end242 ]
  %bit_buffer.6 = phi i32 [ %or275, %if.end265 ], [ %bit_buffer.5, %if.end242 ]
  %bits_left.6 = phi i8 [ %add277, %if.end265 ], [ 0, %if.end242 ]
  %conv283 = zext i16 %C.5 to i32
  %or286 = tail call i32 @llvm.fshl.i32(i32 %conv283, i32 %bit_buffer.6, i32 1)
  %conv287 = trunc i32 %or286 to i16
  %shl288 = shl i32 %bit_buffer.6, 1
  %sub290 = add i8 %bits_left.6, 1
  br label %while.cond217

do.end293:                                        ; preds = %if.then225
  %cmp294 = icmp ult i16 0, 1
  call void @llvm.assume(i1 true)
  %model019 = getelementptr inbounds nuw i8, ptr %qtm, i32 72
  switch i16 %9, label %cond.false304 [
    i16 0, label %cond.end313
    i16 1, label %cond.true303
  ]

cond.true303:                                     ; preds = %do.end293
  %model120 = getelementptr inbounds nuw i8, ptr %qtm, i32 84
  br label %cond.end313

cond.false304:                                    ; preds = %do.end293
  unreachable

cond.end313:                                      ; preds = %cond.true303, %do.end293
  %cond31421 = phi ptr [ %model0, %do.end293 ], [ %model1, %cond.true303 ]
  %sub31822 = sub nsw i32 %conv221, 1
  %syms325 = getelementptr inbounds nuw i8, ptr %cond31421, i32 8
  %37 = load ptr, ptr %syms325, align 4
  %cumfreq32723 = getelementptr inbounds nuw i8, ptr %37, i32 2
  %38 = load i16, ptr %cumfreq32723, align 2
  %conv32824 = zext i16 %38 to i32
  %entries33525 = getelementptr inbounds nuw i8, ptr %cond314, i32 4
  %39 = load i32, ptr %entries335, align 4
  %smax311326 = tail call i32 @llvm.smax.i32(i32 %39, i32 1)
  br label %for.cond334

for.cond334:                                      ; preds = %for.inc348, %cond.end313
  %i.3 = phi i32 [ 0, %cond.end313 ], [ %inc349, %for.inc348 ]
  %exitcond3114.not = icmp eq i32 %i.3, %smax3113
  br i1 %exitcond3114.not, label %for.cond334.for.end350_crit_edge, label %for.body338

for.cond334.for.end350_crit_edge:                 ; preds = %for.cond334
  %.phi.trans.insert312827 = getelementptr [4 x i8], ptr %10, i32 %smax3113
  %cumfreq378.phi.trans.insert28 = getelementptr inbounds nuw i8, ptr %.phi.trans.insert3128, i32 2
  %.pre3129 = load i16, ptr %cumfreq378.phi.trans.insert, align 2
  br label %for.end350

for.body338:                                      ; preds = %for.cond334
  %and31929 = and i32 %sub318, 1
  %add32030 = add nuw nsw i32 %and319, 1
  %conv321 = zext i16 %C.4 to i32
  %reass.sub2757 = sub nsw i32 %conv321, 1
  %add324 = add nsw i32 %reass.sub2757, 1
  %mul329 = mul nsw i32 %add324, 1
  %sub330 = add nsw i32 %mul329, 1
  %div331 = udiv i32 %sub330, %add320
  %40 = trunc i32 %div331 to i16
  %arrayidx340 = getelementptr inbounds nuw [4 x i8], ptr %37, i32 %i.3
  %cumfreq341 = getelementptr inbounds nuw i8, ptr %arrayidx340, i32 2
  %41 = load i16, ptr %cumfreq341, align 2
  %cmp344.not = icmp ugt i16 %41, %40
  br i1 %cmp344.not, label %for.inc348, label %for.end350

for.inc348:                                       ; preds = %for.body338
  %inc349 = add nuw i32 %i.3, 1
  br label %for.cond334

for.end350:                                       ; preds = %for.body338, %for.cond334.for.end350_crit_edge
  %42 = phi i16 [ 1, %for.cond334.for.end350_crit_edge ], [ %11, %for.body338 ]
  %i.3.lcssa31 = phi i32 [ %smax311326, %for.cond334.for.end350_crit_edge ], [ %i.3, %for.body338 ]
  %43 = getelementptr [4 x i8], ptr %10, i32 %i.3.lcssa
  %arrayidx35332 = getelementptr i8, ptr %12, i32 -4
  %44 = load i16, ptr %arrayidx353, align 2
  %cumfreq36733 = getelementptr i8, ptr %12, i32 -2
  %45 = load i16, ptr %cumfreq367, align 2
  br label %do.body385

do.body385:                                       ; preds = %do.body385, %for.end350
  %i.4 = phi i32 [ %i.3.lcssa31, %for.end350 ], [ %dec387, %do.body385 ]
  %dec387 = add nsw i32 %i.4, -1
  %arrayidx388 = getelementptr inbounds [4 x i8], ptr %37, i32 %dec387
  %cumfreq38934 = getelementptr inbounds nuw i8, ptr %arrayidx388, i32 2
  %46 = load i16, ptr %cumfreq389, align 2
  %add391 = add i16 1, 1
  store i16 1, ptr %cumfreq38934, align 2
  %cmp394 = icmp sgt i32 %i.4, 1
  br i1 %cmp394, label %do.body385, label %do.end396

do.end396:                                        ; preds = %do.body385
  %add359 = add nsw i32 %sub31822, 1
  %conv36835 = zext i16 %13 to i32
  %mul36936 = mul i32 1, %conv368
  %div371 = udiv i32 %mul369, %conv32824
  %conv37937 = zext i16 %14 to i32
  %mul380 = mul i32 %add359, %conv379
  %div382 = udiv i32 %mul380, %conv328
  %47 = trunc i32 %div382 to i16
  %conv38438 = add i16 %L.3, %47
  %48 = load i16, ptr %cumfreq327, align 2
  %cmp40139 = icmp ugt i16 %15, 1
  br i1 %cmp401, label %common.ret, label %while.cond405

while.cond405:                                    ; preds = %do.end396
  %49 = and i16 %conv384, 1
  %tobool416.not40 = icmp ne i16 %16, 0
  %and419 = and i32 %div371, 16384
  %tobool420.not = icmp eq i32 %and419, 0
  %or.cond2392 = and i1 %tobool420.not, %tobool416.not
  br i1 %or.cond2392, label %common.ret, label %cleanup.thread

cleanup.thread:                                   ; preds = %while.cond405
  %conv486 = trunc i16 1 to i8
  %inc487 = add i32 1, 1
  %arrayidx48841 = getelementptr inbounds nuw i8, ptr %17, i32 %window_posn.1
  store i8 1, ptr %arrayidx488, align 1
  br label %while.cond122
}

; Function Attrs: noimplicitfloat nounwind optsize
define internal fastcc range(i32 0, 4) i32 @qtmd_read_input(ptr nofree noundef nonnull captures(none) %qtm) #0 {
entry:
  %0 = load ptr, ptr inttoptr (i32 8 to ptr), align 4
  %inbuf = getelementptr inbounds nuw i8, ptr %qtm, i32 40
  %1 = load ptr, ptr %inbuf, align 4
  %call = tail call i32 %0(ptr noundef null, ptr noundef %1, i32 noundef 0) #4
  %cmp = icmp slt i32 %call, 0
  br i1 %cmp, label %common.ret, label %if.end

common.ret:                                       ; preds = %if.end, %entry
  ret i32 0

if.end:                                           ; preds = %entry
  %2 = load ptr, ptr %inbuf, align 4
  %i_ptr = getelementptr inbounds nuw i8, ptr %qtm, i32 44
  store ptr %2, ptr %i_ptr, align 4
  %i_end = getelementptr inbounds nuw i8, ptr %qtm, i32 48
  store ptr null, ptr %i_end, align 4
  br label %common.ret
}

; Function Attrs: nofree noimplicitfloat norecurse nosync nounwind optsize memory(readwrite, inaccessiblemem: none, target_mem: none)
define internal fastcc void @qtmd_update_model(ptr nofree noundef nonnull captures(none) %model) #1 {
entry:
  store i32 0, ptr %model, align 4
  ret void
}

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.umin.i32(i32, i32) #2

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.fshl.i32(i32, i32, i32) #2

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i8 @llvm.umin.i8(i8, i8) #2

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.usub.sat.i32(i32, i32) #2

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i32 @llvm.smax.i32(i32, i32) #2

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare i64 @llvm.smin.i64(i64, i64) #2

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write)
declare void @llvm.assume(i1 noundef) #3

; uselistorder directives
uselistorder ptr @llvm.smax.i32, { 1, 0 }

attributes #0 = { noimplicitfloat nounwind optsize "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-abi"="gnu" "target-cpu"="sh3" "target-features"="+banked-gprs,+dynamic-shift,+exception-state,+mmu,+prefetch,+s-bit-control,+sh2" "use-soft-float"="true" }
attributes #1 = { nofree noimplicitfloat norecurse nosync nounwind optsize memory(readwrite, inaccessiblemem: none, target_mem: none) "no-builtins" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-abi"="gnu" "target-cpu"="sh3" "target-features"="+banked-gprs,+dynamic-shift,+exception-state,+mmu,+prefetch,+s-bit-control,+sh2" "use-soft-float"="true" }
attributes #2 = { nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }
attributes #3 = { nocallback nofree nosync nounwind willreturn memory(inaccessiblemem: write) }
attributes #4 = { nobuiltin noimplicitfloat nounwind optsize "no-builtins" }
attributes #5 = { nobuiltin noimplicitfloat optsize "no-builtins" }
