#!/usr/bin/env python


from socket import *
from time import ctime
import struct
import threading

from cm_utils import *
from cm_msgs  import *


def connect_server(addr):
	try:
		sock = socket(AF_INET, SOCK_STREAM)
		sock.connect(addr)
	except:
		print("connect failed to: ", addr)
		return None

	return sock


def connect_cm_server():
	return connect_server(('localhost', 10001))


def showUsage():
	tips = """
cmd list: 
	ls  (list all users on cm server)
	conn addr port (connect to peer user)
	reg (reg self)
	bepeer (for easy and quik)
	help show this doc
	"""

	print(tips.lstrip())


def send_and_recv(fd, tag, st, value):
	try :
		buf = pack_rsp(tag, st, value)

		#print_hex(buf)

		fd.send(buf)

		buf = fd.recv(2048)

		if not buf:
			return None

		#print("recved: ")
		#print_hex(buf)
		return buf

	except Exception as e:
		print ("socket error: ", e)
		return None

	return None



def send_cm_msg(fd, tag, st, value):
	return send_and_recv(fd, tag, st, value)



# {ticket = aaa }
def connect_with_peer(fwd, ticket, peer):

	tmp = {}
	tmp['ticket'] = ticket

	js = encode_json(tmp)

	print("send fwd ticket: %s"%(js))

	buf = send_and_recv(fwd, MSG_CLIENT_FWD_CONN, 0, js)

	if not buf:
		return None
	if not peer:
		print("ok, now, me and peer are connected, send file to peer")

		for i in range(50):
			with open("/tmp/code.avi", "rb") as f:
				while True:
					data = f.read(4096)

					if data:
						fwd.send(data)
					else:
						break;

			print("send and recved finished: %d times"%(i + 1))

#		while True:
#			data = input("input===> ")
#
#			buf = send_and_recv(fwd, MSG_CLIENT_CONN, 0, data)
#
#			if not buf:
#				print("disconnected with peer")
#				return None
#			
#			print("got peer's msg back:")
#			print_hex(buf)
	else:

		try:
			totallen = 0

			while True:
				data = fwd.recv(4096)

				if data:
					totallen += len(data)
				else:
					break;
		except:
			pass

		finally:
			print("recv finished, total: ", 
					totallen, "Bytes",
					totallen/1024,"k",
					totallen/1024/1024, "M",
					totallen/1024/1024/1024, "G")


#		with open("/tmp/new_vc.iso", "wb") as wr:
#			while True:
#				data = fwd.recv(4096)
#
#				if data:
#					wr.write(data)
#
#				else:
#					break
#
#		while True:
#			buf = fwd.recv(1024)
#
#			if not buf:
##				print("disconnected with peer")
#				return None
#
#			print("got peer's msg: ")
#			print_hex(buf)
#
#			fwd.send(buf)



	return None


def connect_with_fwd_server(ticket, fwd_addr, fwd_port, peer):

	try :
		fwdSocket = connect_server((fwd_addr, fwd_port))

		if not fwdSocket:
			return False

		print("ok, now we are connect with the FWD server, and try to connect to peer")
		connect_with_peer(fwdSocket, ticket, peer);

	except:
		print("exception here with peer");
	finally:
		if fwdSocket:
			fwdSocket.close()

	return None

	


def handle_conn_back(buf, peer=0):
	
	c_len = 0
	value = ''
	headsize = requestheadsize()

	length, ver, tag, st = unpack_req_header(buf)

	if st != 0 or length <= headsize:
		return False

	c_len, = unpack_content_len(buf)

	if c_len <= 0:
		return False
	else:
		fmt = "@%ds"%(c_len)
		value, = unpack_content(fmt, buf)
		value  = value.decode()

		print("got a fwd server info: %s:%d"%(value, c_len))

	tmp = decode_json(value)

	print(tmp)

	if not tmp:
		return False

	ticket   = tmp['ticket']
	fwd_addr = tmp['fwd_addr']
	fwd_port = tmp['fwd_port']

	print("ticket===> %s, fwd_addr: %s, port: %d"%(ticket, fwd_addr, fwd_port))

	connect_with_fwd_server(ticket, fwd_addr, int(fwd_port), peer)

	return True


def connect_to_peer(fd, addr, port):
	try :
		tmp = {}
		tmp['peer_addr'] = addr
		tmp['peer_port'] = int(port)

		js = encode_json(tmp)

		buf = send_cm_msg(fd, MSG_CLIENT_CONN, 0, js)

		if not buf:
			return False

		handle_conn_back(buf)
	except:
		return True

	finally:
		return True


def change_self_to_peer(fd):

	try :
		while True:
			buf = fd.recv(1024)

			if not buf:
				print("cm server closed")
				return None

			handle_conn_back(buf, 1)

	except:
		return None

	finally:
		return None



def peer_list(buf):
	
	c_len = 0
	value = ''
	headsize = requestheadsize()

#	buf = buf.encode()
	length, ver, tag, st = unpack_req_header(buf)

	if st != 0 or length <= headsize:
		return False

	c_len, = unpack_content_len(buf)

	if c_len <= 0:
		return True
	else:
		fmt = "@%ds"%(c_len)

		value, = unpack_content(fmt, buf)
		value  = value.decode()

	tmp = decode_json(value)
	if not tmp:
		return False

	for index, key in enumerate(tmp):
		print("--->index = {ind} , key = {key}, value = {val}".
			format(ind=index, key=key, val=tmp[key]))
	


def handle_with_cm():
	
	cmSocket = connect_cm_server()
	if not cmSocket:
		return None

	showUsage()

	try :
		while True:
			data = input("input cmd> ")

			args = data.split()

			if args[0] == 'ls':
				buf = send_cm_msg(cmSocket, MSG_CLIENT_LIST, 0, None)

				if not buf:
					break;

				peer_list(buf)

			elif args[0] == "reg":
				if not send_cm_msg(cmSocket, MSG_CLIENT_REGISTER, 0, None):
					break

			elif args[0] == "bepeer":
				change_self_to_peer(cmSocket)

			elif args[0] == "conn":
				if len(args) < 3:
					showUsage()
				else:
					if not connect_to_peer(cmSocket, args[1], args[2]):
						break
			else:
				showUsage()


	except Exception as e:
		print("got exception:", e)
	finally:
		cmSocket.close();

	return None



def main():
	handle_with_cm()

if __name__ == '__main__':
	main()
