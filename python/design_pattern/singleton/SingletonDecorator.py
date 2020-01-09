#!/usr/bin/env python3
# -*- coding: utf-8 -*- 



class SingletonDecorator:

    def __init__(self, klass):
        self.klass = klass
        self.instance = None


    def __call__(self, *args, **kwargs):
        if not self.instance:
            self.instance = self.klass(*args, **kwargs)

        return self.instance


class foo: pass
foo = SingletonDecorator(foo)

class bar: pass
bar = SingletonDecorator(bar)




class Singleton:
    _inst_singleton = None

    def __new__(cls, *kargs, **kwargs):

        if not cls._inst_singleton:
            cls._inst_singleton = super().__new__(cls, *kargs, **kwargs)

        return cls._inst_singleton



if __name__ == '__main__':
    print("testing for Decorator....")

    x = foo()
    y = foo()
    z = foo()

    print(x, id(x))
    print(y, id(y))
    print(z, id(z))

    x.val = 'hello'
    print(x, id(x), x.val)

    y.val = 'world'
    print(y, id(y), y.val)

    z.val = 'hello, world'
    print(z, id(z), z.val)
    print(y, id(y), y.val)

    print(x, id(x), x.val)

    x = bar()
    x.val = 'hello'
    print(x, id(x), x.val)
    
    print(z, id(z), z.val)
    print(y, id(y), y.val)


    st1 = Singleton()
    st2 = Singleton()
    st3 = Singleton()
    st4 = Singleton()
    st5 = Singleton()

    print(id(st1))
    print(id(st2))
    print(id(st3))
    print(id(st4))
    print(id(st5))

