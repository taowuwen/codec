#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Borg:

    _shared_state = {}

    def __init__(self):
        self.__dict__ = Borg._shared_state

class OnlyOne(Borg):

    def __init__(self, val):
        super().__init__()
        self.val = val

    def __str__(self):
        return self.val

class OnlyOne_V1(Borg):
    def __init__(self, val):
        super().__init__()
        self.val = val

    def __str__(self):
        return repr(self) + " " + self.val


if __name__ == '__main__':
    print("Borg Singleton testing")

    a = OnlyOne('aaa')
    print(a)
    b = OnlyOne('bbb')
    print(b)
    c = OnlyOne('ccc')
    print(c)
    print(a)
    print(b)

    d = OnlyOne_V1('dddd')
    print(d)
    print(c)
    print(a)
    print(b)

