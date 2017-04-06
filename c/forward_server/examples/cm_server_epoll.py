#!/usr/bin/env python


from cm_utils import *
from cm_msgs import *
from socket import *
from time import ctime
#import socket
import struct
import threading

import logging
import select
import errno
import sys



logger = logging.getLogger("cm-server")

# fd: {addr: aaa , ticketserver: 1/0}
connections = {}
recvlist    = {}
sendlist    = {}

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

			len,ver,tag,rsrv = struct.unpack(">hhhh", buf[0:8])

			print("len: {len}, ver: 0x{ver}, tag: 0x{tag:02x}, rsrv: {rsrv}".
					format(len = len, ver=ver, tag=tag, rsrv=rsrv))

			buf = ticket_handle_msg(buf)

		print("client: ", self._addr, "exit..")

		self._sock.close()


def bind_ticketserver():

	try:
		sock = socket(AF_INET, SOCK_STREAM)
		sock.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
		sock.bind(('', 10000 ))
		sock.listen(5)

	except :
		logger.error("socket error in ticket server")
		return None

	return sock


def bind_cm_server():
	try :
		sock = socket(AF_INET, SOCK_STREAM)
		sock.setsockopt(SOL_SOCKET, SO_REUSEADDR, 1)
		sock.bind(('', 10001))
		sock.listen(5)

	except:
		logger.error("socket error in cm server ")
		return None

	return sock

def create_epoll():
	try:
		epoll_fd = select.epoll()
	except (socket.error, msg):
		logger.error(msg)
		return None

	return epoll_fd


def init_log():
	logger.setLevel(logging.DEBUG)

	fh = logging.FileHandler("server.log")
	fh.setLevel(logging.DEBUG)

	ch = logging.StreamHandler()
	ch.setLevel(logging.ERROR)

	fmt = logging.Formatter("%(asctime)s - %(name)s -$(levelname)s - %(message)s")

	ch.setFormatter(fmt)
	fh.setFormatter(fmt)

	logger.addHandler(fh)
	logger.addHandler(ch)

def accept_fwd(ticket, efd):

	conn, addr = ticket.accept()

	logger.debug("accept fwd: %s:%d session: %d"%(addr[0], addr[1],
		conn.fileno()))

	conn.setblocking(False)
	efd.register(conn.fileno(), select.EPOLLIN | select.EPOLLET)

	fwd = {}
	fwd['addr']   = addr
	fwd['fwd' ]   = 1
	fwd['fd'  ]   = conn
	fwd['reg' ]   = 0
	fwd['est']    = 0
	fwd['ticket'] = ''
	fwd['fwd_addr'] = ''
	fwd['fwd_port'] = ''
	fwd['num']   = 0

	connections[conn.fileno()] = fwd

	return conn

def accept_client(cm, efd):

	conn, addr = cm.accept()

	logger.debug("accept cm: %s:%d session: %d"%(addr[0], addr[1], conn.fileno()))

	conn.setblocking(False)
	efd.register(conn.fileno(), select.EPOLLIN | select.EPOLLET)

	tmp = {}
	tmp['addr']   = addr
	tmp['fwd' ]   = 0
	tmp['fd'  ]   = conn
	tmp['reg' ]   = 0
	tmp['est' ]   = 0
	tmp['ticket'] = ''

	connections[conn.fileno()] = tmp

	return conn

def prepare_rsp(tag, st, value):
	buf = pack_rsp(tag, st, value)
	return buf


def client_reg(fd, value):
	connections[fd]['reg'] = 1
	return prepare_rsp(MSG_CLIENT_REGISTER, 0, None)

def choose_a_fwd():

	min_fd  = -1
	min_fwd = None

	for ind, fwd in enumerate(connections):
		user = connections[fwd]
		if user['fwd'] == 1:
			if min_fd <= 0:
				min_fd  = fwd
				min_fwd = user
			elif user['num'] < min_fwd['num']:
				min_fd  = fwd
				min_fwd = user
	return min_fd
	

