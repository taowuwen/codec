#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import ftplib, os, socket
import getpass


HOST = "localhost"
#ftp.mozilla.org"
#DIRN = "pub/mozilla.org/webtools"
FILE = "bugzilla-LATEST.tar.gz"

def main():

	try:
		f = ftplib.FTP(HOST)
	except Exception as e:
		print("ERROR" + str(e))
		return 

	try:
		user = getpass.getuser()
		username = input("username({}): ".format(user))
		if len(username) > 0:
			user = username

		passwd = getpass.getpass("Password: ")
		f.login(user, passwd)
	except ftplib.error_perm:
		print("ERROR: cannot login anonymously")
		f.quit()
		return 

	except Exception as e:
		print("ERROR: " + str(e))

	print("login as anonymous")

	try:
		f.dir()
	except Exception as e:
		print("ERROR: cwd" + str(e))
		f.quit()
		return 

	f.quit()


if __name__ == '__main__':
	main()

