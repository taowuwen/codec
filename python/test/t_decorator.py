#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
from functools import wraps

def my_docorator(func):
	@wraps(func)
	def wrapper(*args, **kwargs):
		print("start my decorator {}".format(func.__name__))
		tm_start = time.time()
		res = func(*args, **kwargs)
		tm_stop  = time.time()
		print("total excution time is: {} {}".format(func.__name__, tm_stop - tm_start));
		return res;
	return wrapper

def the_other_decorator(func):
	@wraps(func)
	def wrapper(*args, **kwargs):
		print("START the other decorator for function: {}".format(func.__name__))
		res = func(*args, **kwargs)
		print("END the other decorator for function: {}".format(func.__name__))

		return res
	return wrapper


@my_docorator
def do_print(info=None):
	print(info is None and "list comprehention, world" or info)
	print("list comprehention, world" if info is None else info)
	num_list = [n for n in range(0, 10000)]


@my_docorator
def another_way():
	num_list = []
	for n in range(0, 10000):
		num_list.append(n)

@the_other_decorator
@my_docorator
def print_num(num):
	return num


request = None

def env_prepare(*pres):
	def decorator(func):
		@wraps(func)
		def wrapper(*args, **kwargs):
			global request
			if request is None:
				stop, *c = pres
				request = [n for n in range(stop)]

			return func(*args, **kwargs)
		return wrapper
	return decorator

@env_prepare(10, 11, 20)
def do_request():
	print(request)

def main():
	do_print();
	do_print("hello, my decorator");

	another_way()

	print(print_num(5))

	do_request()


if __name__ == '__main__':
	main()
