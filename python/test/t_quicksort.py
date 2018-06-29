#!/usr/bin/env python3

import random
import time

def quicksort(ary):

    print(ary)

    if len(ary) <= 1:
        return ary

    pivot = ary[len(ary)//2]

    _l = [x for x in ary if x < pivot ]
    _m = [x for x in ary if x == pivot ]
    _r = [x for x in ary if x > pivot ]

    return quicksort(_l) + _m + quicksort(_r)


def main():
    random.seed(int(time.time()))
    print(quicksort([random.randint(1, 100) for x in range(20)]))

if __name__ == '__main__':
    main()
