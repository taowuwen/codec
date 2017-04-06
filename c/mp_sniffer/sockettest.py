#!/usr/bin/env python3


msg_from_ac=0x1001
msg_from_apcfg=0x1002

msg_evt_to_wtp=0x1101
msg_evt_to_apcfg=0x1102

import socket, sys, os
import struct
import json


def send_msg(sock, tag, msg):

	data = None
	l    = 4
	if msg:
		l += len(msg)
		fmt = ">2H" + str(len(msg)) + "s"
		data = struct.pack(fmt, tag, l, msg)
	else:
		l += 0
		fmt = ">2H"
		data = struct.pack(fmt, tag, l)

	sock.sendall(data)



def send_request(sock, tag, req):
	msg = None
	if req:
		print("request:", req)
		msg = json.dumps(req).encode('utf-8')
	send_msg(sock, tag, msg)


def send_response(sock, tag, rsp, status):
	msg = None

	rsp['status'] = status
	if rsp:
		print("response:", rsp)
		msg = json.dumps(rsp).encode('utf-8')
		
	send_msg(sock, tag, msg)


# type (1: radio. 2. wlan, 3. local interface)

def send_start_msg(connection, inf="enp3s0"):
	req = {}

	req['type'] = 2
	req['wlanid'] = 1
	req['radioid'] = 1
	req['interface'] = inf

	server = {}
	server['type'] = 1
	server['data_compress'] = 0

	

#ftp = {}
#ftp['user'] = 'a'
#ftp['pass'] = 'a'
#ftp['host'] = '10.0.12.9'
#ftp['port'] = 21
#ftp['path'] = '/'
#	server['ftp'] = ftp

#	server['ftp_server'] = "ftp://192.168.100.5:21/"
#	server['ftp_server'] = "ftp://192.168.100.5:21"
#	server['ftp_server'] = "ftp://192.168.100.5/"
#	server['ftp_server'] = "ftp://192.168.100.5"
#	server['ftp_server'] = "ftp://a@192.168.100.5"
#	server['ftp_server'] = "http://a@192.168.100.5"
#	server['ftp_server'] = "ftp://a:aa@192.168.100.5"
#	server['ftp_server'] = "ftp://a:aa@192.168.100.5/"
	server['ftp_server'] = "ftp://a:aa@192.168.100.5:21/"

	req['server'] = server

	req['limit_filesize'] = 1024 * 1024 * 100
	req['limit_time']     = 0
	req['limit_package']  = 0
#	req['filter'] = 'tcp port 80'

	req['method'] = 'capture_start'

	send_request(connection, msg_from_ac, req)


def handle_event(connection):
	buf=b''
	data=b''
	while True:
		buf = connection.recv(1024)
		if not buf:
			raise

		data = data + buf

		while len(data) >= 4:
			tag, l = struct.unpack('>2H', data[0:4])

			if l > len(data):
				break

			buf = data[4:l]

			data = data[l:]

			l -= 4
			value, = struct.unpack(">%ds"%(l), buf)
			value = value.decode('utf-8')

			print("tag: ", hex(tag), " len: ", l, " value: ", value)

#handle_msgs_evts(connection, tag, json.loads(value))



def handle_msg_from_apcfg(connection, req):

	print("request", req)

	method = req.get('method')

	if not method:
		print("got not method", req)
		return False
	
	if method == "register_sniffer":
		rsp = {"method": method }
		send_response(connection, msg_from_apcfg, rsp, 0)

		# ok, now, send start
		# send stop 

		print("send request for start")
		send_start_msg(connection, "wlp3s0")

		handle_event(connection)

		return True
	else:
		print("unknown method: %s, not handle for now"%(method))
		return False




def handle_msgs(connection, tag, request):
	try:
		if tag == msg_evt_to_wtp:
			print("not, handle for now")
		elif tag == msg_evt_to_apcfg:
			return handle_msg_from_apcfg(connection, request)
		else:
			print("not handle for now")
	except Exception as err:
	  	print("error: ", str(err))
	finally:
		return True



def handle_client(connection, client_address):
	print("connection from ", client_address)

	try:
		while True:
			data = connection.recv(1024)
			if not data:
				raise

			tag, l = struct.unpack('>2H', data[0:4])

			l -= 4
			fmt=">" + str(l) + "s"
			value, = struct.unpack(fmt, data[4:])
			value = value.decode('utf-8')

			print("tag: ", hex(tag), " len: ", l, " value: ", value)

			handle_msgs(connection, tag, json.loads(value))

	except:
		pass
	finally:
		print("lost client", client_address, "[", connection,"]")

def main():
	server_address = "/tmp/sniffer_apcfg_sock"

	try:
		os.unlink(server_address)
	except OSError:
		if os.path.exists(server_address):
			print("starting up on %s failed"%(server_address))
			raise


	sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

	print("starting up on %s"%(server_address))
	sock.bind(server_address)

	sock.listen(1)

	while True:
		print("waitting for a connection");

		connection, client_address = sock.accept()
		try:
			handle_client(connection, client_address)
		finally:
			pass

		connection.close()

	sock.close()

if __name__ == '__main__':
	main()
