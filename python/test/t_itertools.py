#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import itertools

def start_at_three():
	val = input("input some words or numbers: ").strip()

	while val != '':
		for el in itertools.islice(val.split(), 2, None):
			yield el

		val = input("input some words or numbers: ").strip()


def itertools_tee(iterable, headsize=1):
	a, b = itertools.tee(iterable)
	return list(itertools.islice(a, headsize)), b


def itertools_groupby(data = None):
	if data is None:
		return None

	return ((len(list(group)), name)
			for name, group in itertools.groupby(data))



def main():
	print("hello, itertools")

	s_at_3 = start_at_three()

	print(next(s_at_3))
	print(next(s_at_3))
	print(next(s_at_3))

	seq = [x for x in range(10)]

	print(str(itertools_tee(seq)))
	print(str(itertools_tee(seq, 3)))
	print(list(itertools_groupby("hello, world, taowuwen")))
	print("".join(size*name for size, name in itertools_groupby("hello, world, taowuwen")))



if __name__ == '__main__':
	main()
