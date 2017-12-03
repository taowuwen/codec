#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import copy
import functools

@functools.total_ordering
class MyClass:

    def __init__(self, name):
        self.name = name
        self.ls = ["abcde", "feh"]

    def __eq__(self, other):
        return self.name == other.name

    def __gt__(self, other):
        return self.name > other.name


def test_copy():
    a = MyClass('a')
    mylist = [a]
    dup = copy.copy(mylist)

    print("id(a) == {}".format(id(a)))
    print("mylist {}".format(mylist))
    print("dup {}".format(dup))

    print("id(mylist) {}".format(id(mylist)))
    print("id(dup) {}".format(id(dup)))

    print("dup == mylist? {}".format(dup == mylist))
    print("dup is mylist? {}".format(dup is mylist))

    print("dup[0] == mylist[0]? {}".format(dup[0] == mylist[0]))
    print("dup[0] is mylist[0]? {}".format(dup[0] is mylist[0]))
    print(a.name, a.ls)
    print(mylist[0].name, mylist[0].ls)
    print(dup[0].name, dup[0].ls)

    
def test_deepcopy():
    a = MyClass('a')
    mylist = [a]
    dup = copy.deepcopy(mylist)

    print("id(a) == {}".format(id(a)))
    print("mylist {}".format(mylist))
    print("dup {}".format(dup))

    print("id(mylist) {}".format(id(mylist)))
    print("id(dup) {}".format(id(dup)))

    print("dup == mylist? {}".format(dup == mylist))
    print("dup is mylist? {}".format(dup is mylist))

    print("dup[0] == mylist[0]? {}".format(dup[0] == mylist[0]))
    print("dup[0] is mylist[0]? {}".format(dup[0] is mylist[0]))

    print(a.name, a.ls)
    print(mylist[0].name, mylist[0].ls)
    print(dup[0].name, dup[0].ls)


def main():
    print("hello, test copy and deep copy")
    test_copy()
    test_deepcopy()


if __name__ == '__main__':
    main()
