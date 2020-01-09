#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import atexit
import queue
import threading


class CDebugViewPrint:

    instance = None

    def __new__(cls, *kargs, **kwargs):

        if not cls.instance:
            cls.instance = super().__new__(cls, *kargs, **kwargs)

        return cls.instance

    def __init__(self):
        self.args =  {
            'host': 'localhost',
            'port': 23232,
            'enable_thread': 0,
            'buffer_size': 10,
            'buffer': None,
            'thread_running': 0,
            'thread': None,
            'timeout': 1,
        }

    def __call__(self, ln):

        if not isinstance(ln, str):
            ln = f'{ln}'

        if not self.args.get('sock', None):
            self.do_init_session()

        if not self.args.get('enable_thread'):
            self.send_string(ln)
        else:
            if not self.args.get('buffer'):
                self.args['buffer'] = queue.Queue(self.args['buffer_size'])

            self.args.get('buffer').put(ln)

            if not self.args.get('thread_running', 0):
                self.do_init_thread()

    def do_init_session(self):
        self.args['sock'] = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    def send_string(self, msg):

        try:
            addr = (self.args.get('host'), self.args.get('port'))
            ret  = self.args['sock'].sendto(msg.encode(), addr)
        except socket.error as msg:
            print("socket ERROR {}".format(msg))
            self.do_exit()
            return -1
        else:
            return ret

    def config(self, **kwargs):
        self.args.update(**kwargs)

    def do_exit(self):
        sock = self.args.get('sock')
        if sock:
            sock.close()
            self.args.pop('sock')
            self.args['thread_running'] = 0


    def sender_thread(self):

        msgqueue = self.args['buffer']
        # msg = msgqueue.get_nowait()

        while self.args.get('thread_running'):
            try:
                msg = msgqueue.get(timeout=self.args.get('timeout'))
            except queue.Empty:
                break

            if self.send_string(msg) == -1:
                break

        self.args['thread_running'] = 0


    def do_init_thread(self):

        if self.args['thread']:
            self.args['thread'].join()

        self.args['thread'] = threading.Thread(target=self.sender_thread, args=())
        self.args['thread_running'] = 1
        self.args['thread'].start()


DebugStringPrint = CDebugViewPrint()
DebugStringConfig = DebugStringPrint.config
atexit.register(DebugStringPrint.do_exit)

__all__ = [ 'DebugStringPrint', 'DebugStringConfig' ]

