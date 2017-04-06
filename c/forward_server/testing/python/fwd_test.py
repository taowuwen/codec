#!/usr/bin/env python3


import sys,argparse
from fwd_session import *


class CManager:

	def __init__(self, nsess = 1, bugtrace=0):
		self._sess   = {}
		self._n_sess = nsess

	

def argsparser():
	parser = argparse.ArgumentParser(description='this is arguments helper')

	parser.add_argument('-i', '--input', help='Input file name', required=True)
	parser.add_argument('-o', '--output', help='Output file name', required=True)
	parser.add_argument('-n', '--number', help='number of child', required=True)
	args = parser.parse_args()

	print("input file: %s output file: %s, number user: %s"%(args.input, args.output, args.number))

	


def main():
	argsparser()

	sess = CSession()
	sess.print()

	del sess



if __name__ == '__main__':
	main()
