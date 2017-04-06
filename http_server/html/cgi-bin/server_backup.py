#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os, sys

#reload(sys)
#sys.setdefaultencoding('utf8')
#sys.setdefaultencoding('utf-8')

# step 0, find boundary
# step 1, get boundary
# step 2, get content
# step 3, get tailer
# step 4, ender

def get_datas(bound="-----------", fp_in=sys.stdin, fl_out="/tmp/tmp"):
#	info = ""
	curline = ""
	step = 0
	
	if not bound:
		return None

	fp_out = open(fl_out, "wb")
	if not fp_out:
		print("open file %s failed" %(fl_out))
		return None

	for lines in sys.stdin.buffer:
		try:
			curline = lines.decode()
		except UnicodeDecodeError:
			curline=None

		if step == 0:
			if lines.find(bound.encode()) == 0:
#			if curline and curline.find(bound) == 0:
				step = 1
#				info = ""
	
		elif step == 1:
			if not curline:
				return

			print("curline: ", curline)		
#			info += curline
			if curline == "\r\n":
				step = 2	

		elif step == 2:
			if curline and curline.find(bound) == 0:
				step = 3
			else:
				fp_out.write(lines)

		elif step == 3:
			if curline.find(bound + "--") == 0:
				step = 4
		elif step == 4:
			pass
			
	fp_out.close()


def get_boundary():
	if not os.environ['CONTENT_TYPE']:
		return None

	return os.getenv("CONTENT_TYPE").split("boundary=")[1]


def print_info():
	print("Content-Type: text/html\r\n\r\n")

	print("<html>\n<body>\n")
	print("<p>", str(os.environ) ,"</p>")
	print("<p>", str(sys.argv) ,"</p>")
#	print("<p>", get_boundary() , "</p>")

	get_datas()

#	if os.getenv("REQUEST_METHOD") == "POST":
#		get_datas(bound=get_boundary())

	print("</body>\n</html>")


	"""
	[ "${REQUEST_METHOD}" == "POST" ] && {
		content=`getcontent`
		echo "<p> POST METHOD: $content </p>"
	}

	echo "
	<h1> query_string: ${QUERY_STRING} </h1>
	<h4> `print_all_key_value $content` </h4>


	<h4> wanna get firstname:  `getargs firstname` </h4>
	<h4> wanna get lastname:  `getargs lastname` </h4>
	</body>
	</html>
	"""



def main():
	print_info()



if __name__ == '__main__':
	main()

