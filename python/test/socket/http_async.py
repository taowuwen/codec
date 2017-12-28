#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import selectors
import socket
import enum
import errno
import traceback
from urllib.parse import urlparse

from pprint import pprint


HttpStatus = enum.Enum(
    value="HttpStatus",
    names=("unknown init connecting request response done error")
)

HttpResponseStatus = enum.Enum(
    value = "HttpResponseStatus",
    names = ("statuscode headerinfo bodyinfo bodyinfo_lv done")
)

class InvalidHttpResponse(socket.error):
    pass


def errno_from_exception(e):

    if hasattr(e, 'errno'):
        return e.errno
    elif e.args:
        return e.args[0]
    else:
        return None


class HttpRequest:
    def __init__(self, url, data=None, header={}):
        self._header = {}
        self._data = data
        self._url  = url

        self._method = "GET" if data is None else "POST"

        parsed = urlparse(url)

        print(parsed)

        self._path = parsed.path
        if parsed.query:
            self._path += "?" + parsed.query
        self._version = 'HTTP/1.1'
        self._header['Host'] =  parsed.netloc
        self._header['Cache-Control'] = 'no-cache'
        self._header['Connection'] = 'close'
        self._header['Accept'] = 'test/html,application/xhtml+xml,*/*;q=0.8'

        if header:
            self._header.extend(header)

    def build_request(self):

        _ctx = b''
        _ctx += "{method} {path} {version}\r\n".format(method=self._method,
                path=self._path, version=self._version).encode('utf-8')

        self._header['Content-Length'] = 0
        if self._data:
            if not isinstance(self._data, bytes):
                self._data = self._data.encode('utf-8')
                self._header['Content-Length'] = len(self._data)

        for key in self._header:
            _ctx += "{}: {}\r\n".format(key, self._header[key]).encode('utf-8')

        _ctx += "\r\n".encode('utf-8')

        if self._data:
            _ctx += self._data

        self._ctx = _ctx
        return _ctx

    def add_header(self, key, value):
        self._header[key] = value

    def send(self, sock):

        assert self._ctx != None, "context could not be None"

        while self._ctx:
            _l = sock.send(self._ctx)
            self._ctx = self._ctx[_l:]

        return 0


class HttpResponse:
    def __init__(self):
        self._info = {}
        self._data = b''
        self._status_code = 0
        self._version = None
        self._st = HttpResponseStatus.statuscode
        self._ctx = b''
        self._content_length = 0

    def add_header(self, key, value):
        self._header[key] = value

    def get_next_content(self):
        sep = b'\r\n'

        ctx = self._ctx

        pos = ctx.find(sep)
        if pos == -1:
            raise InvalidHttpResponse('Http Content invalid')

        line = ctx[:pos]
        ctx = ctx[pos + len(sep):]

        try:
            _l = int(line, 16)

        except Exception as e:
            raise InvalidHttpResponse('http Content invalid')

        else:
            print("next content length {}".format(_l))

            if _l <= len(ctx):
                _d = ctx[:_l]
                self._ctx = ctx[_l + len(sep):]

                if _l == 0:
                    self._st = HttpResponseStatus.done
                    print('Http Received Finished....LV')
                return _d

        return None

    def handle_body(self):

        if self._st is HttpResponseStatus.bodyinfo:

            self._data += self._ctx
            self._ctx = b''

            if len(self._data) >= self._content_length:
                self._st = HttpResponseStatus.done
                print('Http Received Finished')
        else:
            assert self._st is HttpResponseStatus.bodyinfo_lv

            while len(self._ctx) >= 2:

                _d = self.get_next_content()

                if _d:
                    self._data += _d

                else:
                    break

                if self._st is HttpResponseStatus.done:
                    break


    def handle_rsp(self):

        sep = b'\r\n'
        if self._st in (HttpResponseStatus.bodyinfo,
                        HttpResponseStatus.bodyinfo_lv):

            return self.handle_body()


        while self._st != HttpResponseStatus.bodyinfo:

            pos = self._ctx.find(sep)
            line = self._ctx[:pos]
            self._ctx = self._ctx[pos + len(sep):]

            if pos == 0:

                assert self._st == HttpResponseStatus.headerinfo

                if self._st != HttpResponseStatus.headerinfo:
                    raise InvalidHttpResponse('Invalid Http Response Info')

                self._st = HttpResponseStatus.bodyinfo_lv

                if 'Content-Length' in self._info:
                    self._content_length = int(self._info['Content-Length'])
                    self._st = HttpResponseStatus.bodyinfo

                return self.handle_body()

            line = line.decode('utf-8')

            if self._st == HttpResponseStatus.statuscode:

                info = line.split()
                self._version = info[0].strip()
                self._status_code = int(info[1].strip())

                print('version: {}, status code: {}'.format(
                    self._version, self._status_code))

                if self._status_code != 200:
                    raise InvalidHttpResponse('We are support 200 only for now')

                self._st = HttpResponseStatus.headerinfo

            elif self._st == HttpResponseStatus.headerinfo:

                info = line.split(':')
                _k = info[0].strip()
                _v = info[1].strip()

                self._info[_k] = _v

    def recv(self, sock):

        while True:
            _d = sock.recv(1024)
            self._ctx += _d
            self.handle_rsp()

            if self._st is HttpResponseStatus.done:
                break


