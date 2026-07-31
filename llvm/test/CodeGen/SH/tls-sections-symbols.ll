; RUN: llc -mtriple=sh-unknown-elf -relocation-model=static -data-sections -filetype=obj < %s -o %t.be.o
; RUN: llc -mtriple=shle-unknown-elf -relocation-model=static -data-sections -filetype=obj < %s -o %t.le.o
; RUN: llvm-readobj --sections --symbols %t.be.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readobj --sections --symbols %t.le.o | FileCheck %s --check-prefix=OBJECT
; RUN: llvm-readelf -hSWs %t.be.o | FileCheck %s --check-prefix=READELF
; RUN: llvm-readelf -hSWs %t.le.o | FileCheck %s --check-prefix=READELF
; RUN: %python %S/Inputs/check-sh-pic-elf.py llvm-readelf %t.be.o %t.le.o

@initialized_tls = thread_local(localexec) global i32 42, align 4
@zero_tls = internal thread_local(localexec) global [4 x i32] zeroinitializer, align 16
@hidden_float_tls = hidden thread_local(localexec) global float 1.500000e+00, align 4
@protected_double_tls = protected thread_local(localexec) global double 2.500000e+00, align 8
@aggregate_tls = internal thread_local(localexec) global { i32, [2 x i16] } { i32 7, [2 x i16] [i16 8, i16 9] }, align 8
@external_tls = external thread_local global i32, align 4

; OBJECT-DAG: Name: .tdata.initialized_tls
; OBJECT-DAG: Type: SHT_PROGBITS
; OBJECT-DAG: SHF_TLS
; OBJECT-DAG: Name: .tbss.zero_tls
; OBJECT-DAG: Type: SHT_NOBITS
; OBJECT-DAG: SHF_TLS
; OBJECT-DAG: AddressAlignment: 16
; OBJECT-DAG: Name: initialized_tls
; OBJECT-DAG: Size: 4
; OBJECT-DAG: Binding: Global
; OBJECT-DAG: Type: TLS
; OBJECT-DAG: Name: zero_tls
; OBJECT-DAG: Size: 16
; OBJECT-DAG: Binding: Local
; OBJECT-DAG: Type: TLS
; OBJECT-DAG: Name: hidden_float_tls
; OBJECT-DAG: Type: TLS
; OBJECT-DAG: Other [ (0x2)
; OBJECT-DAG: STV_HIDDEN
; OBJECT-DAG: Name: protected_double_tls
; OBJECT-DAG: Type: TLS
; OBJECT-DAG: Other [ (0x3)
; OBJECT-DAG: STV_PROTECTED
; OBJECT-DAG: Name: aggregate_tls
; OBJECT-DAG: Size: 8
; OBJECT-DAG: Type: TLS
; OBJECT-DAG: Name: external_tls
; OBJECT-DAG: Type: TLS
; OBJECT-DAG: Section: Undefined

; READELF-DAG: .tdata.initialized_tls PROGBITS
; READELF-DAG: .tbss.zero_tls NOBITS
; READELF-DAG: TLS{{.*}}GLOBAL{{.*}}initialized_tls
; READELF-DAG: TLS{{.*}}LOCAL{{.*}}zero_tls
; READELF-DAG: TLS{{.*}}GLOBAL HIDDEN{{.*}}hidden_float_tls
; READELF-DAG: TLS{{.*}}GLOBAL PROTECTED{{.*}}protected_double_tls
; READELF-DAG: TLS{{.*}}GLOBAL DEFAULT{{.*}}UND external_tls
