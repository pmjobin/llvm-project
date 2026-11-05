// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

// Test-only QEMU Linux-user adapter. Board applications must provide their own
// I/O and process termination. This adapter exposes no host filesystem.
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/_initfini.h>
#include <sys/types.h>
#include <unistd.h>

extern int platform_write(const void *, unsigned);
extern void platform_exit(int) __attribute__((noreturn));

ssize_t write(int FD, const void *Data, size_t Size) {
  if (FD == 1 || FD == 2)
    return platform_write(Data, Size);
  errno = EBADF;
  return -1;
}

ssize_t read(int FD, void *Data, size_t Size) {
  (void)Data;
  (void)Size;
  if (FD == 0)
    return 0;
  errno = EBADF;
  return -1;
}

off_t lseek(int FD, off_t Offset, int Whence) {
  (void)FD;
  (void)Offset;
  (void)Whence;
  errno = ESPIPE;
  return -1;
}

int close(int FD) {
  (void)FD;
  errno = EBADF;
  return -1;
}

void _exit(int Code) { platform_exit(Code); }

int sh4_start(void) {
  extern int main(void);
  __libc_init_array();
  setvbuf(stdin, NULL, _IONBF, 0);
  setvbuf(stdout, NULL, _IONBF, 0);
  setvbuf(stderr, NULL, _IONBF, 0);
  exit(main());
}
