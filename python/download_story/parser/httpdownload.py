#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import urllib
from urllib import request
from urllib.parse import urlencode


class HTTPDownload:

    def _do_http_request(self, url, data = None):
        assert url != None

        req = request.Request(url=url, 
            data = urlencode(data).encode('utf-8') if data != None else None
        )

        req.add_header('User-Agent', 'Stories Downloader(By TWW)')

        try:
            rsp = request.urlopen(req)
        except Exception as e:
            print(e)
        else:
            cmd = self.parse_post if req.get_method() == 'POST' else self.parse_get
            return cmd(rsp.read().decode('utf-8'))

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


def _main():

    http = HTTPDownload()
    http.http_get("http://www.quanben5.com/n/jiuzhuanhunchunjue/27535.html")
    http.http_post("http://localhost:8000/cgi-bin/hello.py", {"foo":"bar"})


if __name__ == '__main__':
    print("hello, test http download here")
    _main()

