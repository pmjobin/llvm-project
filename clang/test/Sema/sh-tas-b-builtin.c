// RUN: %clang_cc1 -triple sh-unknown-elf -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple shle-unknown-elf -fsyntax-only -verify %s

void invalid(void) {
	__builtin_sh_tas_b(); // expected-error {{too few arguments to function call, expected 1, have 0}}
	__builtin_sh_tas_b(1); // expected-error {{incompatible integer to pointer conversion passing 'int' to parameter of type 'volatile unsigned char *'}}
	__builtin_sh_tas_b((unsigned short *)0); // expected-error {{incompatible pointer types passing 'unsigned short *' to parameter of type 'volatile unsigned char *'}}
	__builtin_sh_tas_b((const unsigned char *)0); // expected-warning {{passing 'const unsigned char *' to parameter of type 'volatile unsigned char *' discards qualifiers}}
	__builtin_sh_tas_b((unsigned char *)0, 1); // expected-error {{too many arguments to function call, expected 1, have 2}}
}
