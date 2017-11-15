#!/usr/bin/env python3
# -*- coding: utf-8 -*-

class MyClass:

	__slots__ = ["_val"]

	def __init__(self, arg):
		self._val = arg

	
	@property
	def val(self):
		print(u"get value")
		return self._val

	@val.setter
	def val(self, val):
		print(u"set value")
		self._val = val

	@val.deleter
	def val(self):
		print(u"delete value")
		del self._val
		


def main():
	print(u"hello, testing slots")

	my = MyClass("hello")

#	my.y = "new"
#	print("my value: {}".format(my.y))

	print("my value: {}".format(my.val))
	del my.val

	my.val = "new value"
	print("my value: {}".format(my.val))





if __name__ == '__main__':
	main()
