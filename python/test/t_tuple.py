#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from collections import namedtuple

def main():

	print("hello, test namedtuple")

	MyStock = namedtuple("MyStock", "symbol high low current")

	stk = MyStock("apple", 600, 200, 450)

	print(stk.symbol)
	print(stk.high)
	print(stk.low)
	print(stk.current)



if __name__ == '__main__':
	main()
