// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
typedef float _Complex CF;
typedef double _Complex CD;
struct W {
  CF z;
};
struct N {
  struct W w;
};
struct D {
  CD z;
};
struct ND {
  struct D d;
};
struct P {
  CF z;
} __attribute__((packed));
struct H {
  CF z;
} __attribute__((aligned(16)));
struct M {
  float a, b;
};
struct SF {
  float x;
};
extern unsigned result[256][8];
void var_W_0(int tag, ...);
void var_W_1(float f0, int tag, ...);
void var_W_2(float f0, float f1, int tag, ...);
void var_W_4(float f0, float f1, float f2, float f3, int tag, ...);
void var_W_5(float f0, float f1, float f2, float f3, float f4, int tag, ...);
void var_W_6(float f0, float f1, float f2, float f3, float f4, float f5,
             int tag, ...);
void var_W_7(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, int tag, ...);
void var_W_8(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, float f7, int tag, ...);
void named_W_0(struct W v, unsigned tail);
void named_W_1(float f0, struct W v, unsigned tail);
void named_W_6(float f0, float f1, float f2, float f3, float f4, float f5,
               struct W v, unsigned tail);
void named_W_7(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, struct W v, unsigned tail);
void named_W_8(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, float f7, struct W v, unsigned tail);
struct W return_W(void);
void var_N_0(int tag, ...);
void var_N_1(float f0, int tag, ...);
void var_N_2(float f0, float f1, int tag, ...);
void var_N_4(float f0, float f1, float f2, float f3, int tag, ...);
void var_N_5(float f0, float f1, float f2, float f3, float f4, int tag, ...);
void var_N_6(float f0, float f1, float f2, float f3, float f4, float f5,
             int tag, ...);
void var_N_7(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, int tag, ...);
void var_N_8(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, float f7, int tag, ...);
void named_N_0(struct N v, unsigned tail);
void named_N_1(float f0, struct N v, unsigned tail);
void named_N_6(float f0, float f1, float f2, float f3, float f4, float f5,
               struct N v, unsigned tail);
void named_N_7(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, struct N v, unsigned tail);
void named_N_8(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, float f7, struct N v, unsigned tail);
struct N return_N(void);
void var_D_0(int tag, ...);
void var_D_1(float f0, int tag, ...);
void var_D_2(float f0, float f1, int tag, ...);
void var_D_4(float f0, float f1, float f2, float f3, int tag, ...);
void var_D_5(float f0, float f1, float f2, float f3, float f4, int tag, ...);
void var_D_6(float f0, float f1, float f2, float f3, float f4, float f5,
             int tag, ...);
void var_D_7(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, int tag, ...);
void var_D_8(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, float f7, int tag, ...);
void named_D_0(struct D v, unsigned tail);
void named_D_1(float f0, struct D v, unsigned tail);
void named_D_6(float f0, float f1, float f2, float f3, float f4, float f5,
               struct D v, unsigned tail);
void named_D_7(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, struct D v, unsigned tail);
void named_D_8(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, float f7, struct D v, unsigned tail);
struct D return_D(void);
void var_ND_0(int tag, ...);
void var_ND_1(float f0, int tag, ...);
void var_ND_2(float f0, float f1, int tag, ...);
void var_ND_4(float f0, float f1, float f2, float f3, int tag, ...);
void var_ND_5(float f0, float f1, float f2, float f3, float f4, int tag, ...);
void var_ND_6(float f0, float f1, float f2, float f3, float f4, float f5,
              int tag, ...);
void var_ND_7(float f0, float f1, float f2, float f3, float f4, float f5,
              float f6, int tag, ...);
void var_ND_8(float f0, float f1, float f2, float f3, float f4, float f5,
              float f6, float f7, int tag, ...);
void named_ND_0(struct ND v, unsigned tail);
void named_ND_1(float f0, struct ND v, unsigned tail);
void named_ND_6(float f0, float f1, float f2, float f3, float f4, float f5,
                struct ND v, unsigned tail);
void named_ND_7(float f0, float f1, float f2, float f3, float f4, float f5,
                float f6, struct ND v, unsigned tail);
void named_ND_8(float f0, float f1, float f2, float f3, float f4, float f5,
                float f6, float f7, struct ND v, unsigned tail);
struct ND return_ND(void);
void var_P_0(int tag, ...);
void var_P_1(float f0, int tag, ...);
void var_P_2(float f0, float f1, int tag, ...);
void var_P_4(float f0, float f1, float f2, float f3, int tag, ...);
void var_P_5(float f0, float f1, float f2, float f3, float f4, int tag, ...);
void var_P_6(float f0, float f1, float f2, float f3, float f4, float f5,
             int tag, ...);
void var_P_7(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, int tag, ...);
void var_P_8(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, float f7, int tag, ...);
void named_P_0(struct P v, unsigned tail);
void named_P_1(float f0, struct P v, unsigned tail);
void named_P_6(float f0, float f1, float f2, float f3, float f4, float f5,
               struct P v, unsigned tail);
void named_P_7(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, struct P v, unsigned tail);
void named_P_8(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, float f7, struct P v, unsigned tail);
struct P return_P(void);
void var_H_0(int tag, ...);
void var_H_1(float f0, int tag, ...);
void var_H_2(float f0, float f1, int tag, ...);
void var_H_4(float f0, float f1, float f2, float f3, int tag, ...);
void var_H_5(float f0, float f1, float f2, float f3, float f4, int tag, ...);
void var_H_6(float f0, float f1, float f2, float f3, float f4, float f5,
             int tag, ...);
void var_H_7(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, int tag, ...);
void var_H_8(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, float f7, int tag, ...);
void named_H_0(struct H v, unsigned tail);
void named_H_1(float f0, struct H v, unsigned tail);
void named_H_6(float f0, float f1, float f2, float f3, float f4, float f5,
               struct H v, unsigned tail);
void named_H_7(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, struct H v, unsigned tail);
void named_H_8(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, float f7, struct H v, unsigned tail);
struct H return_H(void);
void var_M_0(int tag, ...);
void var_M_1(float f0, int tag, ...);
void var_M_2(float f0, float f1, int tag, ...);
void var_M_4(float f0, float f1, float f2, float f3, int tag, ...);
void var_M_5(float f0, float f1, float f2, float f3, float f4, int tag, ...);
void var_M_6(float f0, float f1, float f2, float f3, float f4, float f5,
             int tag, ...);
void var_M_7(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, int tag, ...);
void var_M_8(float f0, float f1, float f2, float f3, float f4, float f5,
             float f6, float f7, int tag, ...);
void named_M_0(struct M v, unsigned tail);
void named_M_1(float f0, struct M v, unsigned tail);
void named_M_6(float f0, float f1, float f2, float f3, float f4, float f5,
               struct M v, unsigned tail);
void named_M_7(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, struct M v, unsigned tail);
void named_M_8(float f0, float f1, float f2, float f3, float f4, float f5,
               float f6, float f7, struct M v, unsigned tail);
struct M return_M(void);
