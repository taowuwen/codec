#!/usr/bin/env python3


class OnlyOne:

    class __OnlyOne:
        def __init__(self, arg):
            self.val = arg

        def __str__(self):
            return repr(self) + self.val

    instance = None

    def __init__(self, arg):

        if not OnlyOne.instance:
            OnlyOne.instance = OnlyOne.__OnlyOne(arg)

        else:
            OnlyOne.instance.val = arg

    def __getattr__(self, arg):
        return getattr(OnlyOne.instance, arg)

    def __str__(self):
        return str(OnlyOne.instance)


if __name__ == '__main__':

    x = OnlyOne('hello')
    print(x)
    y = OnlyOne('foo')
    print(y)
    z = OnlyOne('bar')
    print(z)
    print(x)
    print(y)
    print(x.val)

