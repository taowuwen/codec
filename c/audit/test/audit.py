#!/usr/bin/env python3

import struct
from socket import *
import json
from ctypes import create_string_buffer
import string

class c_audit:
	def __init__(self, addr='127.0.0.1', port=9110):
		self._addr = addr
		self._port = port
		self._sock = None

	def __del__(self):
		if self._sock != None:
			self._sock.close()
			self._sock = None


	def connect(self):
		self._sock = socket(AF_INET, SOCK_STREAM)
		if self._sock == None:
			return None

		try :
			self._sock.connect((self._addr, self._port))

		except ConnectionRefusedError:
			print("connect to %s:%d refused"%(self._addr, self._port))

		except:
			import sys
			print("connect failed", str(sys.exc_info()))

	''' TLV ii '''
	def encode_requst(self, evt, msg):
		val  = None
		l_val = 0
		total = 8

		if msg != None:
			if not isinstance(msg, dict):
				return None

			val = str.encode(json.dumps(msg, separators=(',', ':')))
			l_val = len(val)

			if len(val) % 4:
				l_val = l_val + 4 - len(val) % 4

			total = 8 + 4 + l_val

		buf = create_string_buffer(total)

		struct.pack_into(">ii", buf, 0, evt, total)

		if val != None:
			l = len(val)
			struct.pack_into(">i", buf, 8, l)
			struct.pack_into("@%ds%ds"%(l, l_val - l), buf, 12, val, b'\0\0\0\0')

		return buf.raw	

	def print_hex(self, msg):
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

	def decode_response(self, buf):
		if not isinstance(buf, bytes):
			return None

		v   = None
		l_v = 0
		if len(buf) < 12:
			return None

		t, l, st, = struct.unpack_from(">iii", buf, 0)

		print("recved tag 0x%x, length: %d, status: %d"%(t & 0xffffffff, l, st))

		if l > 12:
			if len(buf) < l:
				print("not enough bytes, continue reading")
				return None

			l_v, = struct.unpack_from(">i", buf, 12)

			if l_v > 0:
				v, = struct.unpack_from("@%ds"%(l_v), buf, 16)
				v = v.decode()

		return (t, l, st, l_v, v)


	def sendmsg(self, evt, msg=None):

		pkg = self.encode_requst(evt, msg)
		self.print_hex(pkg)

		try:
			self._sock.send(pkg)
		except Exception as e:
			print("socket error: ", str(e))
			return None

	def recvmsg(self):

		try:
			buf = self._sock.recv(4096)
			return self.decode_response(buf)
		except Exception as e:
			print("socket error: ", str(e))

		return None


	def desc(self):
		return "address: %s, port: %d"%(self._addr, self._port)



def e6wifi_ftp_test(audit):

	ftp = {}

	ftp['addr'] = "111.13.47.155"
	ftp['port'] = 5521
	ftp['username'] = "maipu1"
	ftp['password'] = "maipu&&77)"
	ftp['home'] = "/data1/firm/maipu_network"

	audit.sendmsg(0x0501, ftp)

	print(audit.recvmsg())


def e6wifi_citycode(audit):

	req = {}

	req['citycode'] = "101"

	audit.sendmsg(0x0502, req)
	print(audit.recvmsg())

def flush_all_user(audit):
	req = {}

	req['users'] = [
		{"username": "aaa", "usermac": "11:22:33:44:55:66"},
		{"username": "foo", "usermac": "aa:bb:cc:dd:ee:ff"},
		{"username": "bar", "usermac": "74:27:ea:c0:ba:c1"}
	]

	audit.sendmsg(0x0503, req)
	print(audit.recvmsg())


def setuser(audit):

	req = {}

	req["username"] = "FOO"
	req["usermac"]  = "74:27:ea:c0:ba:c1"

	audit.sendmsg(0x0504, req)
	print(audit.recvmsg())

def deluser(audit):
	req = {}

	req["usermac"]  = "11:22:33:44:55:66"

	audit.sendmsg(0x0505, req)
	print(audit.recvmsg())

def mod_ctrl(audit):
	req = {}
	req["net"] = {"enable": 1}

	audit.sendmsg(0x0506, req)
	print(audit.recvmsg())
	
def mod_info(audit):
	audit.sendmsg(0x0507)
	print(audit.recvmsg())


def mod_set(audit):
	req = {}
	req["net"] = {"enable": 0}

	audit.sendmsg(0x0508, req)
	print(audit.recvmsg())

def mod_get(audit):
	audit.sendmsg(0x0509)
	print(audit.recvmsg())


def main():
	req = {}

	audit = c_audit(addr="192.168.10.1")

	audit.connect()
	print(audit.desc())

#	e6wifi_ftp_test(audit)
#	e6wifi_citycode(audit)
#	mod_get(audit)
#	flush_all_user(audit)
#	mod_set(audit);
	mod_info(audit);
	mod_ctrl(audit);
#	setuser(audit)
#	deluser(audit)



if __name__ == '__main__':
	main()
	
