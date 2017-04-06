#!/usr/bin/env python3
# -*- coding: utf-8 -*-  

import os, sys
import shutil
import time


#reload(sys)
#sys.setdefaultencoding('utf8')
#sys.setdefaultencoding('utf-8')
#print(locale.getdefaultlocale())
#locale.setlocale(locale.LC_ALL, 'en_US')
#print(locale.getdefaultlocale())

#print sys.getdefaultencoding()

def file_md5(fl):
	import hashlib

	try:
		fd = open(fl, 'rb')
		m = hashlib.md5()

		while True:
			data = fd.read(8192)
			if not data:
				break
			m.update(data)

		fd.close()

		return m.hexdigest()
	except:
		print("open file", fl, "failed")
		return ""


# step 0, find boundary
# step 1, get boundary
# step 2, get content
# step 3, get tailer
# step 4, ender

def get_datas(bound="-----------", fp_in=sys.stdin, fl_out="/tmp/tmp"):
	step = 0
	lastline = None
	filename = None

	b_bound = bound.encode()
	
	if not b_bound:
		return None

	print("bound: [", b_bound.decode(), "]", "<br />")

	fp_out = open(fl_out, "wb")
	if not fp_out:
		print("open file %s failed" %(fl_out))
		return None

	for line in fp_in.buffer:

		if step == 0:
			if line.find(b_bound) == 0:
				step = 1
	
		elif step == 1:

			try:
				info = line.decode()
				if not filename:
					key = "filename="
					inx = info.find(key)
					if inx > 0:
						filename = info[inx + len(key):].strip().strip('"')
				print("===>>>", info , "<br />")

			except UnicodeDecodeError:
				pass

			if line == b"\r\n":
				step = 2	

		elif step == 2:

			if line.find(b_bound) == 0:
				if lastline != None:
					lastline = lastline[:lastline.rfind(b"\r\n")]
					fp_out.write(lastline)
					lastline = None
				step = 3
			else:
				if lastline != None:
					fp_out.write(lastline)

				lastline = line

		elif step == 3:

			try:
				print("===>>>", line.decode(), "<br />")

			except (UnicodeEncodeError,UnicodeDecodeError):
				pass

			if line.find(b_bound) == 0:
				step = 4

		elif step == 4:
			pass
			
	fp_out.close()

	return filename


def get_boundary():
	if not os.environ['CONTENT_TYPE']:
		return None

	return os.getenv("CONTENT_TYPE").split("boundary=")[1]

def print_header():
	print(
'''
<html lang="zh-cn"> 
<title> 上传完成 </title>
<head> 
	<meta charset="utf-8"> 
	<h1 align="center">版本信息</h1> 
</head> 
<body bgcolor="#aabbcc">
''')

def print_tailer():
	print(
"""
</body> </html>
""")


def get_fileinfo(fl):
	filename = fl.split('/')[-1]

	ary = filename.split('.')

	if len(ary) != 7 or ary[0] != "Yingke-WiFi-G1" or filename[-7:] != ".tar.gz":
		return (None, None, None)

	chk = file_md5(fl)
	if not chk:
		return (None, None, None)
	
	return (ary[0].replace("-"," "), filename[len(ary[0]) + 1:-7], chk)

def update_file_config(dev, ver, chk, fl):
	conf = "%s/conf/rom.conf"%(os.getenv("DOCUMENT_ROOT"))

	fp = open(conf, "wb")
	if not fp:
		print("open file %s failed" %(fl_out))
		return None
	ctx = "rom.cfgnum=%s\nrom.path=%s\nrom.checksum=%s\nrom.version=%s\nrom.dev=%s\n"%(
			 time.strftime("%H%M%S"),fl.split(os.getenv("DOCUMENT_ROOT"))[-1],chk, ver, dev)

	fp.write(ctx.encode("utf-8"))

	fp.close()


def print_info():

	print('''<p align="center"> <font color=#ff0000> >>>> 上传完成 <<<< </font> </p>''')
	print('''<p align="center"> <a href="/index.html"><font color="red">返回首页</font></a></p> <br />''')

	print(''' <p align="center"> <table align="center"> <tr> <td> ''')

	print("<table align=\"center\"> <p align=\"center\">")


	if os.getenv("REQUEST_METHOD") == "POST":
		print("<p>")
		out_fl = "/tmp/tmp.%d"%(os.getpid())

		fl = get_datas(bound="--" + get_boundary(), fp_in=sys.stdin, fl_out=out_fl)
		if fl != None:
			print("<p> filename: ", fl, "</p>")

			target = "%s/upload/%s"%(os.getenv("DOCUMENT_ROOT"), fl)
			shutil.move(out_fl, target)

			(dev, ver, chk) = get_fileinfo(target)
			print("<h4>", dev, ver, chk, " </h4>")

			if not dev or not ver or not chk:
				print("<h4> <font color=#ff0000> 错误的版本文件: ",fl, "</font></h4>")
				os.remove(target)
			else:
				update_file_config(dev, ver, chk, target)

				print("<h4>文件信息</h4>")
				print("<ul>")
				print("<li> 文件:", fl ,"</li>")
				print("<li> MD5: ", chk,"</li>")
				print("<li> 版本:", ver ,"</li>")
				print("<li> 大小:", os.path.getsize(target),"</li>")
				print("<li> 设备类型: ", dev ,"</li>")
				print("</ul>")
		print("</p>")

	print(''' </td></tr></table></p>''')

	print("<p>", os.getcwd() ,"</p>")





def main():
	print("Content-Type: text/html\r\n\r\n")

	print_header()

	print_info()

	print_tailer()



if __name__ == '__main__':
	main()

