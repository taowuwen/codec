#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class A:
	def do_print(self):
		print("this is class A")

def do_print():
	print("this is just a function, not a class method")


def main():
	print("hello, monkey patching")

	a = A()
	
	a.do_print()
	a.do_print = do_print
	a.do_print()



if __name__ == '__main__':
	main()


