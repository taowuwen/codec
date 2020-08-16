#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import threading
import selectors
import queue
from dbgmsg import DbgMessage
from dbgactiondef import dbg_print

class DbgServerThread(threading.Thread):

    def __init__(self, mq_filter, host='0.0.0.0', port=23232, **kwargs):

        self.addr = (host, port)
        self.mq_filter = mq_filter
        self._selector = selectors.DefaultSelector()
        self._run = False
        self._recv_stat = 0
        self._send_stat = 0
        self.q_msg = []

        super().__init__()

    def be_server(self):
        self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self._sock.bind(self.addr)
        self._sock.setblocking(False)
        self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024 * 1024 * 2)
        self._run = True

        self._selector.register(self._sock, selectors.EVENT_READ, self._recv)

    def do_send_one(self, msg):
        try:
            self.mq_filter.put_nowait(msg)
            self._send_stat += 1
        except queue.Full as e:
            return 0
        else:
            return 1

    def send_msg(self, msg = None):
        if msg:
            '''
                sent by recved cb
            '''
            self.q_msg.append(msg)

#           if self.q_msg:
#               self.q_msg.append(msg)
#           else:
#               if not self.do_send_one(msg):
#                   self.q_msg.append(msg)
        else:
            '''
                sent by timeout event
            '''
            while self.q_msg:
                msg = self.q_msg[0]
                if self.do_send_one(msg):
                    self.q_msg.pop(0)
                else:
                    break

    def _recv(self, sock, mask):

        if not mask & selectors.EVENT_READ:
            return

        try:
            data, addr = sock.recvfrom(8192)
            self._recv_stat += 1

            msg = DbgMessage(self._recv_stat, addr, self.addr, data.decode())
            self.send_msg(msg)

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
                events = self._selector.select(timeout = 0.1)

                if not events:
                    self.send_msg()
                    continue

                for key, mask in events:
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


    def dbg_show(self):
        dbg_print(f'Server({self.addr}) running? {"yes" if self._run else "no"}, TX {self._send_stat}, RX: {self._recv_stat} ')

    def dbg_clear(self):
        self._send_stat = 0
        self._recv_stat = 0

