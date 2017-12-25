#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import os
import sys

sys.path.insert(0, os.path.dirname(os.getcwd()))

from test_utils import timeit as _timeit
from test_utils import separate_func as _separate_function


@_separate_function
def be_server(sock):

    while True:
        print('Waitting for a connection...')

        client_sock, client_address = sock.accept()

        try:
            print('connection from {} {}'.format(client_sock, client_address))

            while True:
                data = client_sock.recv(16)
                print('Received {!r}'.format(data))

                if data:
                    client_sock.sendall(data)

                else:
                    print('no data received from client')
                    break

        finally:
            client_sock.close()


if __name__ == '__main__':
    print("hello, test socket echo server")

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        addr = ('localhost', 10000)

        sock.bind(addr)

        sock.listen(1)

        be_server(sock)
    except socket.error as msg:
        print('socket Error: {}'.format(msg))
        sys.exit(1)

    except KeyboardInterrupt:
        print('server do Exit')

    finally:
        sock.close()
