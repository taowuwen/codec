#!/usr/bin/env python


from socket import *
from time import ctime
import struct
import threading


def print_hex(msg):
	a = 16
	ender = ''
	for b in msg:
		a = a -1

		if (a == 0):
			ender = '\n'
			a = 16
		else:
			ender = ' '

		print("{:02x}".format(b), end =ender)



def ticket_handle_msg(msg):
	buflen = 0
	content = ""

	totallen,ver,tag,rsrv = struct.unpack(">hhhh", msg[:8])

	if (totallen > 8):
		buflen, = struct.unpack_from(">l", msg, 8)

		fmt = "@%ds"%(buflen)

		if (buflen > 0) :
			content, = struct.unpack_from(fmt, msg, 12)

	print("buflen: ", buflen, "content: ", content)

	return msg
	


class TicketServer(threading.Thread):

	def __init__(self, sock, addr):
		threading.Thread.__init__(self)

		self._sock = sock
		self._addr = addr

	
	def run(self):
		while True:
			print("Recving from : ", self._addr)

			buf = self._sock.recv(1024)

			if not buf:
				break

			print_hex(buf)

			print("got msg from: ", self._addr)

			header=buf[0:8]

			len,ver,tag,rsrv = struct.unpack(">hhhh", header)

			print("len: {len}, ver: 0x{ver}, tag: 0x{tag:02x}, rsrv: {rsrv}".
					format(len = len, ver=ver, tag=tag, rsrv=rsrv))

			buf = ticket_handle_msg(buf)

		print("client: ", self._addr, "exit..")

		self._sock.close()






def main():
	addr=('localhost', 10000)

	tcpSock = socket(AF_INET, SOCK_STREAM)

	tcpSock.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
	tcpSock.bind(addr)
	tcpSock.listen(5)

	while True:
		print("Waitting for connecting...")

		tcpClientSock, clientAddr = tcpSock.accept()

		ts = TicketServer(tcpClientSock, clientAddr)
		ts.start()

	tcpSock.close()

	print("hello, world")

	del tcpSock



if __name__ == '__main__':
	main()