class HttpDownloader:

    def __init__(self, url, data=None, header=None):

        self._req = HttpRequest(url, data, header)
        self._rsp = HttpResponse()

        self._sel    = None
        self._st     = HttpStatus.unknown
        self._sock   = None
        self._mask   = 0
        self._addr   = None 

        self._cmd_entry = {
                HttpStatus.unknown: None,
                HttpStatus.init: self._init,
                HttpStatus.connecting: self._connect,
                HttpStatus.request: self._request,
                HttpStatus.response: self._response,
                HttpStatus.done: self._done,
                HttpStatus.error: self._error
        }


    def destroy(self):

        self._sel.close()
        self._sock.close()

    def run(self):

        try:
            self._change_status(HttpStatus.init)
            self.run_status()

        except socket.error as msg:

            e_n = errno_from_exception(msg)
            if e_n in (errno.EAGAIN, errno.EINPROGRESS, errno.EWOULDBLOCK):
                pass
            else:
                print('Error: {}'.format(msg))
                traceback.print_exc()
                return 1

        while self._st not in (HttpStatus.done, HttpStatus.error):

            # time out handle here maybe

            for key, mask in self._sel.select(timeout=1):
                self.run_status(key.fileobj, mask)

                if self._st in (HttpStatus.done, HttpStatus.error):
                    break

        print('Done...')

    def run_status(self, sock=None, mask=0):
        
        self._mask = mask

        cmd = self._cmd_entry.get(self._st, None)

        assert cmd != None, "cmd should never be null"
        cmd()
        
    def _change_status(self, new_st):

        if self._st != new_st:
            print('change status from {} to {}'.format(self._st.name, new_st.name))
            self._st = new_st

    def _init(self):
        self._sel  = selectors.DefaultSelector()
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.setblocking(False)

        if not self._addr:
            parsed = urlparse(self._req._url)

            for rsp in socket.getaddrinfo(parsed.netloc,
                                          parsed.scheme,
                                          type=socket.SOCK_STREAM,
                                          proto=socket.IPPROTO_TCP,
                                          flags=socket.AI_CANONNAME):
                family, socktype, proto, canoname, sockaddr = rsp

                self._addr = sockaddr
                break

            else:
                print('did not get url address, for {}'.format(self._url))
                self._change_status(HttpStatus.error)
                return self.run_status(self.sock, self._mask)

        print('address: {}'.format(self._addr))

        req = self._req.build_request()
        print('Request: \n{}'.format(req.decode('utf-8')))

        self._sel.register(self._sock, selectors.EVENT_WRITE|selectors.EVENT_READ)
        self._change_status(HttpStatus.connecting)
        self._sock.connect(self._addr)

    def _connect(self):

        if self._mask & selectors.EVENT_WRITE:
            self._sel.modify(self._sock, selectors.EVENT_WRITE)
            self._change_status(HttpStatus.request)
            return self.run_status(self._sock, self._mask)

    def _request(self):

        if self._mask & selectors.EVENT_WRITE:

            try:
                self._req.send(self._sock)

            except socket.error as e:
                print(e)

                self._change_status(HttpStatus.error)
                return self.run_status(self.sock, self._mask)

            else:
                self._sel.modify(self._sock, selectors.EVENT_READ)
                return self._change_status(HttpStatus.response)

    def _response(self):

        if self._mask & selectors.EVENT_READ:

            try:
                self._rsp.recv(self._sock)

            except socket.error as e:

                e_n = errno_from_exception(e)
                if e_n in (errno.EAGAIN, errno.EINPROGRESS, errno.EWOULDBLOCK):

                    if self._rsp._st == HttpResponseStatus.done:
                        self._change_status(HttpStatus.done)
                        return self.run_status(None, 0)
                else:
                    print('Error: {}'.format(e))
                    traceback.print_exc()
                    self._change_status(HttpStatus.error)
                    return self.run_status(None, 0)

            else:
                print('common finished...')
                self._change_status(HttpStatus.done)
                return self.run_status(None, 0)


    def _done(self):
        self._sel.unregister(self._sock)
        self.destroy()

    def _error(self):
        self._sel.unregister(self._sock)
        self._sock.close()

        print('if we have retry, we may add it here')

    @property
    def body(self):
        return self._rsp._data

    @property
    def info(self):
        return str(self._rsp._info)

if __name__ == '__main__':
    print('hello, test http download')

    url ='http://restapi.amap.com/v3/weather/weatherInfo?city=110101&key=2383d460b0c9def51819247bdaa18c41'

    http = HttpDownloader(url)

    http.run()
    print(http.info)
    body = http.body.decode()
    print(body)
    print(len(body))

    import json

    jobj = json.loads(body)
    print(jobj)
    print(jobj['status'])

#   del http
#
#   url='http://www.quanben5.com/n/wodemeinvzongcailaopo/66454.html'
#   http = HttpDownloader(url)
#   http.run()
#   print(http.info)
#   print(http.body.decode())