# { "peer_addr": aaa, "peer_port": port}
def client_try_connect(fd, value, efd):

	try :
		peer_id = -1
		peer    = None

		user = decode_json(value)

		logger.debug("user: %s: %d want to connect to : %s: %d"%(
			connections[fd]['addr'][0],
			connections[fd]['addr'][1],
			user['peer_addr'],
			int(user['peer_port'])
		))


		for ind, key in enumerate(connections):
			clnt = connections[key]

			logger.debug("clnt info: %s: %d <===>>> users: %s: %d"%(
				clnt['addr'][0],
				clnt['addr'][1],
				user['peer_addr'],
				user['peer_port']
			))

			if clnt['fwd'] != 1:
				if (user['peer_addr'] == clnt['addr'][0]) and (int(user['peer_port']) == clnt['addr'][1]):
					peer = clnt
					peer_id = key
					break;


		if not peer:
			return prepare_rsp(MSG_CLIENT_CONN, 1, None)

		
		fwdid = choose_a_fwd()

		logger.debug("fwdid: %d, peer_id: %d, got peer : %s: %d"%(
				fwdid, 
				peer_id,
				peer['addr'][0],
				peer['addr'][1]
			))

		if fwdid <= 0:
			return prepare_rsp(MSG_CLIENT_CONN, 1, None)

		ticket = gen_ticket()

		logger.debug("[1]fwdid: %d, ticket: %s, peer_id: %d"%(fwdid, ticket, peer_id))

		sendlist[fwdid] =  send_fwd_ticket(fwdid, ticket)
		efd.modify(fwdid, select.EPOLLET|select.EPOLLOUT)

		logger.debug("[2]fwdid: %d, ticket: %s, peer_id: %d"%(fwdid, ticket, peer_id))

		addr = connections[fwdid]['fwd_addr']
		port = connections[fwdid]['fwd_port']

		logger.debug("[3]fwdid: %d, ticket: %s, peer_id: %d"%(fwdid, ticket, peer_id))
		sendlist[peer_id] =  send_user_ticket(peer_id, ticket, addr, port)
		efd.modify(peer_id, select.EPOLLET|select.EPOLLOUT)


		logger.debug("[4]fwdid: %d, ticket: %s, peer_id: %d"%(fwdid, ticket, peer_id))

		return send_user_ticket(fd, ticket, addr, port)
	except:
		return prepare_rsp(MSG_CLIENT_CONN, -1, None)

	return None
"""
{
	{"ip": xxx, "port": aaa}
} 
"""
def client_list(fd, value):
	print("get user lists")

	user = ''
	userlist = {}

	for index, clnt in enumerate(connections):
		user = connections[clnt]
		print("user addr: %s: %d, clnt: %d"%(user['addr'][0], user['addr'][1], clnt))
		if user['fwd'] == 0:
			tmp = {}
			tmp['addr'] = user['addr'][0]
			tmp['port'] = user['addr'][1]
			userlist[index] = tmp
			del tmp

	js = encode_json(userlist)

	print("userlist: %s"%(js))

	if not js:
		return prepare_rsp(MSG_CLIENT_LIST, 0, None); 

	return prepare_rsp(MSG_CLIENT_LIST, 0, js); 


def handle_client_msg(fd, tag, value, st, efd):

	if not valid_clnt_tag(tag):
		print("not valid client tag")
		return None

	if tag == MSG_CLIENT_REGISTER:
		return client_reg(fd, value)

	elif tag == MSG_CLIENT_CONN:
		return client_try_connect(fd, value, efd)

	elif tag == MSG_CLIENT_LIST:
		return client_list(fd, value)
	else:
		print("not valid cmd");

	return None


def prepare_fwd_ticket(ticket):

	tmp = {}
	tmp['ticket'] = ticket
	js = encode_json(tmp)

	print("fwd: ticket: ", js);

	return prepare_rsp(MSG_FWD_TICKET_NEW, 0, js)
	

