#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import numpy as np
import sys
from numpy import pi


def do_print_np(a):
    print("=====================================")
    print(a)
    print("id(a): {}".format(id(a)))
    print("dim: {}".format(a.ndim))
    print("shape: {}".format(a.shape))
    print("dtype.name: {}".format(a.dtype.name))
    print("size: {}".format(a.size))
    print("itemsize: {}".format(a.itemsize))
    



def test_basic():

    a = np.arange(15).reshape(3,5)
    do_print_np(a)


def test_array_creation():
    a = np.arange(15).reshape(3, 5)

    do_print_np(np.array(range(4)))
    do_print_np(np.zeros((4,5), dtype=int))
    do_print_np(np.ones((4,5), dtype=int))
    do_print_np(np.zeros_like(a))
    do_print_np(np.ones_like(a))

    do_print_np(np.arange(10, 30, 5))
    do_print_np(np.arange(1, 2, 0.25))
    do_print_np(np.linspace(1, 2, 9))

    x = np.linspace(0, 2*pi, 100)
    f = np.sin(x)
    print(x)
    print(f)

    x = np.array([0, pi/2, pi/4, pi, pi/3])
    f = np.sin(x)
    print(x)
    print(f)

    c = np.arange(24).reshape(2,3,4)
    do_print_np(c)


def test_array_operation():

    a = np.arange(20, 60, 10)
    b = np.arange(4)
    do_print_np(a)
    do_print_np(b)

    do_print_np(a - b)
    do_print_np(b ** 2)
    do_print_np(10 * np.sin(a))
    do_print_np(a < 35)
            


if __name__ == '__main__':

    print("hello, testing {}".format(sys.argv[0][:-3]))

    test_basic()
    test_array_creation()
    test_array_operation()
