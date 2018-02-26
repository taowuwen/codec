#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class CArray:

    '''
    define array usage
    '''

    def __init__(self, length=0, base=0):

        assert length > 0

        self._data = [None for i in range(length)]
        self._base = base

    def __getitem__(self, index):
        return self._data[index]

    def __setitem__(self, index, val):
        self._data[index] = val

    def __len__(self):
        return len(self._data)

    def set_length(self, l, base=0):

        if l != len(self._data):

            _data = [None for i in range(l)]

            _l = min(len(self._data), l)
            for i in range(_l):
                _data[i] = self._data[i]

            self._data = _data

        self._base = base

    length = property(fget=__len__, fset=set_length)

    def __str__(self):
        return str(self._data)



if __name__ == '__main__':

    a = CArray(10)
    print(a.length)
    a[0] = 'aaa'
    a[2] = 'bbbccc'
    print(a)
