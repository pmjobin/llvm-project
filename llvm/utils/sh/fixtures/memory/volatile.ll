; Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
; SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

declare void @llvm.memcpy.p0.p0.i32(ptr, ptr, i32, i1 immarg)
declare void @llvm.memmove.p0.p0.i32(ptr, ptr, i32, i1 immarg)
declare void @llvm.memset.p0.i32(ptr, i8, i32, i1 immarg)

define void @volatile_copy(ptr %d, ptr %s, i32 %unused) optsize {
  call void @llvm.memcpy.p0.p0.i32(ptr align 1 %d, ptr align 1 %s, i32 60, i1 true)
  ret void
}

define void @volatile_move(ptr %d, ptr %s, i32 %unused) optsize {
  call void @llvm.memmove.p0.p0.i32(ptr align 1 %d, ptr align 1 %s, i32 16, i1 true)
  ret void
}

define void @volatile_set(ptr %d, ptr %s, i32 %unused) optsize {
  call void @llvm.memset.p0.i32(ptr align 1 %d, i8 165, i32 60, i1 true)
  ret void
}
