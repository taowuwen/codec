#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import urllib
from urllib import request
from urllib.parse import urlencode
from bs4 import BeautifulSoup

_http = "http://"
_https = "https://"

class HTTPDownload:

    def _do_http_request(self, url, data = None, parser=1):
        assert url != None

        req = request.Request(url=url,
            data = urlencode(data).encode('utf-8') if data != None else None
        )

        req.add_header('User-Agent', 'Firefox 3.12')
        #req.add_header('Accept-Encoding','gzip, deflate, br')

        try:
            rsp = request.urlopen(req, timeout=30)
        except Exception as e:
            raise
        else:
            if parser:
                cmd = self.parse_post if req.get_method() == 'POST' else self.parse_get
                return cmd(self._get_rsp(rsp))
            else:
                return self._get_rsp(rsp)

    def _get_rsp(self, rsp):

        res = rsp.read()

        if rsp.info().get('Content-Encoding') == 'gzip':
            import gzip
            res = gzip.decompress(res)

        codec = ["utf-8", "gb2312", 'gbk']

        for c in codec:
            try:
                return res.decode(c)
            except UnicodeDecodeError:
                pass
        else:
            raise UnicodeDecodeError

        return res.decode()


    def http_get(self, url):
        return self._do_http_request(url)

    def http_post(self, url, data):
        return self._do_http_request(url, data)

    def parse_post(self, ctx):
        print("IN POST", ctx)

        return ctx

    def parse_get(self, ctx):
        print("IN GET", ctx)
        return ctx

class url_download(HTTPDownload):
    def __init__(self):
        self._ctx = None

    def _do_http_request(self, url, data = None):
        self._ctx = super()._do_http_request(url, data, parser=0)
        return self._ctx

    @property
    def content(self):
        return self._ctx


def _main():

    http = HTTPDownload()
    http.http_get("http://www.quanben5.com/n/jiuzhuanhunchunjue/27535.html")
#   http.http_post("http://localhost:8000/cgi-bin/hello.py", {"foo":"bar"})

#   url = url_download()
#
#   try:
#       url.http_get("http://www.quanben5.com/n/jiuzhuanhunchunjue/27535.html")
#   except Exception as e:
#       print(e)
#   else:
#       print(url.content)


if __name__ == '__main__':
    print("hello, test http download here")
    _main()

