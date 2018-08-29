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
