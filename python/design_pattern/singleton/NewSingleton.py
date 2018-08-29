#!/usr/bin/env python3
# -*- coding: utf-8 -*-



class OnlyOne:

    class __OnlyOne:
        def __init__(self):
            self.val = None

        def __str__(self):
            return repr(self) + self.val

    instance = None

    def __new__(cls):
        if not OnlyOne.instance:
            OnlyOne.instance = OnlyOne.__OnlyOne()
        return OnlyOne.instance

    def __getattr__(self, name):
        return getattr(OnlyOne.instance, name)

    def __setattr__(self, name, val):
        return setattr(OnlyOne.instance, name, val)

if __name__ == '__main__':
    print("test for new singleton")

    a = OnlyOne()
    a.val = "aaa"
    print(a)

    b = OnlyOne()
    c = OnlyOne()
    b.val = "bbb"
    print(a)
    print(b)
    print(c)

