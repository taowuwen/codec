#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class A:
    def __init__(self, x): print("init, A {}".format(x))

class B:
    def __init__(self, x): print("init, B {}".format(x))

class C:
    def __init__(self, x): print("init, C {}".format(x))


class Facade:
    @staticmethod
    def makeA(x): return A(x)
    @staticmethod
    def makeB(x): return B(x)
    @staticmethod
    def makeC(x): return C(x)


if __name__ == '__main__':
    print("hello, test facade...")

    x = 1

    a = Facade.makeA(x)
    b = Facade.makeB(x)
    c = Facade.makeC(x)
