#!/usr/bin/env python


class CTestProperty():
    def __init__(self):
        self._x = 100

    @property
    def x(self):
        return self._x

    @x.setter
    def x(self, value):
        self._x = value


class CTestProperty_v2(object):
    def __init__(self):
        self._x = 100

    @property
    def x(self):
        return self._x

    @x.setter
    def x(self, value):
        self._x = value

    @x.deleter
    def x(self):
        del self._x


def main():
    p = CTestProperty()

    print("x is: %d" % (p.x))

    p.x = 10
    print("x is: %d" % (p.x))

    p = CTestProperty_v2()

    print("x is: %d" % (p.x))

    p.x = 10
    print("x is: %d" % (p.x))

    del p.x


if __name__ == '__main__':
    main()
