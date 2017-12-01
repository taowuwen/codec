#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import webbrowser

def main():
	print("hello, test for webbrowser")

	url = 'http://209.141.33.9/'

#webbrowser.open(url)
#webbrowser.open_new(url)
#	webbrowser.open_new_tab(url)


	# use firefox
	print("start with firefox")
	b = webbrowser.get('firefox')
	b.open(url)


if __name__ == '__main__':
	main()
