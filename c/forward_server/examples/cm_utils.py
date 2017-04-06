#!/usr/bin/env python3

import json


def decode_json(str):

	tmpdict = []
	try:
		if not str:
			return None

		tmpdict = dict()
		tmpdict = json.loads(str)
	except ValueError:
		print("json error")
		return None

	finally:
		return tmpdict


def encode_json(tmpdict):

	if not isinstance(tmpdict, dict):
		return None

	js = json.dumps(tmpdict, indent=4, separators=(',', ':'))

	return js


def print_hex(msg):
	if not isinstance(msg, bytes):
		print("type need bytes, but input is: ", type(msg))
		return None

	a = 16
	ender = ''
	for b in msg:
		a = a -1

		if (a == 0):
			ender = '\n'
			a = 16
		elif a == 8:
			ender = '   '
		else:
			ender = ' '

		print("{:02x}".format(b), end =ender)

	print("")
