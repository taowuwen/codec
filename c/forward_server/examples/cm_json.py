#!/usr/bin/env python3


from cm_utils import *

def create_ips():
	tmpdict = dict()
	tmp = {}

	for n in range(10):
		ip   = "192.168.10.%d"%(n + 10)
		port = n + 10000

		tmp['addr'] = ip
		tmp['port'] = port

		tmpdict[n] = tmp

	js = encode_json(tmpdict)
	print(js)
	



def main():
	mydict = dict()

	mydict['foo'] = 'bar'
	mydict['key'] = 'value'

	js = encode_json(mydict)

	print(js, "type: ", type(js))

	b = str.encode(js)
	print_hex(b)
	print(b);

	s = b.decode()
	print("====>>>", s);

	mydict = decode_json(s)

	if not mydict or not isinstance(mydict, dict) :
		print("decode failed")
		return None

	for index, key in enumerate(mydict):
		print("--->index = {ind} , key = {key}, value = {val}".
			format(ind=index, key=key, val=mydict[key]))

	create_ips()
	


if __name__ == '__main__':
	main()
