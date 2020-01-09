#!/usr/bin/env python3
# -*- coding:utf-8 -*-

import socket
import sys
import os

sys.path.insert(0, os.path.dirname(os.getcwd()))

from test_utils import timeit as _timeit


@_timeit
def _be_one_server(sock, do_send):

    data, addr = sock.recvfrom(4096)

    print('received data from {} data {!r}'.format(addr, data))

    if data and do_send:
        sent = sock.sendto(data, addr)
        print('sent out {}'.format(sent))


if __name__ == '__main__':

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    sock.bind(('localhost', 23232))

    while True:
        try:
            print('waitting to receive data...')
            _be_one_server(sock, 0)

        except socket.error as msg:
            print('error: {}'.format(msg))
            pass

        except KeyboardInterrupt:
            sock.close()
            sys.exit(1)
            break
