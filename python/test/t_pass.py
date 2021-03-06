#!/usr/bin/env python3
# -*- coding: utf-8 -*-


def getpass(prompt="Password: "):
	import termios, sys
	fd = sys.stdin.fileno()
	old = termios.tcgetattr(fd)
	new = termios.tcgetattr(fd)
	new[3] = new[3] & ~termios.ECHO          # lflags
	try:
		termios.tcsetattr(fd, termios.TCSADRAIN, new)
		passwd = input(prompt)
	finally:
		termios.tcsetattr(fd, termios.TCSADRAIN, old)
	return passwd


def main():
	print("your input is: {}".format(getpass()))

if __name__ == '__main__':
	main()
