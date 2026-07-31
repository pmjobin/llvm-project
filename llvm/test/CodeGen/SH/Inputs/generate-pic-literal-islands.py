#!/usr/bin/env python3

import sys


def main():
	print("declare i32 @external_function(i32)")
	print()
	print("define i32 @pic_literal_clones(ptr %slot, i32 %value) {")
	print("entry:")
	print("\t%first = call i32 @external_function(i32 %value)")
	print("\tstore volatile i32 %first, ptr %slot, align 4")
	print("\tbr label %pad0")
	for index in range(300):
		print()
		print(f"pad{index}:")
		print(f"\tstore volatile i32 {index % 128}, ptr %slot, align 4")
		print(f"\tbr label %pad{index + 1}")
	print()
	print("pad300:")
	print("\t%tail = load volatile i32, ptr %slot, align 4")
	print("\t%last = call i32 @external_function(i32 %tail)")
	print("\tret i32 %last")
	print("}")


if __name__ == "__main__":
	sys.exit(main())