def prepare_clen_conn(ticket, fwd_addr, fwd_port):
	tmp = {}
	tmp['ticket']   = ticket
	tmp['fwd_addr'] = fwd_addr
	tmp['fwd_port'] = fwd_port
	js = encode_json(tmp)

	print("clnt: ticket: ", js);

	return prepare_rsp(MSG_CLIENT_CONN, 0, js)
	

def send_user_ticket(fd, ticket, fwd_addr, fwd_port):

	connections[fd]['ticket'] = ticket
	logger.debug("fd: %d, ticket: %s, after change ticket: %s"%(fd, ticket, connections[fd]['ticket']))

	logger.debug("send [client] %s:%d with ticket: %s, (fwd: %s:%d)" %(
					connections[fd]['addr'][0],
					connections[fd]['addr'][1],
					ticket,
					fwd_addr,
					fwd_port
	))
	return prepare_clen_conn(ticket, fwd_addr, fwd_port)
	

def send_fwd_ticket(fd, ticket):

	logger.debug("fd: %d, ticket: %s"%(fd, ticket))

	connections[fd]['ticket'] = ticket
	logger.debug("fd: %d, ticket: %s, after change ticket: %s"%(fd, ticket, connections[fd]['ticket']))

	logger.debug("send %s:%d with ticket: %s" %(
					connections[fd]['addr'][0],
					connections[fd]['addr'][1],
					ticket
	))

	return prepare_fwd_ticket(ticket)

def fwd_ticket_fired(fd, tag, value):

	try :
		tmp = decode_json(value)
		logger.debug("ticket: %s fired"%(tmp['ticket']))
	except:
		pass
	finally:
		return prepare_rsp(MSG_FWD_TICKET_FIRED, 0, None)

def fwd_req_hb(fd, tag, value):

	try :
		tmp = decode_json(value)

		for index, key in enumerate(tmp):
			print(" %d => %s : %s"%(index, key, tmp[key]))
	except:
		pass
	finally:
		return prepare_rsp(MSG_FWD_HEART_BEAT, 0, None)


# {"fwd_addr": addr, "fwd_port": port}

def fwd_reg(fd, tag, value):

	try:
		fwd = decode_json(value)

		for index, key in enumerate(fwd):
			print(" %d => %s : %s"%(index, key, fwd[key]))

		connections[fd]['fwd_addr'] = fwd['fwd_addr']
		connections[fd]['fwd_port'] = fwd['fwd_port']
	except:
		return prepare_rsp(MSG_FWD_REGISTER, 1, None)
		
	return prepare_rsp(MSG_FWD_REGISTER, 0, None)


def fwd_rsp_ticket(fd, value, st):
	if st != 0:
		logger.debug("ticket create error, regen one")

		ticket = gen_ticket()
		return send_fwd_ticket(fd, ticket)
	else:
		connections[fd]['est'] = 1 # well, all we know it that the fwd server 
					   # had already have the ticket
		return None


def handle_fwd_msg(fd, tag, value, st):
	if not valid_fwd_tag(tag):
		return None

	if tag == MSG_FWD_REGISTER:
		return fwd_reg(fd, tag,  value)

	elif tag == MSG_FWD_TICKET_NEW:
		return fwd_rsp_ticket(fd, value, st)

	elif tag == MSG_FWD_TICKET_FIRED:
		return fwd_ticket_fired(fd, tag, value)

	elif tag == MSG_FWD_HEART_BEAT:
		return fwd_req_hb(fd, tag, value)

	return None



