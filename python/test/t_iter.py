#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class MyIter:
    def __init__(self, n):
        self._max = n
        self._cur = 0

    def __iter__(self):
        self._cur = 0
        return self

    def __next__(self):
        
        if self._cur < self._max:
            res = self._cur
            self._cur += 1

            return res

        else:
            raise StopIteration

class MyIter1:
    def __init__(self, n):
        self._vals = {a:a**2 for a in MyIter(n)}

    def __iter__(self):
        self.cur = iter(self._vals)
        return self

    def __next__(self):
        return next(self.cur)

    def keys(self):
        return self._vals.keys()

    def values(self):
        return self._vals.values()

    def items(self):
        return self._vals.items()

    def __getitem__(self, key):
        return self._vals[key]



def _main():
    print("hello, test iter")

    print([ a for a in MyIter(10)])

    for i in MyIter(4):
        print(i)

    myiter = MyIter1(10)

    for i in myiter:
        print(i)

    for i in myiter.keys():
        print("key = {}, values = {}".format(i, myiter[i]))

    for i in myiter.values():
        print(i)

    for key, value in myiter.items():
        print("{} : {}".format(key, value))



if __name__ == '__main__':
    _main()
