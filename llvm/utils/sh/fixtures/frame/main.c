// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#include <stdint.h>
#include <stdio.h>

extern unsigned frame_gpr(unsigned);
extern unsigned frame_fpr(unsigned);
extern unsigned frame_large(unsigned);
extern unsigned frame_dynamic(unsigned);
extern unsigned frame_realigned(unsigned);

// This TU is the GNU caller. The empty read/write asm operands make the values
// observed after the call unknown to the optimizer while keeping each sentinel
// in its named architectural register. Omitting this caller's frame pointer
// permits testing r14 even when the provider uses it as a frame pointer.
__attribute__((noinline, optimize("omit-frame-pointer"))) static unsigned
check_frame(unsigned (*function)(unsigned), unsigned input, unsigned expected) {
  register unsigned r8 __asm__("r8") = 0x10203048;
  register unsigned r9 __asm__("r9") = 0x20304059;
  register unsigned r10 __asm__("r10") = 0x3040506a;
  register unsigned r11 __asm__("r11") = 0x4050607b;
  register unsigned r12 __asm__("r12") = 0x5060708c;
  register unsigned r13 __asm__("r13") = 0x6070809d;
  register unsigned r14 __asm__("r14") = 0x708090ae;
  register float fr12 __asm__("fr12") = 1.25f;
  register float fr13 __asm__("fr13") = -2.5f;
  register float fr14 __asm__("fr14") = 3.75f;
  register float fr15 __asm__("fr15") = -4.5f;
  __asm__ volatile(""
                   : "+r"(r8), "+r"(r9), "+r"(r10), "+r"(r11), "+r"(r12),
                     "+r"(r13), "+r"(r14), "+f"(fr12), "+f"(fr13), "+f"(fr14),
                     "+f"(fr15));
  unsigned result = function(input);
  __asm__ volatile(""
                   : "+r"(r8), "+r"(r9), "+r"(r10), "+r"(r11), "+r"(r12),
                     "+r"(r13), "+r"(r14), "+f"(fr12), "+f"(fr13), "+f"(fr14),
                     "+f"(fr15));
  return (result != expected) | ((r8 != 0x10203048) << 1) |
         ((r9 != 0x20304059) << 2) | ((r10 != 0x3040506a) << 3) |
         ((r11 != 0x4050607b) << 4) | ((r12 != 0x5060708c) << 5) |
         ((r13 != 0x6070809d) << 6) | ((r14 != 0x708090ae) << 7) |
         ((fr12 != 1.25f) << 8) | ((fr13 != -2.5f) << 9) |
         ((fr14 != 3.75f) << 10) | ((fr15 != -4.5f) << 11);
}

// The external call retains the full provider allocation and its non-leaf PR
// save. Check alignment and both ends of the allocated region, then require the
// provider to read back the writes after returning through its own frame.
__attribute__((noinline)) unsigned frame_observe(volatile unsigned char *buffer,
                                                 unsigned size,
                                                 unsigned alignment) {
  if ((uintptr_t)buffer & (alignment - 1))
    return 0x10000;
  unsigned sum = buffer[0] + buffer[size - 1];
  buffer[0] = 31;
  buffer[size - 1] = 47;
  return sum + size;
}

int main(void) {
  const struct {
    unsigned (*function)(unsigned);
    unsigned input;
    unsigned expected;
  } cases[] = {{frame_gpr, 13, 14},         {frame_fpr, 17, 19},
               {frame_large, 67, 2239},     {frame_dynamic, 37, 161},
               {frame_dynamic, 129, 253},   {frame_realigned, 43, 1315},
               {frame_realigned, 131, 1403}};
  for (unsigned i = 0; i != sizeof cases / sizeof *cases; ++i) {
    unsigned failed =
        check_frame(cases[i].function, cases[i].input, cases[i].expected);
    if (failed) {
      printf("frame-preservation:FAIL:%u:%03x\n", i, failed);
      return 1;
    }
  }
  puts("frame-preservation:PASS");
  return 0;
}
