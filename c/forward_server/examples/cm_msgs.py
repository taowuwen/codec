#!/usr/bin/env python3


from cm_utils import *
import struct
from ctypes import create_string_buffer
import string
import random



MSG_FWD_REGISTER       = 0x1501
MSG_FWD_TICKET_NEW     = 0x1503
MSG_FWD_TICKET_FIRED   = 0x1505
MSG_FWD_HEART_BEAT     = 0x1506

MSG_CLIENT_REGISTER    = 0x1600
MSG_CLIENT_CONN        = 0x1601
MSG_CLIENT_LIST        = 0x1602


MSG_CLIENT_FWD_CONN    = 0x1504

clnt_tags=(
	MSG_CLIENT_REGISTER,
	MSG_CLIENT_CONN,
	MSG_CLIENT_LIST
)

fwd_tags=( 
	MSG_FWD_REGISTER, 
	MSG_FWD_TICKET_NEW, 
	MSG_FWD_TICKET_FIRED,
	MSG_FWD_HEART_BEAT
	)


def valid_clnt_tag(tag):

	try :
		if tag not in clnt_tags:
			return False
	except:
		return False

	return True

def valid_fwd_tag(tag):
	try:
		if tag not in fwd_tags:
			return False
	except:
		return False

	return True

def valid_tag(tag):
	if not valid_clnt_tag(tag) and not valid_fwd_tag(tag):
		return False
	return True


def unpack_req_header(buf):
	return struct.unpack(">hhhh", buf[:8])

def unpack_content_len(buf):
	return struct.unpack_from(">l", buf, 8)

def unpack_content(fmt, buf):
	return struct.unpack_from(fmt, buf, 12)

def align_length(value):
	if not value:
		return 0

	left = len(value)%4
	if left > 0:
		return len(value) + 4 - left

	return len(value)



def pack_rsp(tag, st, value):

	if value is not None:
		value = value.encode()

	headsize = requestheadsize()
	valuelen = align_length(value)

	totallen = headsize

	if valuelen > 0:
		totallen += valuelen + 4

	buf = create_string_buffer(totallen)

	struct.pack_into(">hhhh", buf, 0, totallen, 1, tag, st)


	if valuelen > 0 :
		l_val = len(value)
		print("totallen: %d, l_val : %d"%(totallen, l_val))

		struct.pack_into(">l", buf, headsize, l_val)

		fmt = "@%ds%ds"%(l_val, valuelen - l_val)
		print("totallen: %d, l_val : %d, fmt: %s"%(totallen, l_val, fmt))
		struct.pack_into(fmt, buf, headsize + 4, value, b'\0\0\0\0')

	print_hex(buf.raw)
	return buf.raw

#	print(buf.raw)
#	print("after decode")
#	ret = buf.raw.decode();
#	print(ret)

#	return ret

def gen_random(size=32, chars=string.ascii_uppercase + string.ascii_lowercase + string.digits):
	return ''.join(random.choice(chars) for x in range(size))


def gen_ticket():
	return gen_random(32)


def requestheadsize():
	tmp = struct.pack(">hhhh", 0, 0, 0, 0)
	return len(tmp)

