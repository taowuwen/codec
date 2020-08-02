#!/usr/bin/env python3

import concurrent.futures
from xixitao.math import C_N_n, A_N_n

class Node:
    def __init__(self, num):
        self._num = num

    def __str__(self):
        return str(self._num)

    def __repr__(self):
        return str(self)

    @property
    def value(self):
        return self._num

    def __add__(self, another):
        if isinstance(another, Node):
            return self.value + another.value

        if isinstance(another, int):
            return self.value + another

    def __lt__(self, another):
        if isinstance(another, Node):
            return self.value < another.value

        if isinstance(another, int):
            return self.value < another

    def __gt__(self, another):
        if isinstance(another, Node):
            return self.value > another.value

        if isinstance(another, int):
            return self.value > another

    def __int__(self):
        return self._num


def valid_triangle(a, b, c):

    def _valid_triangle(x, y, z):
        if (x + y) > z:
            return True

        return False

    if  _valid_triangle(a, b, c) and \
        _valid_triangle(a, c, b) and \
        _valid_triangle(b, c, a):
            return (f'({a}, {b}, {c})', True)

    return (f'{a}, {b}, {c}', False)


def do_get_result(*nodes):
    Cnn = C_N_n(len(nodes), 3)
    success = 0
    failed  = 0

    with concurrent.futures.ThreadPoolExecutor(max_workers = 10) as executor:
        results = [executor.submit(valid_triangle, *args) for args in Cnn.parse(*nodes)]

        for res in concurrent.futures.as_completed(results):
            if res.result()[1]:
                success += 1
            else:
                failed += 1

    print(f'success = {success}, failed = {failed}')


def main():
    while True:
        print("Input Numbers: ")
        nodes = [Node(int(n)) for n in input().split()]
        do_get_result(*nodes)

if __name__ == '__main__':
    #main()

    do_get_result( *[x for x in range(1, 1000) if x % 10 == 0 ])
