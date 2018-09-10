#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from pprint import pprint


class UpperAttrMetaclass(type):
    def __new__(cls, name, bases, dct):
        attrs = ((name, value) for name, value in dct.items() if not name.startswith('__'))
        uppercase_attr = dict((name.upper(), value) for name, value in attrs)
        return super(UpperAttrMetaclass, cls).__new__(cls, name, bases, uppercase_attr)


class UpperCaseMetaClass(type):

    def __new__(cls, name, bases, attrs):
        new_attrs = dict((name.upper(), val) for name, val in attrs.items() if not name.startswith('__'))
        return super().__new__(cls, name, bases, new_attrs)
        #return type(name, bases, new_attrs)


# for python 2.x version
class Foo:
    __metaclass__ = UpperAttrMetaclass
    bar = 'aaa'


# for python 3.x version
class Bar(object, metaclass=UpperCaseMetaClass):
    foo = 'foo'


class Hello:

    def hello(self, name="world"):
        print("hello, {}".format(name))


def fn(self, name="world"):
    print("hello, {}".format(name))

Hello1 = type('Hello1', (object, ), dict(hello=fn))

if __name__ == '__main__':
    print("testing for metaclass....")

    f = Foo()
    pprint([a for a in dir(f) if not a.startswith('__')])

    b = Bar()
    pprint([a for a in dir(b) if not a.startswith('__')])

    h = Hello()
    h.hello('tww')
    print(type(h))
    print(type(Hello))

    h = Hello1()
    h.hello('tww')
    print(type(h))
    print(type(Hello))
