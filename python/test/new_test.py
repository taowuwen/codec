#!/usr/bin/env python3
# -*- coding: utf-8 -*-

class Inventory:
	__slots__ = [ "observers", "_product", "_quantity" ]

	def __init__(self):
		self.observers = []
		self._product = None
		self._quantity = 0


	@property
	def product(self):
		return self._product


def main():
	print("hello, my observer")

	inv = Inventory()
	inv.product = "goog"
	inv.quantity = 5
	inv.a = "aaa"

if __name__ == '__main__':
	main()


