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

    property


