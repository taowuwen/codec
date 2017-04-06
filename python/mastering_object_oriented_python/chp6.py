#!/usr/bin/env python3
# -*- coding: utf-8 -*-



import collections.abc
from chp3 import Hand6
from chp3 import Deck
from chp3 import Card
import math


class Hand(Hand6):
	def __contains__(self, rank):
		if isinstance(rank, Card):
			return any(rank in self.cards)

		return any(c.rank == rank for c in self.cards)



d = Deck(decks = 3)

h = Hand(*(d.pop() for i in range(5)))




class StatsList2(list):
	def __init__(self, *args, **kw):
		super().__init__(*args, **kw)
		self.sum0 = 0
		self.sum1 = 0
		self.sum2 = 0

		for x in self:
			self._new(x)

	def _new(self, value):
		self.sum0 += 1
		self.sum1 += value
		self.sum2 += value * value
		print("do add ", value)

	
	def _rmv(self, value):
		self.sum0 -= 1
		self.sum1 -= value
		self.sum2 -= value * value

		print("do remove :", value)

	def insert(self, idx, val):
		super().insert(idx, val)
		self._new(val)

	def pop(self, idx):
		val = super().pop(idx)
		self._rmv(val)
		return val


	@property
	def mean(self):
		if self.sum0 <= 0:
			return 0

		return self.sum1 / self.sum0

	@property
	def stdev(self):
		if self.sum0 <= 0:
			return 0

		return math.sqrt(self.sum0 * self.sum2 - self.sum1*self.sum1) / self.sum0


	def __setitem__(self, idx, val):

		if isinstance(idx, slice):

			start, stop, step = idx.indices(len(self))
			olds = [self[i] for i in range(start, stop, step)]

			super().__setitem__(idx, val)

			for v in olds:
				self._rmv(v)

			for v in val:
				self._new(v)

		else:
			if isinstance(idx, int):
				old = self[idx]
				super().__setitem__(idx, val)

				self._rmv(old)
				self._new(val)
			else:
				super().__setitem__(idx, val)

	def __delitem__(self, idx):
		if isinstance(idx, slice):

			start, stop, step = idx.indices(len(self))
			olds = [self[i] for i in range(start, stop, step)]

			super().__delitem__(idx)

			for v in olds:
				self._rmv(v)
		else:
			if isinstance(idx, int):
				self._rmv(self[idx])
				super().__delitem__(idx)
			else:
				super().__delitem__(idx)


class Explore(list):
	def __getitem__(self, idx):
		val = super().__getitem__(idx)

		try:
			print(idx, idx.indices(len(self)))
		except:
			pass

		return val




class StatsList3:
	def __init__(self):
		self._list = []
		self.sum0, self.sum1, self.sum2 = (0, 0, 0)

	def append(self, val):
		self._list.append(val)
		self.sum0 += 1
		self.sum1 += val
		self.sum2 += val * val


	def __getitem__(self, idx):
		return self._list.__getitem__(idx)

	def __len__(self):
		return self._list.__len__()

	def __iter__(self):
		return iter(self._list)

	@property
	def mean(self):
		if self.sum0 <= 0:
			return 0

		return self.sum1 / self.sum0

	@property
	def stdev(self):
		if self.sum0 <= 0:
			return 0

		return math.sqrt(self.sum0 * self.sum2 - self.sum1*self.sum1) / self.sum0





from collections import Counter


class StatsCounter(Counter):
	@property
	def mean(self):
		sum0 = sum(v for k, v in self.items())
		sum1 = sum(k * v for k, v in self.items())

		return sum1/sum0

	@property
	def stdev(self):
		sum0 = sum(v for k, v in self.items())
		sum1 = sum(k * v for k, v in self.items())
		sum2 = sum(k * k * v for k, v in self.items())

		return math.sqrt(sum0 * sum2 - sum1 * sum1) / sum0


	@property
	def median(self):

		mid = sum(self.values()) // 2
		low = 0

		for k, v in sorted(self.items()):
			if low <= mid < low + v:
				return k

			low += v

		assert False






class FindPrime(list):
	def __init__(self, num):
		self.num = num
		super().__init__([2, 3, 5, 7])

		self._build()


	def _isprime(self, num):
		for n in self:
			v1, v2 = divmod(num, n)

			if v2 == 0:
				return False

			if n >= v1:
				return True

#		if num % n == 0:
#			return False
#
#		if n >= num // n:
#			return True


	def _build(self):
		for n in range(self[-1] + 2, self.num + 1, 2):
			if self._isprime(n):
				self.append(n)



class TreeNode:
	pass

class Tree(collections.abc.MutableSet):
	def __init__(self, iterable = None):
		self.root = TreeNode(None)
		self.size = 0

		try:
			for item in iterable:
				self.add(item)
		except:
			pass

	def add (self, item):
		self.root.add(item)
		self.size += 1

	def discard(self, item):
		try:
			self.root.more.remove(item)
			self.size -= 1

		except KeyError:
			pass

	def __contains__(self, item):
		try:
			self.root.more.find(item)
			return True
		except KeyError:
			return False

	def __iter__(self):
		for item in iter(self.root.more):
			yield item

	def __len__(self):
		return self.size




import weakref
class TreeNode:
	def __init__(self, item, less=None, more=None, parent=None):
		self.item = item
		self.less = less
		self.more = more
		if parent:
			self.parent = parent

	@property
	def parent(self):
		return self.parent_ref()

	@parent.setter
	def parent(self, val):
		self.parent_ref = weakref.ref(val)


	def __repr__(self):
		return ("{__class__.__name__}(item!r),{less!r},{more!r})".
				format(__class__=self.__class__, **self.__dict__))

	def find(self, item):
		if self.item is None:
			if self.more:
				return self.more.find(item)

		elif self.item == item:
			return self

		elif self.item > item and self.less:
			return self.less.find(item)

		elif self.item < item and self.more:
			return self.more.find(item)

		raise KeyError

	def __iter__(self):
		if self.less:
			for item in iter(self.less):
				yield item

		yield self.item

		if self.more:
			for item in iter(self.more):
				yield item

	def add(self, item):
		if self.item is None or self.item < item:
			if self.more:
				self.more.add(item)
			else:
				self.more = TreeNode(item, parent = self)

		elif self.item >= item:
			if self.less:
				self.less.add(item)

			else:
				self.less = TreeNode(item, parent = self)

		else:
			assert False

	def remove(self, item):
		if self.item is None or item > self.item:
			if self.more:
				self.more.remove(item)
			else:
				raise KeyError

		elif item < self.item:
			if self.less:
				self.less.remove(item)
			else:
				raise KeyError

		else:
			assert item == self.item

			if self.less and self.more:
				successor = self.more._least()
				self.item = successor.item
				successor.remove(successor.item)

			elif self.less:
				self._replace(self.less)

			elif self.more:
				self._replace(self.more)

			else:
				self._replace(None)

	def _least(self):
		if self.less:
			return self.less._least()

		return self

	def _replace(self, new=None):
		if self.parent:
			if self == self.parent.less:
				self.parent.less = new
			else:
				self.parent.more = new

		if new is not None:
			new.parent = self.parent


#def __del__(self):
#	print("removing ", self.item)


