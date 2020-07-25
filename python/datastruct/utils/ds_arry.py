#!/usr/bin/env python3

from array import array
from copy import copy

class InvalidDimensionArray(Exception): pass

class DSArray:

    def __init__(self, _sz = 32):
        self._size = _sz
        self._len  = 0
        self._data = array('h', [0 for x in range(self._size)])

    def _offset(self, idx):
        if not 0 <= idx < self._size:
            raise IndexError('Out of Index')
        return idx

    def __getitem__(self, idx):
        return self._data[self._offset(idx)]

    def __setitem__(self, idx, item):
        self._data[self._offset(idx)] = item

    def __str__(self):
        return " ".join(str(self._data[a]) for a in range(self._size) )


class ListBasedArray:
    def __init__(self, _size):
        self._pos = 0
        self._size = _size
        self.arry = DSArray(self._size)

    def insert(self, pos, item):
        pass

    def append(self, item):
        pass

    def pop(self, item):
        pass

    def remove(self, idx):
        pass

    def find(self, item):
        pass

    def index(self, item):
        pass

class QueueBasedArray:
    def __init__(self, _size):
        pass

    def empty(self):
        return self._pos == 0

    def full(self):
        return self._pos == self._size

class StackBasedArray:
    def __init__(self, _size):
        self._pos = 0
        self._size = _size
        self._ary = DSArray(self._size)

    def pop(self):
        if self.empty():
            return None

        self._pos -= 1
        return self._ary[self._pos]

    def push(self, item):
        if self.full():
            return -1

        self._ary[self._pos] = item
        self._pos += 1
        return 0

    def full(self):
        return self._pos == self._size

    def empty(self):
        return self._pos == 0

    @property
    def length(self):
        return self._pos

    def __str__(self):
        return f'[{", ".join(str(self._ary[a]) for a in range(self._pos))}]'



class MultiDimensionalArray:
    def __init__(self, *dimensions):
        self._dimensions = [ d for d in dimensions if d > 0]
        self._ND         = len(dimensions)

        if len(self._dimensions) != self._ND:
            raise InvalidDimensionArray('Invalid Arguments')

        self._arry_size = self.multiple_all(self._dimensions)
        self._ary = DSArray(self._arry_size)

        self._factors = [1 for d in range(self._ND)]
        idx = self._ND -1

        while idx > 0:
            self._factors[idx - 1] = self._factors[idx] * self._dimensions[idx]
            idx -= 1

        print(self._factors)

    def multiple_all(self, lst):
        res = 1

        for item in lst:
            res *= item

        return res

    def _offset(self, key):
        _pos = 0
        if not isinstance(key, tuple) or len(key) != self._ND:
            raise InvalidDimensionArray(f'this is a {self._ND} demension array, not {len(key)}')

        for idx in range(self._ND):
            if key[idx] < 0 or key[idx] >= self._dimensions[idx]:
                raise IndexError('Index Error for {key}, pos {idx}')

        return sum([key[idx] * self._factors[idx] for idx in range(self._ND)])

    def __getitem__(self, key):
        return self._ary[self._offset(key)]

    def __setitem__(self, key, val):
        self._ary[self._offset(key)] = val

    def __str__(self):
        return str(self._ary)

def main():

    arr = DSArray(10)
    arr[1] = 12
    print(arr)

    stack = StackBasedArray(10)
    stack.push(1)
    stack.push(2)
    stack.push(3)
    print(stack)
    stack.pop()
    print(stack)
    stack.pop()
    print(stack)

    stack.pop()
    print(stack)

    mary = MultiDimensionalArray(2,3,4)
    mary[1,2,3] = 23
    mary[0,1,2] = 13
    mary[0,0,0] = 100
    mary[1,0,0] = 12

    print(mary)

if __name__ == '__main__':
    main()
