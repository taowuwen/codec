#!/usr/bin/env python3

from xixitao.arry import MultiDimensionalArray
from xixitao.excpt import InvalidArgument

class GroupArray:
    def __init__(self, top, y, x):
        self._top = top
        self._x = x
        self._y = y

    @property
    def x(self):
        return self._x

    @property
    def y(self):
        return self._y

    @property
    def top(self):
        return self._top

    def __setitem__(self, key, val):
        self._top[self._x, self._y, key[0], key[1]] = val

    def __getitem__(self, key):
        return self._top[self._x, self._y, key[0], key[1]]

class TripleGroupArray:
    def __init__(self):
        self._ary = MultiDimensionalArray(9,9)

        self._grps = [ GroupArray(self, x, y) for x in range(3) for y in range(3) ]

    def virtual_to_physical(self, key):
        if len(key) != 4:
            raise InvalidArgument('Invlaid arguments')

        if any([ a <0 or a >= 3 for a in key]):
            raise InvalidArgument(f'out of range {key}')

        g_y, g_x, y, x = key

        return ((g_y * 3 + y), g_x * 3 + x)

    def __setitem__(self, key, val):
        if len(key) == 4:
            y,x = self.virtual_to_physical(key)
            self._ary[y, x] = val
        else:
            self._ary[key] = val

    def __getitem__(self, key):
        if len(key) == 4:
            y, x = self.virtual_to_physical(key)
            return self._ary[y, x]

        return self._ary[key]

    def pprint(self):

        for y in range(9):
            print([self._ary[y, x] for x in range(9)])

    def get_group(self, x, y):

        for grp in self._grps:
            if grp.x == x and grp.y == y:
                return grp

        return None

    def group(self, x):
        _y = x // 3
        _x = x % 3

        return self.get_group(_y, _x)

def main():
    print("hello, 9x9 == 9 x 3x3")
    tga = TripleGroupArray()

    for grpid  in range(9):
        grp = tga.group(grpid)

        for y in range(3):
            for x in range(3):
                grp[x, y] = x + y
                
    tga.pprint()


if __name__ == '__main__':
    main()
