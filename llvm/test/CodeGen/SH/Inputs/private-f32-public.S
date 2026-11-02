.text
.p2align 2
.global ordinary_boundary
.type ordinary_boundary,@function
ordinary_boundary:
	sts fpscr,r0
	mov.l .Lmode_mask,r1
	and r1,r0
	mov.l .Lobserved_boundary,r1
	mov.l r0,@r1
	sts fpscr,r0
	or #4,r0
	lds r0,fpscr
	! GNU hard-float scalar argument word order follows the target endian.
.ifdef SH_F32_PUBLIC_BE
	fmov fr4,fr0
.else
	fmov fr5,fr0
.endif
	rts
	nop
.p2align 2
.Lmode_mask:
	.long 0x00380000
.Lobserved_boundary:
	.long observed_boundary
.size ordinary_boundary,.-ordinary_boundary
.bss
.p2align 2
.global observed_boundary
observed_boundary:
	.space 4
