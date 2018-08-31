#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class State_d:

    def __init__(self, imp):
        self._imp = imp

    def changeimp(self, imp):
        self._imp = imp

    def __getattr__(self, name):
        return getattr(self._imp, name)


class SM:

    def f(self):
        print("ffffffffffff {}".format(self.__class__))

    def g(self):
        print("gggggggggggg {}".format(self.__class__))

    def h(self):
        print("hhhhhhhhhhhh {}".format(self.__class__))

class Implementation1(SM):
    pass

class Implementation2(SM):
    pass
    

def run(r):
    r.f()
    r.g()
    r.h()
    r.g()


if __name__ == '__main__':
    print("Hello, testing for State")

    r = State_d(Implementation1())
    run(r)

    r.changeimp(Implementation2())
    run(r)
