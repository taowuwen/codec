#!/usr/bin/env python3
# -*- coding: utf-8 -*-



class Property:

	__slots__ = ["_a", "_b"]

	def __init__(self, a = 1, b = 2):
		self._a = a
		self._b = b

	@property
	def a(self):
		return self._a

	@a.setter
	def a(self, val):
		print("set new value: a = {}".format(val))
		self._a = val



def main():
	print("hello, my property")

	p = Property()

	print(p.a)
	p.a = "hello"
	print(p.a)


if __name__ == '__main__':
	main()



