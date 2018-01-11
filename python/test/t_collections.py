#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import collections
from test_utils import timeit
import random
import time


@timeit
def _test_deque():

    d = collections.deque('abcdefgabc')
    print(d, len(d), d[0], d[-1])
    d.remove('c')
    print(d)

    d.extend('abc')
    print(d)

    d.extendleft('xxx')
    print(d)

    d.appendleft('ABC')
    print(d)

    print(d.pop())
    print(d.popleft())
    print(d)

    d = collections.deque(range(10))
    print(d)

    d.rotate(2)
    print(d)

    d.rotate(-2)
    print(d)

    d1 = collections.deque(maxlen=3)
    d2 = collections.deque(maxlen=3)

    random.seed(int(time.time()))

    for i in range(5):

        n = random.randint(0, 100)
        d1.append(n)
        d2.appendleft(n)

        print(n)
        print(d1)
        print(d2)


@timeit
def _test_defaultdict():

    def _mydefault():
        return 'default mydefault'

    d = collections.defaultdict(lambda : 'default value' , foo='bar')
    e = collections.defaultdict(_mydefault, foo='bar')
    print(d)
    print(d['foo'])
    print(d['aaa'])

    print(e)
    print(e['foo'])
    print(e['aaa'])


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

@timeit
def _test_counter():
    print(collections.Counter('abcdef'))

    c = collections.Counter()
    c.update('hello, world')
    c.update(['foo','bar'])
    c.update({'foo':2222})
    print(c)
    print(c.most_common(3))


    c1 = collections.Counter('hello')
    c2 = collections.Counter('world')

    print("c1 == ", c1)
    print("c2 == ", c2)

    print("c1 + c2:  ", c1 + c2)
    print("c1 - c2:  ", c1 - c2)
    print("c1 & c2:  ", c1 & c2)
    print("c1 | c2:  ", c1 | c2)

    print("c2 + c1:  ", c2 + c1)
    print("c2 - c1:  ", c2 - c1)
    print("c2 & c1:  ", c2 & c1)
    print("c2 | c1:  ", c2 | c1)


@timeit
def _test_namedtuple():

    bob = ('Bob', 29, 'male')
    lily = ('Lily', 30, 'female')

    fmt = '{:>12} {:>5} {:>8}'

    print(fmt.format('Name', 'Age', 'Gender'))
    print(fmt.format('-'*12, '-'*5, '-'*8))
    for p in [bob, lily]:
        print('{:>12} {:>5} {:>8}'.format(*p))


    print(fmt.format('-'*12, '-'*5, '-'*8))
    Person = collections.namedtuple('Person', 'name age gender')

    bob = Person('Bob', 29, 'male')
    lily = Person('Lily', 30, 'female')
    tao  = Person(name='Tao', age=30, gender='male')

    print(fmt.format('Name', 'Age', 'Gender'))
    print(fmt.format('-'*12, '-'*5, '-'*8))
    for p in [bob, lily, tao]:
        print('{:>12} {:>5} {:>8}'.format(p.name, p.age, p.gender))


    with_class = collections.namedtuple('Person', 'name class age', rename=True)
    print(with_class)
    print(with_class._fields)
    bol = with_class('Bob', '12', 12)
    print(bol)

    print('_asdict() {}'.format(bol._asdict()))
    bol_new = bol._replace(name='Bol')
    print(bol_new)
    print(bol is bol_new)



        

if __name__ == '__main__':

    print('hello, collections testing')
    _test_ChainMap()
    _test_counter()
    _test_defaultdict()
    _test_deque()
    _test_namedtuple()
