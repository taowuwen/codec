#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os
from functools import wraps
import time

def timeit(f):
    @wraps(f)
    def wrapper(*args, **kwargs):

        print("---START OF {:^30}---".format(f.__name__))
        s = time.time()
        r = f(*args, **kwargs)
        e = time.time()
        print("---END OF {:^30} -> time:{}---:".format(f.__name__, e - s))

    return wrapper


def show_item_info(f):
    @wraps(f)
    def wrapper(*args, **kwargs):

        print("\n--START Effective Python: {:^20}--\n".format(sys.argv[0]))
        res = f(*args, **kwargs)
        print("\n--END   Effective Python: {:^20}--".format(sys.argv[0]))

        return f

    return wrapper


@show_item_info
@timeit
def main():
    print("hello, this is effective python common lib")

if __name__ == '__main__':
    main()
