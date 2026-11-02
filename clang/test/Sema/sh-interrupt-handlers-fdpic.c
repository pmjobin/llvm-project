// RUN: %clang_cc1 -triple sh2-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -fsyntax-only -verify %s
// RUN: %clang_cc1 -triple sh2eb-unknown-linux-muslfdpic -target-cpu sh2 -target-abi gnu-fdpic -fsyntax-only -verify %s

__attribute__((interrupt_handler)) void interrupt_fdpic(void) {} // expected-error {{'interrupt_handler' attribute is unsupported with the SuperH FDPIC ABI because hardware entry provides neither the required incoming r12 context nor a function descriptor entry point}}
__attribute__((trapa_handler)) void trapa_fdpic(void) {} // expected-error {{'trapa_handler' attribute is unsupported with the SuperH FDPIC ABI because hardware entry provides neither the required incoming r12 context nor a function descriptor entry point}}