def handle_recved_datas(fd, efd):
	c_len  = 0
	value  = ''

	headsize = requestheadsize()
	if len(recvlist[fd]) < headsize:
		return True

	buf = recvlist[fd].encode()

	length, version, tag, rsrv = unpack_req_header(buf)
	logger.debug("length: 0x%x, version: 0x%x, tag: 0x%x, rsrv: 0x%x"%(length, version, tag, rsrv))

	if not valid_tag(tag):
		logger.error("invalid tag, from %d"%(fd))
		return False

	if len(buf) < length:
		return True

	if length > headsize:
		c_len, = unpack_content_len(buf)

		fmt = "@%ds"%(c_len)
		if c_len > 0:
			value, = unpack_content(fmt, buf)
			value = value.decode()

	logger.debug("tag: 0x%x, value: %s"%(tag, value))

	recvlist[fd] = recvlist[fd][len(buf[:length].decode()):]

	logger.debug("session : %d, left: %s"%(fd, recvlist[fd]))

	if connections[fd]['fwd'] == 0:
		buf = handle_client_msg(fd, tag, value, rsrv, efd)
	else:
		buf = handle_fwd_msg(fd, tag, value, rsrv)

	print("buf: %s"%(str(buf)))

	if not buf:
		return True

	sendlist[fd]  = buf

	return True
	


def recv_datas(fd, efd):
	datas = ''

	print("trying to recv datas from : %s:%d on %d"%(
						connections[fd]['addr'][0],
						connections[fd]['addr'][1],
						fd
	))

	try:
		while True:
			try:
				data = connections[fd]['fd'].recv(1024)

				if not data:
					efd.unregister(fd)
					connections[fd]['fd'].close()
					logger.debug("%s:%d closed"%(
						connections[fd]['addr'][0],
						connections[fd]['addr'][1]))

					del connections[fd];
					break;

				else:
					datas += data.decode()
					print_hex(datas.encode())

					# handle msgs goes from here

			except Exception as e:
				if e.errno == errno.EAGAIN:
					if fd not in recvlist:
						recvlist[fd] = datas

					elif len(recvlist[fd]) <= 0:
						recvlist[fd] = datas
					else:
						recvlist[fd] += datas

					if not handle_recved_datas(fd, efd):
						efd.unregister(fd)
						connections[fd]['fd'].close()
						logger.debug("close %s:%d"%(
							connections[fd]['addr'][0],
							connections[fd]['addr'][1]))
						del connections[fd];
						break;

					if fd not in sendlist:
						break;

					efd.modify(fd, select.EPOLLET|select.EPOLLOUT)
				else:

					print("=====>>>>>error: ", e)
					efd.unregister(fd)
					connections[fd]['fd'].close()
					del connections[fd];
					logger.error(e)

				break
	except Exception as e:
		print("error while recv: ", e)

	finally:
		return None
	return None




def socket_down(fd, efd):
	efd.unregister(fd)
	connections[fd]['fd'].close()
	logger.debug("%s:%d closed"%(connections[fd]['addr'][0],
					connections[fd]['addr'][1]))
	del connections[fd];

def send_datas(fd, efd):
	sendlen = 0

	try :
		while True:
			sendlen += connections[fd]['fd'].send(sendlist[fd][sendlen:])

			if sendlen == len(sendlist[fd]):
				sendlist.pop(fd)
				break
	except:
		pass
	finally:
		efd.modify(fd, select.EPOLLIN| select.EPOLLET)


def runpoll(ticket, cm, efd):
	efd.register(ticket.fileno(), select.EPOLLIN)
	efd.register(cm.fileno(), select.EPOLLIN)

	while True:
		epoll_list = efd.poll()

		for fd, events in epoll_list:
			if fd == ticket.fileno():
				accept_fwd(ticket, efd)
			elif fd == cm.fileno():
				accept_client(cm, efd)
			elif select.EPOLLIN & events:
				recv_datas(fd, efd)
			elif select.EPOLLHUP & events:
				socket_down(fd, efd)
			elif select.EPOLLOUT & events:
				send_datas(fd, efd)
			else:
				continue



def main():
	init_log()

	ticketSocket = bind_ticketserver()
	cmSocket     = bind_cm_server()
	efd          = create_epoll()

	if not ticketSocket or not cmSocket:
		if ticketSocket:
			ticketSocket.close()
		if cmSocket:
			cmSocket.close()

		if efd:
			efd.close()
		return None

	try:
		runpoll(ticketSocket, cmSocket, efd)
	except Exception as e:
		print("socket error", e)
	finally:
		ticketSocket.close()
		cmSocket.close()
		efd.close()

if __name__ == '__main__':
	main()
