#!/usr/bin/env python3


import collections.abc
import timeit

class Power1(collections.abc.Callable):
	def __call__(self, x, n):
		p = 1

		for i in range(n):
			p *= x

		return p




class Power2(collections.abc.Callable):
	def __call_(self, x, n):
		p = 1
		for i in range(n):
			p *= x
		return p



class Power3():
	def __call_(self, x, n):
		p = 1

		for i in range(n):
			p *= x

		return p



#import timeit
#
#iterative = timeit.timeit( "pow1(2, 1024)", """
#
#import collections.abc
#
#class Power1(collections.abc.Callable):
#	def __call__(self, x, n):
#		p = 1
#
#		for i in range(n):
#			p *= x
#
#		return p
#
#
#pow1 = Power1()
#""", number = 100000)
#print("iterative", iterative)



#iterative = timeit.timeit( "pow_x(2, 1024)", """
#
#import collections.abc
#
#class Power4(collections.abc.Callable):
#	def __call__(self, x, n):
#		if n == 0:
#			return 1
#
#		if n % 2 == 1:
#			return self.__call__(x, n - 1) * x
#
#		if n % 2 == 0:
#			t = self.__call__(x, n // 2)
#			return t * t
#
#
#pow_x= Power4()
#""", number = 100000)
#print("iterative", iterative)

class Power4(collections.abc.Callable):
	def __call__(self, x, n):
		if n == 0:
			return 1

		if n % 2 == 1:
			return self.__call__(x, n - 1) * x

		if n % 2 == 0:
			t = self.__call__(x, n // 2)
			return t * t





class Power5(collections.abc.Callable):
	def __init__(self):
		self.memo = {}

	def __call__(self, x, n):
		if n == 0:
			self.memo[x, n] = 1

		elif n % 2 == 1:
			self.memo[x, n] = self.__call__(x, n-1) * x

		elif n % 2 == 0:
			t = self.__call__(x, n // 2)
			self.memo[x, n] = t * t

		else:
			assert False
			raise Exception("Logic Error")

		return self.memo[x,n]


#iterative = timeit.timeit( "pow_x(2, 1024)", """
#
#import collections.abc
#
#class Power5(collections.abc.Callable):
#	def __init__(self):
#		self.memo = {}
#
#	def __call__(self, x, n):
#		if not self.memo.get((x, n), None) is None:
#			return self.memo[x, n]
#
#		if n == 0:
#			self.memo[x, n] = 1
#
#		elif n % 2 == 1:
#			self.memo[x, n] = self.__call__(x, n-1) * x
#
#		elif n % 2 == 0:
#			t = self.__call__(x, n // 2)
#			self.memo[x, n] = t * t
#
#		else:
#			assert False
#			raise Exception("Logic Error")
#
#		return self.memo[x,n]
#
#pow_x= Power5()
#""", number = 100000)
#print("iterative", iterative)


from functools import lru_cache

@lru_cache(None)
def pow6(x, n):
	if n == 0:
		return 1

	elif n % 2 == 1:
		return pow6(x, n-1) * x

	else:

		assert n % 2 == 0
		t = pow6(x, n // 2)
		return t * t



#iterative = timeit.timeit( "pow_x(2, 1024)", """
#from functools import lru_cache
#
#@lru_cache(None)
#def pow6(x, n):
#	if n == 0:
#		return 1
#
#	elif n % 2 == 1:
#		return pow6(x, n-1) * x
#
#	else:
#
#		assert n % 2 == 0
#		t = pow6(x, n // 2)
#		return t * t
#
#pow_x= pow6
#""", number = 1000000)
#print("iterative", iterative)



class BettingStrategy:
	def __init__(self):
		self.win = 0
		self.loss = 0

	def __call__(self):
		return 1



class BettingMartingale(BettingStrategy):
	def __init__(self):
		self._win = 0
		self._loss = 0
		self.stage = 1

	@property
	def win(self):
		return self._win

	@win.setter
	def win(self, value):
		self._win = value
		self.stage = 1

	@property
	def loss(self):
		return self._loss

	@loss.setter
	def loss(self, value):
		self._loss = value
		self.stage *= 2

	def __call__(self):
		return self.stage



class BettingMartingale2(BettingStrategy):
	def __init__(self):
		self.win = 0
		self.loss = 0
		self.stage = 1

	def __setattr__(self, name, value):

		if name == "win":
			self.stage = 1

		elif name == "loss":
			self.stage *= 2

		super().__setattr__(name, value)

	def __call__(self):
		return self.stage




import random
class KnownSqeuence:
	def __init__(self, seed = 0):
		self.seed = seed

	def __enter__(self):
		self.was = random.getstate()
		random.seed(self.seed, version = 1)
		return self

	def __exit__(self, exc_type, exc_vlaue, traceback):
		random.setstate(self.was)


import chp3

#from chp3 import *


class Deterministic_Deck:
	def __init__(self, *args, **kw):
		self.args = args
		self.kw   = kw

	def __enter__(self):
		self.was = random.getstate()

		random.seed(0, version = 1)
		return chp3.Deck(*self.args, **self.kw)
		#return Deck(*self.args, **self.kw)

	def __exit__(self, exc_type, exc_vlaue, traceback):
		random.setstate(self.was)



