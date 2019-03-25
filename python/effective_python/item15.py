#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from ep import show_item_info
from ep import timeit
import time
import random
import copy


def sort_priority(values, group):
    def helper(x):
        if x in group:
            return (0, x)
        return (1, x)
    values.sort(key=helper)

def sort_priority2(values, group):
    found = False
    def helper(x):
        if x in group:
            found = True
            return (0, x)

        return (1, x)
    values.sort(key=helper)
    return found


def sort_priority3(values, group):
    found = False
    def helper(x):
        nonlocal found
        if x in group:
            found = True
            return (0, x)

        return (1, x)
    values.sort(key=helper)
    return found

class Sorter:

    def __init__(self, group):
        self.group = group
        self.found = False

    def __call__(self, x):

        if x in self.group:
            self.found = True
            return (0, x)
        return (1, x)

def sort_priority5(values, group):
    found = [False,]
    def helper(x):
        nonlocal found
        if x in group:
            found[0] = True
            return (0, x)

        return (1, x)
    values.sort(key=helper)
    return found[0]


@show_item_info
@timeit
def main():

    random.seed(int(time.time()))

    numbers = [random.randint(1,10) for a in range(10) ]
    group = [ a for a in range(1, 10, 2)]

    print("Before: {}".format(numbers))
    print("Group: {}".format(group))
    cp = copy.deepcopy(numbers)
    sort_priority(cp, group)
    print("After Sort1: {}".format(cp))

    cp = copy.deepcopy(numbers)
    r = sort_priority2(cp, group)
    print("After Sort2: {}, found: {}".format(cp, r))

    cp = copy.deepcopy(numbers)
    r = sort_priority3(cp, group)
    print("After Sort3: {}, found: {}".format(cp, r))

    cp = copy.deepcopy(numbers)
    sorter = Sorter(group)
    cp.sort(key = sorter)
    print("After Sort4: {}, found: {}, this is class".format(cp, sorter.found))

    cp = copy.deepcopy(numbers)
    r = sort_priority5(cp, group)
    print("After Sort5: {}, found: {}".format(cp, r))




if __name__ == '__main__':
    main()
