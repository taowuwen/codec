#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import socket
import os
import sys

addr = '/tmp/socket_unix.domain'


if __name__ == '__main__':

    print("hello, this is socket test for unix domain client")

    msg = b'msg send to unix domain server'

    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

    try:
        sock.connect(addr)

        sock.sendall(msg)

        recv = len(msg)

        while recv > 0:
            data = sock.recv(16)
            recv -= len(data)

            print('received data {!r}, left = {}'.format(data, recv))

    except socket.error as msg:
        print('Error: {}'.format(msg))

    finally:
        sock.close()
