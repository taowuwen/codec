#!/usr/bin/env python3

import os, sys, stat, math

money = 1000

def addMoney():
	global money
#	money = 1
	money = money + 1
	print("inner, money:", money)
	return money

def testglob():
	import glob
	print(glob.glob("*.py"))


def main():
	print("hello, world")
	print("money old = ", money)
	print("money = ", addMoney())
	print("money outter = ", money)
	testglob()

def testdir():
	print("\n")
	print("=============  OS   =========================")
	print(dir(os))
	print("\n")
	print("=============  SYS  =========================")
	print(dir(sys))
	print("\n")
	print("=============  STAT =========================")
	print(dir(stat))

	print("\n")
	print("=============  MATH =========================")
	print(dir(math))

if __name__ == '__main__':
	main()

