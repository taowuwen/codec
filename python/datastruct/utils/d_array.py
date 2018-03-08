#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class InvalidOperation(Exception): pass

class CArray:

    '''
    define array usage
    '''

    def __init__(self, length=0, base=0):

        assert length > 0

        self._data = [None for i in range(length)]
        self._base = base

    def _offset(self, index):

        _off = index - self._base

        if _off < 0 or _off >= len(self._data):
            raise IndexError

        return _off

    def __getitem__(self, index):
        return self._data[self._offset(index)]

    def __setitem__(self, index, val):
        self._data[self._offset(index)] = val

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

    def __copy__(self):

        result = CArray(len(self._data))

        for i, _d in enumerate(self._data):
            result[i] = _d

        result._base = self._base
        return result

    @property
    def data(self):
        return self._data

    @property
    def base(self):
        return self._base

    @base.setter
    def base(self, val):
        self._base = val




def _test_array():
    a = CArray(10)
    print(a.length)
    a[0] = 'aaa'
    a[2] = 'bbbccc'
    a[1] = 12
    print(a)
    from copy import copy

    b = copy(a)
    b[9] = 11
    print(b)
    print(b.base)
    print(b.data)


class MultiDimensionalArray:

    def __init__(self, *dimensions):

        self._dimensions = CArray(len(dimensions))
        self._factors = CArray(len(dimensions))
        product = 1

        i = len(dimensions) - 1

        while i >= 0:

            self._dimensions[i] = dimensions[i]
            self._factors[i] = product
            product *= self._dimensions[i]
            i -= 1

        self._data = CArray(product)

    def _offset(self, indices):

        if not isinstance(indices, tuple) or len(indices) != len(self._dimensions):
            raise IndexError

        offset = 0

        for i, dim in enumerate(self._dimensions):
            if indices[i] < 0 or indices[i] >= dim:
                raise IndexError

            offset += self._factors[i] * indices[i]

        return offset

    def __getitem__(self, indices):
        return self._data[self._offset(indices)]

    def __setitem__(self, indices, value):
        self._data[self._offset(indices)] = value

    def __str__(self):
        return str(self._data)



class Matrix:

    def __init__(self, row, col):
        self._row = row
        self._col = col

    @property
    def row(self):
        return self._row

    @property
    def col(self):
        return self._col

class DenseMatrix(Matrix):

    def __init__(self, row, col):

        super(DenseMatrix, self).__init__(row, col)
        self._array = MultiDimensionalArray(row, col)

    def __getitem__(self, index):
        return self._array[index]

    def __setitem__(self, index, value):
        self._array[index] = value

    def __str__(self):
        _str = "[" + "\n"

        for row in range(self._row):
            for col in range(self._col):
                _str += " " + str(self._array[row, col])
                _str += ","

            _str += "\n"

        _str += "]"

        return _str

    def __mul__(self, mat):
        """
        n*m x m *x == n * x
        """

        if self.col != mat.row:
            raise InvalidOperation

        nx = DenseMatrix(self.row, mat.col)

        for i in range(self.row):
            for j in range(mat.col):

                sum = 0
                for k in range(self.col):
                    sum += self[i, k] * mat[k, j]

                nx[i,j] = sum

        return nx

def _test_dense_matrices():
    dm = DenseMatrix(5, 6)

    dm[3, 5] = 123
    dm[1, 2] = 32
    dm[1, 5] = 33
    print(dm)

    dm_23 = DenseMatrix(2, 3)
    dm_31 = DenseMatrix(3, 1)

    dm_23[0, 0] = 1
    dm_23[0, 1] = 2
    dm_23[0, 2] = 3
    dm_23[1, 0] = 4
    dm_23[1, 1] = 5
    dm_23[1, 2] = 6

    dm_31[0,0] = 3
    dm_31[1,0] = 2
    dm_31[2,0] = 1

    print(dm_23)
    print(dm_31)
    print(dm_23 * dm_31)


def _test_multidimensional_array():

    a = MultiDimensionalArray(2, 3, 4)
    a[0,2,1] = 'hello, world'
    a[0,0,0] = 123
    a[0,1,0] = 345
    print(a[0,1,0])
    print(a[0,0,0])
    print(a)
    print(a[1,2,0])


if __name__ == '__main__':
    _test_array()
    _test_multidimensional_array()
    _test_dense_matrices()


