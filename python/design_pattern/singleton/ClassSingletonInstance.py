#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Singleton:

    __instance = None

    def __new__(cls, val):
        if not Singleton.__instance:
            #Singleton.__instance = super().__new__(cls)
            Singleton.__instance = object.__new__(cls)

        Singleton.__instance.val = val

        return Singleton.__instance

    def __str__(self):
        return Singleton.__instance.val


class OnlyOne(Singleton):
    pass


if __name__ == '__main__':

    a = OnlyOne('aaa')
    print(id(a),  a)
    b = OnlyOne('bbb')
    print(id(a), a)
    print(id(b), b)

