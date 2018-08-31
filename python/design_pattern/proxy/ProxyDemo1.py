#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Implementation:

    def g(self):
        print("hello, G")

    def f(self):
        print("hello, F")

    def h(self):
        print("hello, H")

class Proxy:

    def __init__(self):

        self._implementation = Implementation()


    def __getattr__(self, name):
        return getattr(self._implementation, name)


if __name__ == '__main__':
    print("Proxy Design mode testing...")

    p = Proxy()
    p.g()
    p.f()
    p.h()

