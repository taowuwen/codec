#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from functools import wraps
import time

def separate_func(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        print()
        print()
        print("------------------------------------------------------")
        print("-----------{:^30}-------------".format(func.__name__))
        return func(*args, **kwargs)
    return wrapper

def timeit(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        print("")
        print("")
        print("------------------------------------------------------")
        print("-----------{:^30}-------------".format(func.__name__))
        s = time.time()
        res =  func(*args, **kwargs)
        e = time.time()
        print("-----------END of {:^30} -> time: {}".format(func.__name__, e - s))
        return res

    return wrapper
