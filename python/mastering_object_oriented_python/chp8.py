#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import sys
import functools


import logging
logging.basicConfig(stream = sys.stderr, level = logging.DEBUG)

def debug(function):
	@functools.wraps(function)
	def logged_function(*args, **kw):
		logging.debug("%s( %r, %r)", function.__name__, args, kw)

		result = function(*args, **kw)

		logging.debug("%s = %r", function.__name__, result)

		return result

	return logged_function


def debug2(function):
	@functools.wraps(function)
	def logged_function(*args, **kw):

		log = logging.getLogger(function.__name__)
		log.debug("call (%r, %r)", args, kw)

		result = function(*args, **kw)

		log.debug("result --> %r", result)

		return result

	return logged_function


def debug_named(log_name):
	def create_debug_name(function):
		@functools.wraps(function)
		def wrapped(*args, **kw):
			log = logging.getLogger(log_name)

			log.debug("%s(%r, %r)", function.__name__, args, kw)

			result = function(*args, **kw)

			log.debug("%s = %r", function.__name__, result)

			return result
		return wrapped
	return create_debug_name


#@debug2
#@debug
@debug_named("debugname")
def ackermann(m, n):
	"""
	this is ackermann 's represention
	"""
	if m == 0:
		return n + 1

	elif m > 0 and n == 0:
		return ackermann(m - 1, 1)
	elif m > 0 and n > 0:
		return ackermann(m - 1, ackermann(m, n-1))
	else:
		return 0




def audit(method):
	@functools.wraps(method)
	def wrapper(self, *args, **kw):
		log = logging.getLogger("audit")

		before = repr(self)

		try:
			result = method(self, *args, **kw)
			after = repr(self)
		except Exception as e:
			log.exception("{0} before {1}\n after {2}".format(
				method.__qualname__, before, after))

			raise

		log.info("{0} before {1}\n after {2}".format(
				method.__qualname__, before, after))

		return result

	return wrapper



class Hand:
	def __init__(self, cards):
		self._cards = list(cards)

	@audit
	def __iadd__(self, card):
		self._cards.append(card)
		return self

	def __repr__(self):
		cards = ",".join(map(str, self._cards))
		return "{__class__.__name__} ({cards})".format(__class__=self.__class__, cards=cards)


class UglyClass1:
	def __init__(self):
		self.logger = logging.getLogger(self.__class__.__qualname__)
		self.logger.info("New Thing")

	def method(self, *args):
		self.logger.info("method %r", args)


class UglyClass2:
	logger = logging.getLogger("UglyClass2")

	def __init__(self):
		self.logger.info("New thing")
	
	def method(self, *args):
		self.logger.info("method %r", args)



def logged(class_):
	class_.logger = logging.getLogger(class_.__qualname__)
	return class_

@logged
class SomeClass:
	def __init__(self):
		self.logger.info("New thing")

	def method(self, *args):
		self.logger.info("method %r", args)

@logged
class SomeClass1:
	def __init__(self):
		self.logger.info("New thing")

	def method(self, *args):
		self.logger.info("method %r", args)

@logged
class SomeClass2:
	def __init__(self):
		self.logger.info("New thing")

	def method(self, *args):
		self.logger.info("method %r", args)


def memento(class_):
	def memento(self):
		return "{0.__class__.__qualname__} ({0!r})".format(self)

	class_.memento = memento

	return class_


@memento
class SomeClass3:
	def __init__(self, value):
		self.value = value

	def __repr__(self):
		return "{0.value}".format(self)


class SomeClass4(SomeClass3):
	def memento(self):
		return "hello"


class Memento:
	def memento(self):
		return "{0.__class__.__qualname__} ({0!r})".format(self)


class SomeClass5(Memento):
	def __init__(self, value):
		self.value = value

	def __repr__(self):
		return "{0.value}".format(self)


def html(function):
	@functools.wraps(function)
	def wrapper(*args, **kw):

		print("<title> hello, world </title>\n")
		return function(*args, **kw)

	return wrapper

def body(function):
	@functools.wraps(function)
	def wrapper(*args, **kw):

		print("<body>\n")
		result = function(*args, **kw)
		print("</body>\n")
		return result
	return wrapper

@html
@body
def print_context(*args):
	print(*args)



