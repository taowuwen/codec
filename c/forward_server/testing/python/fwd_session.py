#!/usr/bin/env python3

from fwd_user   import *
from fwd_status import *

class CSession():
	def __init__(self):
		self._user1 = CUser(self)
		self._user2 = CUser(self)

		self._st_m  = CMStatus.init
		self._st_c  = CCStatus.default

		self.userpeer().setpeer(self.userself())
		self.userself().setpeer(self.userpeer())

		self.userpeer().print()
		self.userself().print()

	def userpeer(self):
		return self.user2()
	
	def userself(self):
		return self.user1()

	def user1(self):
		return self._user1

	def user2(self):
		return self._user2

	def setuser1(self, user1):
		self._user1 = user1
		return self.user1()

	def setuser2(self, user2):
		self._user2 = user2
		return self.user2()

	def changestatus(self, st, st_c=CCStatus.default):
		self._st_m = st
		self._st_c = st_c

		return None

	def print(self):
		print("sess: %d, user1: %d, user2: %d, st_m: %s, st_c: %s"%(
			id(self), id(self.user1()), id(self.user2()), self._st_m, self._st_c))
		pass

