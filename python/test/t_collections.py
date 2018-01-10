#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import collections
from test_utils import timeit

@timeit
def _test_ChainMap():

    a = {'a': 'A', 'b': 'B', 'c':'C'}
    b = {'d': 'D', '1': 'one', 'c': 'two'}

    m = collections.ChainMap(a, b)

    print(m['c'])
    print(m['d'])

    m['c'] = 'E'
    m['f'] = 'FFFF'
    print(m['c'])
    print(m['f'])
    print(a)
    print(b)
    print(m)

    m1 = m.new_child()
    print(m1)
    print(m1['c'])
    m1['c'] = 'hello, world'
    print(m1['c'])
    print(m1)


if __name__ == '__main__':

    print('hello, collection testing')
    _test_ChainMap()
