#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from Synchronization import *

class C(Synchronization):
    def __init__(self):
        self.data = 1
        super().__init__()

    def m(self):
        self.data += 1
        return self.data

    m = synchronized(m)

    def f(self):
        return 47

    def g(self):
        return 'spam'


class D(C):
    def __init__(self):
        super().__init__()

    def f(self):
        return super().f()


class E(C):
    def __init__(self):
        super().__init__()

    def m(self):
        return super().m()

    def f(self):
        return super().f()

    def g(self):
        return super().g()



if __name__ == '__main__':
    print("testing synchronization...")

    c = C()
    synchronize(D)

    d = D()
    print(d.f())
    print(d.g())
    print(d.m())

    synchronize(E, 'm g')
    e = E()
    print(e.f())
    print(e.g())
    print(e.m())
