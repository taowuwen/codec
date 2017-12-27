#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import selectors
import sys


class MyServer:

    def __init__(self, addr=('localhost', 10000)):

        self._addr = addr
        self._sel  = selectors.DefaultSelector()
        self._run  = False

        print('selector {}'.format(self._sel.__class__.__name__))

    def run(self):
        try:
            self.be_server()
        except socket.error as msg:
            print('Error : {}'.format(msg))
            return

        while self._run:
            try:
                print('waitting for client...')
                for key, mask in self._sel.select(timeout=1):
                    callback = key.data
                    callback(key.fileobj, mask)

            except KeyboardInterrupt:
                self._run = False

        self._sel.unregister(self._sock)
        self._sock.close()
        self._sel.close()


    def be_server(self):

        self._sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self._sock.setblocking(False)
        self._sock.bind(self._addr)
        self._sock.listen(4)

        self._run = True

        self._sel.register(self._sock, selectors.EVENT_READ, self._accept)

    def _accept(self, sock, mask):
        
        _sock, _addr = sock.accept()
        print('got a connection from {!r}, {}'.format(_sock, _addr))

        _sock.setblocking(False)
        self._sel.register(_sock, selectors.EVENT_READ, self._read)

    def _read(self, sock, mask):

        try:
            _addr = sock.getpeername()
            print('read ({})'.format(_addr))

            data = sock.recv(10)
            if data:
                print('     received {!r}'.format(data))
                sock.sendall(data)

            else:
                print('     closed')
                self._sel.unregister(sock)
                sock.close()

        except socket.error as msg:
            print('socket error: {}'.format(msg))
            self._sel.unregister(sock)
            sock.close()


if __name__ == '__main__':
    print('hello, my test for i/o multiplexing')
    MyServer().run()



