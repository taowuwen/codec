#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
from functools import wraps

def time_it(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        print("start exceution for : {}".format(func.__name__))
        s = time.time()
        r = func(*args, **kwargs)
        e = time.time()

        print("total exceution time is: {} {}".format(func.__name__, s -e ))
        
        return r

    return wrapper


def time_it_all_for_class(Cls):
    class NewCls:
        def __init__(self, *args, **kwargs):
            self.obj = Cls(*args, **kwargs)

        def __getattribute__(self, s):

            try:
                return super(NewCls, self).__getattribute__(s)

            except AttributeError:
                pass

            x = self.obj.__getattribute__(s)

            if type(x) == type(self.__init__):
                return time_it(x)
            else:
                return x

    return NewCls


@time_it_all_for_class
class MyClass:
    
    a = "abc"

    def method_a(self, size):
        abc = [x for x in range(size + 1) ]

        return sum(abc)

    @classmethod
    def method_b(cls):
        print("hello, class")



class MyClassDecrator:

    def __init__(self, f):
        print("WITHOUT params, in init")
        self.f = f

    def __call__(self, *args, **kwargs):

        s = time.time()

        r = self.f(*args, **kwargs)

        e = time.time()

        print("total exceution time is {}".format(s - e))
        return r

class MyClassDecratorWithParams:
    def __init__(self, arg1, arg2, *args):
        print("with params, in init" + arg1 + arg2 + " ,".join(args))
        self.arg1 = arg1
        self.arg2 = arg2

    def __call__(self, f):

        @wraps(f)
        def wrapper(*args, **kwargs):
            s = time.time()
            print("class params is {0.arg1} -- {0.arg2}".format(self))
            r = f(*args, **kwargs)
            e = time.time()

            return r
        return wrapper
        

@MyClassDecrator
def do_print(mystring=None):
    print("hello, world" + mystring)


@MyClassDecratorWithParams("foo", "bar", "hello", "world")
def do_print_arg(myargs=None):
    print("hello arguments versions " + myargs)


@MyClassDecrator
def do_print_without_params(mystring=None):
    print("WITHOUT, world" + mystring)

def test_func_as_decrators():
    mycls = MyClass()
    print(mycls.a)
    print(mycls.method_b())
    print(mycls.method_a(100))
    print(mycls.method_a(1000))


def test_class_as_decrators():
    do_print(" tww"*5)
    do_print_arg(" tao "*10)
    do_print_without_params(" wuwen "*3)




def main():
    print("hello, decorator class")

    test_func_as_decrators()
    test_class_as_decrators()


if __name__ == '__main__':
    main()
