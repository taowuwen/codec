#!/usr/bin/env python

import socket

#GET / HTTP/1.1
#Host: www.speedtest.cn
#User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:45.0) Gecko/20100101 Firefox/45.0
#Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8
#Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3
#Accept-Encoding: gzip, deflate
#Connection: keep-alive
#Pragma: no-cache
#Cache-Control: no-cache


request = "GET / HTTP/1.1\r\n"
request += "Host: www.speedtest.cn\r\n"
request += "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:45.0) Gecko/20100101 Firefox/45.0\r\n"
request += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
request += "Accept-Language: zh-CN,zh;q=0.8,en-US;q=0.5,en;q=0.3\r\n"
request += "Accept-Encoding: gizp, deflate\r\n"
request += "Connection: keep-alive\r\n"
request += "Pragma: no-cache\r\n"
request += "Cache-Control: no-cache\r\n"
request += "\r\n"




print(request)

def send_msg(sock, msg):

	t = 0
	l = len(msg)

	while t < l:
		sent = sock.send(msg[t:])

		if sent == 0:
			raise "socket broken"

		t += sent



s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
	s.connect(("www.speedtest.cn", 80))

	send_msg(s, request.encode("utf-8"));

	msg = ""

	while True:
		chunk = s.recv(1024);

		if chunk == b'':
			raise "socket down"

		print(str(chunk));
#		msg = msg + chunk.decode("utf-8")



except Exception as e:
	print(e)

finally:
	s.close()


