#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Hello:

    def hello(self, name="world"):
        print("hello, {}".format(name))


def fn(self, name="world"):
    print("hello, {}".format(name))

Hello1 = type('Hello1', (object, ), dict(hello=fn))

if __name__ == '__main__':
    print("hello, testing for metaclass")

    h = Hello()
    h.hello('tww')
    print(type(h))
    print(type(Hello))

    h = Hello1()
    h.hello('tww')
    print(type(h))
    print(type(Hello))

    help(type)
