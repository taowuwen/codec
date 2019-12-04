#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import random
import sys


class LoadBalancer:

    def __init__(self, M = 128):

        self.servers = [ None for x in range(M) ]
        self._m = M

    def hash(self, hash_id):
        return hash_id % self._m

    def register(self, s):

        base = 0

        if s not in self.servers:

            while True:

                _id = self.hash(id(s) + base)

                if self.servers[_id]:
                    base += 11
                    continue

                self.servers[_id] = s
                break

    def unregister(self, s):

        if s in self.servers:
            self.servers.remove(s)

    def __str__(self):
        return f'{self.__class__.__name__} {self.servers}'

    def __repr__(self):
        return self.__str__()

    def find_server(self, r):

        _id = self.hash(id(r))

        for s in self.servers[_id:] + self.servers[:_id]:
            if s:
                return s

    def request(self, r):
        s = self.find_server(r)
        if s:
            return s.request(r)
        else:
            return "no server available for now"


class Server:

    def __init__(self):
        self.clients = set()

    def __str__(self):
        return f'{self.__class__.__name__}_{len(self.clients)}_{id(self)}'

    def __repr__(self):
        #return f'{self.__class__.__name__}'
        return self.__str__()

    def request(self, r):

        st = "registed"

        if r not in self.clients:
            self.clients.add(r)
            st = "new"


        return f'{self} -> {st}, {r}'


class Client:

    def __init__(self, server, key="hello"):
        self._key = key
        self._server = server

    def request(self, r = None):

        if not r:
            self._key = r

        print(self._server.request(self))

    def __str__(self):
        return f'C {self.__class__.__name__} {id(self)} {self._key}'

    def __repr__(self):
        return f'{self.__class__.__name__}'


def main(lb):

    clients = [ Client(lb, x*x) for x in range(100) ]

    print("first round, do register")
    print(lb)

    for c in clients:
        c.request()

    print("second round, do register")
    for c in clients:
        c.request()


    print("third round, add more server nodes")
    servers =[ Server(), Server()]

    for s in servers:
        lb.register(s)

    for c in clients:
        c.request()

    print("fouth round, delete one")

    lb.unregister(servers[0])
    for c in clients:
        c.request()

    print("fifth round, recovery")

    lb.register(servers[0])
    for c in clients:
        c.request()


if __name__ == '__main__':

    lb = LoadBalancer(49)

    nodes = [ Server() for x in range(5) ]

    for s in nodes:
        lb.register(s)

    sys.exit(main(lb))

