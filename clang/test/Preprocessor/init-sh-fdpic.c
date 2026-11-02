// RUN: %clang -target sh2-linux-muslfdpic -dM -E -x c /dev/null | FileCheck %s --check-prefixes=FDPIC,LE
// RUN: %clang -target sh2eb-linux-muslfdpic -dM -E -x c /dev/null | FileCheck %s --check-prefixes=FDPIC,BE
// RUN: %clang -target sh2-linux-musl -mfdpic -dM -E -x c /dev/null | FileCheck %s --check-prefixes=FDPIC,LE
// RUN: %clang -target sh2-linux-musl -dM -E -x c /dev/null | FileCheck %s --check-prefix=ORDINARY
// RUN: %clang -target sh2-linux-muslfdpic -mno-fdpic -dM -E -x c /dev/null | FileCheck %s --check-prefix=ORDINARY

// FDPIC-DAG: #define __FDPIC__ 1
// FDPIC-DAG: #define __SH_FDPIC__ 1
// FDPIC-DAG: #define __SH2__ 1
// FDPIC-DAG: #define __SIZEOF_POINTER__ 4
// FDPIC-DAG: #define __sh__ 1
// LE-DAG: #define __BYTE_ORDER__ __ORDER_LITTLE_ENDIAN__
// BE-DAG: #define __BYTE_ORDER__ __ORDER_BIG_ENDIAN__
// ORDINARY-NOT: __FDPIC__
// ORDINARY-NOT: __SH_FDPIC__
