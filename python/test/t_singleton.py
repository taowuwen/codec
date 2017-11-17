#!/usr/bin/env python3

class OneOnly:

	_singleton = None

	def __new__(cls, *args, **kwargs):
		if not cls._singleton:
			cls._singleton = super(OneOnly, cls).__new__(cls, *args, **kwargs)

		return cls._singleton



def main():
	print("hello, singleton")

	o1 = OneOnly()
	o2 = OneOnly()

	assert o1 == o2
	assert id(o1) == id(o2)

	print(o1, o2)
	print(id(o1), id(o2))

if __name__ == '__main__':
	main()

