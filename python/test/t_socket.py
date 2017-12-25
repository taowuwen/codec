#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import os
import sys

from test_utils import separate_func
from test_utils import timeit


HOSTS = [
        'apu',
        'www.baidu.com',
        'www.google.com',
        'www.cuit.edu.cn',
        'xixitao.com',
]

HOST_ADDR = [
        '209.141.33.9',
        '8.8.8.8',
        '10.1.1.1',
        '127.0.0.1'
]

'''
family=socket.AF_INET,
type=socket.SOCK_STREAM,
proto=socket.IPPROTO_TCP,
flags=socket.AI_CANONNAME
'''

@timeit
def _test_getaddrinfo():

    for host in HOSTS:

        print(host)
        try:
            for rsp in socket.getaddrinfo(host,
                    'http',
                    type=socket.SOCK_STREAM,
                    proto=socket.IPPROTO_TCP,
                    flags=socket.AI_CANONNAME):
                family, socktype, proto, canoname, sockaddr = rsp

                print('\t{:<30}: {}'.format("family", family))
                print('\t{:<30}: {}'.format("socktype", socktype))
                print('\t{:<30}: {}'.format("proto", proto))
                print('\t{:<30}: {}'.format("canoname", canoname))
                print('\t{:<30}: {}'.format("sockaddr", sockaddr))

        except socket.error as msg:
            print('\t ERROR'.format(msg))

        print('----------------')


@timeit
def _test_gethostbyaddr():

    for addr in HOST_ADDR:
        hostname, aliases, addresses = socket.gethostbyaddr(addr)

        print("{:<30}: {}".format('Name', hostname))
        print("{:<30}: {}".format('Aliases', aliases))
        print("{:<30}: {}".format('Addresses', addresses))


@timeit
def _test_getfqdn():
    for host in HOSTS:
        print('{:<30} -> {}'.format(host, socket.getfqdn(host)))

@timeit
def _test_gethostbyname_ex():

    for host in HOSTS:
        print(host)

        try:
            name, aliases, addresses = socket.gethostbyname_ex(host)

            print("\t Name: {}".format(name))
            print("\t Aliases: {}".format(aliases))
            print("\t Addresses: {}".format(addresses))

        except socket.error as msg:
            print('ERROR {}'.format(msg))


@timeit
def _test_gethostbyname():

    for item in HOSTS:
        try:
            print("\t{} -> {}".format(item, socket.gethostbyname(item)))
        except socket.error as msg:
            print('{} : {}'.format(item, msg))


if __name__ == '__main__':
    print('hello, socket testing\n\tgethostname() = {}'.format(
                                                    socket.gethostname()))

#    _test_gethostbyname()
#    _test_gethostbyname_ex()
#    _test_getfqdn()
#    _test_gethostbyaddr()
    _test_getaddrinfo()
