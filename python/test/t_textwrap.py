#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import textwrap



def load_strings(fl=None):
	res = []

	with open(fl, "r") as f:
		res = [ line for line in f]

	return "".join(res)



def main():
	print("hello, textwrap")
	mystr='''hello, this is a test for this,
	Reformat the single paragraph in 'text' to fit in lines of no more
	than 'width' columns, and return a new string containing the entire
	wrapped paragraph.  As with wrap(), tabs are expanded and other
	whitespace characters converted to space.  See TextWrapper class for
	available keyword args to customize wrapping behaviour.'''

	print(mystr)

	print(textwrap.fill(mystr, 50));
	print(textwrap.dedent(mystr));
	

	mytext = load_strings("/tmp/tmp.txt")
	print("===================================================")

	print(textwrap.fill(mytext, 50));
	print(textwrap.dedent(mytext));
	print(textwrap.indent(mytext, ">"));

	for w in [40, 60]:
		print("wraptext width: {}".format(w))
		print(textwrap.fill(textwrap.dedent(mystr), w));
		print("")


	print(textwrap.fill(textwrap.shorten(mytext, 100), width=50));



if __name__ == '__main__':
	main()
