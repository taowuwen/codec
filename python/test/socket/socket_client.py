#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import os
import sys

sys.path.insert(0, os.path.dirname(os.getcwd()))

from test_utils import timeit as _timeit
from test_utils import separate_func as _separate_function


@_timeit
def be_client(sock):

    msg = b'This is a message for testing.. It will be repeatted'

    sock.sendall(msg)

    amount_received = 0
    amount_expected = len(msg)

    while amount_received < amount_expected:
        data = sock.recv(16)
        amount_received += len(data)
        print('received {!r}'.format(data))


if __name__ == '__main__':
    print("hello, test socket echo server")

    addr = ('localhost', 10000)

    try:
#       sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
#       sock.connect(addr)
        sock = socket.create_connection(addr)

        be_client(sock)

    except socket.error as msg:
        print('socket Error: {}'.format(msg))

    finally:
        sock.close()
