#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import threading
import selectors
from dbgmsg import DbgMessage

class DbgServerThread(threading.Thread):

    def __init__(self, mq_filter, host='0.0.0.0', port=23232, **kwargs):

        self.addr = (host, port)
        self.mq_filter = mq_filter
        self._selector = selectors.DefaultSelector()
        self._run = False
        self._stat = 0

        super().__init__()

    def be_server(self):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.bind(self.addr)
        self._sock.setblocking(False)
        self._run = True

        self._selector.register(self._sock, selectors.EVENT_READ, self._recv)

    def _recv(self, sock, mask):

        if not mask & selectors.EVENT_READ:
            return

        try:
            data, addr = sock.recvfrom(8192)
            self._stat += 1

            # we need to send tm + addr + msg
            # do it later 
            msg = DbgMessage(self._stat, addr, self.addr, data.decode())
            self.mq_filter.put(msg)

        except socket.error as msg:
            print("error recv {}, if needed, we neeed to restart server here".format(msg))
            self.mq_filter.put("DbgServerThread.socket.error")


    def run(self):

        print("server started")

        try:
            self.be_server()
        except socket.error as msg:
            print("Error: {}".format(msg))
            self.mq_filter.put("DbgServerThread.socket.error.be_server.failed")

        while self._run:
            try:
                for key, mask in self._selector.select(timeout = 1):
                    cb = key.data
                    cb(key.fileobj, mask)

            except KeyboardInterrupt:
                self._run = False;

        self._run = False
        self._selector.unregister(self._sock)
        self._sock.close()
        self._selector.close()

        print("server exited")

    def stop(self):
        print("stop server")
        self._run = False

