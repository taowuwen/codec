#!/usr/bin/env python3
# -*- coding: utf-8 -*-

def finally_test_trap_one():
	print("hello, finally test ---> trap 1")

	while True:
		try:
			raise ValueError("this is value error")

		except NameError as e:
			print("NameErorr {}".format(e))
			break

		finally:
			print("Finally here, should never 'break' here")
			break


def finally_test_trap_two(arg=0):
	print("hello, finally test ---> trap 2")

	try:
		if arg <= 0:
			raise ValueError("Value Error here")
		else:
			return 1

	except ValueError as e:
		print(e)

	finally:
		print("The End -1, should never do 'RETURN' in finally")
		return -1


def main():
	print("hello, finally test")

	finally_test_trap_one()

	finally_test_trap_two(0)
	finally_test_trap_two(1)

if __name__ == '__main__':
	main()
