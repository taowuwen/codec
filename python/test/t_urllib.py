#!/usr/bin/env python3
# -*- coding: utf-8 -*-


from urllib.parse import urlparse
from urllib.parse import urlsplit
from urllib.parse import urldefrag
from urllib.parse import urlunparse
from urllib.parse import urljoin
from urllib.parse import urlencode
from urllib.parse import parse_qs, parse_qsl
from urllib.parse import quote, quote_plus
from urllib.parse import unquote, unquote_plus
import sys

import urllib


from urllib import request

def test_request():

	url = "http://localhost:8000/"

	def test_urlopen(url=url, data=None):

		print("URL: {}".format(url))
		print("urlparse(url): {}".format(urlparse(url)))
		if not (url.startswith("http://") or url.startswith("https://")):
			print("Invalid URL: {}".format(url))
			return None
			
		rsp = request.urlopen(url, data)
		print(rsp)
		print("rsp.geturl() = {}".format(rsp.geturl()))

		headers = rsp.info()
		print(type(headers), headers)

		data = rsp.read().decode('utf-8')
		print("LENGTH = {length}".format(length=len(data)))
		print("---------")
		print(data)
		print(rsp.status)


	def test_urlencode_open(key = None):

		url = '''http://www.quanben5.com/index.php?'''

		args = {
			"c": "book",
			"a": "search",
			"keywords": key
		}

		test_urlopen(url + urlencode(args))

	def test_urlopen_post(url=url, data=None):
		try:
			test_urlopen(url, urlencode(data).encode('utf-8') if data != None else None)
		except urllib.error.HTTPError as e:
			print(e)
		

	def test_urlopen_post_v1(url=url, data=None):

		r = request.Request(
				url = url, 
				data = urlencode(data).encode('utf-8')
				)

		r.add_header( 'User-agent', 'PyMOTW (https://pymotw.com/)',)

		print("Out going data: {}".format(r.data))

		response = request.urlopen(r)
		print("SERVER response: \n", response.read().decode('utf-8'))

	

	test_urlopen()
#test_urlencode_open(u'最强')
	test_urlopen_post(url=url + "/cgi-bin/hello.py", data={'foo':'bar'})
	test_urlopen_post_v1(url=url + "/cgi-bin/hello.py", data={'foo':'bar'})




def test_urlencode():

	url = 'http://localhost:8080/~hellmann/'

	args = [
		{
			"method" : "callback",
			"args"   : 5,
			"keyword": u'中文字体',
			"query_args": ["abc", "edf"]
		},

		{
			"query_args": ["abc", "edf"]
		},

		{
			'url': url
		}
	]

	for arg in args:
	
		encoded = urlencode(arg, doseq=True)

		print("encoded args >>> '{}'".format(encoded))

		print(parse_qs(encoded))
		print(parse_qsl(encoded))

	print("quote(url) \t:  ", quote(url))
	print("quote_plus(url)\t:  ", quote_plus(url))

	print("unquote(url)\t: ", unquote(quote(url)))
	print("unquote(plused url)\t: ", unquote(quote_plus(url)))

	print("unquote_plus(url)\t: ", unquote_plus(quote(url)))
	print("unquote_plus(plused url)\t: ", unquote_plus(quote_plus(url)))

			
def test_urlparse():
	urls = (
		'http://netloc/path;param?query=arg#frag',
		'http://netloc/path/index?query=arg#frag',
		'http://netloc/path/index?query=arg&a=b',
		'http://www.quanben5.com/n/zuiqiangbaijiazi/12210.html',
		'http://user:pwd@NetLoc:80/path;param?query=arg#frag'
	)

	for method in (urlparse, urlsplit, urldefrag):
		print()
		print("#######################################")
		print("{} results:".format(method.__name__))

		for url in urls:
			parsed = urlparse(url)
			print(parsed)
			if 'username' in dir(parsed):
				print(parsed.username)

			new_url = urlunparse(parsed)
			print(new_url)
			assert new_url == url


def test_urljoin():
	print(urljoin("http://www.bing.com/a/b/c.html", "./another.html"))
	print(urljoin("http://www.bing.com/a/b/c.html", "../another.html"))
	print(urljoin("http://www.bing.com/a/b/c.html", "/another.html"))
	print(urljoin("http://www.bing.com/a/b/c.html", "another.html"))
	print(urljoin('http://user:pwd@NetLoc:80/path;param?query=arg#frag', "/path/to/my/page.html"))




def main():
	print("hello, urllib")
	print("system default coding: ".format(sys.getdefaultencoding()))

	test_urlparse()
	test_urljoin()
	test_urlencode()
	test_request()


if __name__ == '__main__':
	main()
