#!/usr/bin/env python3
# -*- coding: utf-8 -*-

def test_iter():

	my_iter = iter("abcd")

	for i in my_iter:
		print(i)


class CRange:
	def __init__(self, start, stop = 0, step = -1):
		self._start = start 
		self._stop  = stop
		self._step  = step

		self._check_range()

	def _check_range(self):
		if self._start > self._stop and self._step >= 0 or \
			self._start < self._stop and self._step <= 0:
			self._start, self._stop , self._step = 0, 0, 0

	def __next__(self):

		self._check_range()

		if self._start == self._stop:
			raise StopIteration

		self._start += self._step

		return self._start


	def __iter__(self):
		return self



def my_gen():
	a, b = 0, 1

	while True:
		yield b
		a, b = b, a + b



def psycologist():

	while True:
		answer = (yield)

		if answer.endswith("?"):
			print("ask self too much???")
		elif "good" in answer:
			print("that's good, keep it")
		elif "bad" in answer:
			print("maybe you need a change")
		else:
			print("hmmmmmm!!!!. that's interesting...")
	
	

def main():
	test_iter()

	print([i for i in range(10)]);
	print([i for i in range(10, 0, -3)]);
	print([i for i in CRange(10)]);
	print([i for i in CRange(10, 0, -3)]);

	print("hello, world")

	gen = my_gen()

	print([ next(gen) for i in range(20) ])

	psy = psycologist()
	next(psy)
	print("tell me your problem")
	psy.send("problem??")
	psy.send("good, i am good")
	psy.send("bad, very bad")
	psy.send("nothing")



if __name__ == '__main__':
	main()

