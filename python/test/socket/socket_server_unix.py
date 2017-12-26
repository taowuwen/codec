#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import sys
import os
import socket

sys.path.insert(0, os.path.dirname(os.getcwd()))

from test_utils import timeit as _timeit

addr = '/tmp/socket_unix.domain'


@_timeit
def _serv_client(sock):

    while True:
        data = sock.recv(16)
        if data:
            print('received data [ {} ] -> {!r}'.format(len(data), data))
            sock.sendall(data)

        else:
            break


if __name__ == '__main__':

    try:
        os.unlink(addr)
    except OSError as msg:
        if os.path.exists(addr):
            print('path error {}'.format(msg))
            sys.exit(1)

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.bind(addr)
    sock.listen(1)

    while True:
        try:
            print('Waitting for client connection...')
            sock_client, addr_client = sock.accept()
            print('accepted: {!r} {}'.format(sock_client, addr_client))

            _serv_client(sock_client)

        except socket.error as msg:
            print('Error {}'.format(msg))

        except KeyboardInterrupt:
            sock_client.close()
            sock.close()
            sys.exit(1)

        finally:
            sock_client.close()
