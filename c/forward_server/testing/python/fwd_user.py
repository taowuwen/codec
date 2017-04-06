#!/usr/bin/env python3


from fwd_status import *


class CSession:
	pass
	

class CUser():
	def __init__(self, sess):
		self._sock = None
		self._peer = None
		self._sess = sess
		self._buf  = ''

	def print(self):
		print("user: %d--> fd: %d, peer: %d, sess: %d"%(
				id(self), id(self._sock), id(self._peer), id(self._sess)))

	def setpeer(self, peer):
		self._peer = peer
		return peer

	def session(self):
		return self._sess

	def sock(self):
		return self._sock

