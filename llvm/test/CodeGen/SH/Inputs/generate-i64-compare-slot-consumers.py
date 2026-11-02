#!/usr/bin/env python3
"""Generate all integer relations with independent branch and value consumers."""
import sys

out=''
for pred in ('ult','ule','ugt','uge','slt','sle','sgt','sge','eq','ne'):
	for form in ('select','branch','boolean','accumulate','multiple','phi','clobber'):
		name=form+'_'+pred
		ret='i32' if form in ('boolean','accumulate') else 'i64'
		out+=f'define {ret} @{name}(i64 %a, i64 %b, i64 %x, i64 %y, i32 %n, ptr %p) {{\n\t%c = icmp {pred} i64 %a, %b\n'
		if form in ('branch','phi'):
			out+='\tbr i1 %c, label %yes, label %no\nyes:\n'
			if form=='branch': out+='\tret i64 %x\nno:\n\tret i64 %y\n'
			else: out+='\tstore volatile i64 %x, ptr %p\n\tbr label %join\nno:\n\tstore volatile i64 %y, ptr %p\n\tbr label %join\njoin:\n\t%r = phi i64 [ %x, %yes ], [ %y, %no ]\n\tret i64 %r\n'
		elif form in ('boolean','accumulate'):
			out+='\t%v = zext i1 %c to i32\n'
			if form=='accumulate': out+='\t%r = add i32 %v, %n\n\tret i32 %r\n'
			else: out+='\tret i32 %v\n'
		else:
			if form=='multiple': out+='\t%v = zext i1 %c to i32\n\tstore i32 %v, ptr %p\n'
			if form=='clobber': out+='\tcall void asm sideeffect "sett", "~{tbit}"()\n'
			out+='\t%r = select i1 %c, i64 %x, i64 %y\n\tret i64 %r\n'
		out+='}\n\n'
		# Unsigned selects consume the borrow directly. Signed selects use the
		# native high-equality gate and an unsigned low-word predicate. All seven
		# forms also execute against literal oracles in --case i64-relations.
		if form=='select' and pred.startswith('u'):
			low='hi' if pred in ('ult','ugt') else 'hs'
			out+=f'; OPT-LABEL: {name}:\n; OPT: cmp/{low}\n; OPT-NEXT: subc\n; OPT-NEXT: bf\n; OPT: rts\n\n'
		if form=='select' and pred.startswith('s'):
			low='hi' if pred in ('slt','sgt') else 'hs'
			high='gt' if pred in ('slt','sgt') else 'ge'
			out+=f'; OPT-LABEL: {name}:\n; OPT: cmp/{low}\n; OPT-NEXT: movt [[LOW:r[0-9]+]]\n; OPT: cmp/{high}\n; OPT-NEXT: movt [[HIGH:r[0-9]+]]\n; OPT: cmp/eq\n; OPT-NEXT: bt [[JOIN:.LBB[0-9]+_[0-9]+]]\n; OPT: mov [[HIGH]],[[LOW]]\n; OPT: [[JOIN]]:\n; OPT-NEXT: tst [[LOW]],[[LOW]]\n; OPT-NEXT: bt\n; OPT: rts\n\n'
for name,a,b,pred in [('equal_high_low_boundary',4294967295,4294967296,'ult'),('signed_min_max',-9223372036854775808,9223372036854775807,'slt'),('unsigned_max',-1,0,'ugt'),('threshold_equal',-9223372036854775808,-9223372036854775808,'eq')]:
	out+=f'define i32 @{name}(ptr %p) {{\n\tstore volatile i64 {a}, ptr %p\n\t%v = load volatile i64, ptr %p\n\t%c = icmp {pred} i64 %v, {b}\n\t%r = zext i1 %c to i32\n\tret i32 %r\n}}\n\n'
sys.stdout.write(out)
