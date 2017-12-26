#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import os
import sys

sys.path.insert(0, os.path.dirname(os.getcwd()))

from test_utils import timeit as _timeit

@_timeit
def _main():
    addr = ('localhost', 10000)
    msg  = b'this message need to send out to udp server, which gonna send ' +\
           b'it back to me' 
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    try:
        sent = sock.sendto(msg, addr)
        print('sent out {}'.format(sent))

        data, server = sock.recvfrom(4096)
        if data:
            print('received from server {}, data: {!r}'.format(server, data))

    except socket.error as msg:
        print('Error {}'.format(msg))

    finally:
        sock.close()


if __name__ == '__main__':
    _main()
