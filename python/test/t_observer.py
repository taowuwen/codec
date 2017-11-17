#!/usr/bin/env python3
# -*- coding: utf-8 -*-

class Inventory:
	__slots__ = [ "observers", "_product", "_quantity" ]

	def __init__(self):
		self.observers = []
		self._product = None
		self._quantity = 0

	def attach(self, observer):
		self.observers.append(observer)
	
	def _update_observers(self):
		for obs in self.observers:
			obs()

	@property
	def product(self):
		return self._product

	@product.setter
	def product(self, val):
		print("my setter into new value: {}".format(val))
		self._product = val
		self._update_observers()

	@property
	def quantity(self):
		return self._quantity

	@quantity.setter
	def quantity(self, val):
		print("update my quantity into new value {}".format(val))
		self._quantity = val
	
		self._update_observers()


class ConsoleObserver:

	__slots__ = ["inv"]

	def __init__(self, inventory):
		self.inv = inventory

	def __call__(self):
		print(self.inv.product)
		print(self.inv.quantity)

def main():
	print("hello, my observer")

	inv = Inventory()

	inv.attach(ConsoleObserver(inv))
	inv.attach(ConsoleObserver(inv))

	inv.product = "goog"
	inv.quantity = 5

if __name__ == '__main__':
	main()


