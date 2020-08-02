#!/usr/bin/env python3

from .excpt import *

class X_N_n:

    def __init__(self, N, n):
        self.transform(N, n)

    def transform(self, N, n):
        if N < n or N <= 0 or n <= 0:
            raise InvalidArgument('Invalid arguements')

        self._N = N
        self._n = n
        self._result = []

    @property
    def type(self):
        return 'X'

    def __len__(self):
        return len(self._result)

    @property
    def size(self):
        return len(self)

    def parse(self, args, *kargs):

        _srcs = args
        if kargs:
            _srcs = [args] + list(kargs)

        if len(_srcs) != self._N:
            raise InvalidArgument(f'invalid argument, len(_srcs) != {self._N}')

        if not self._result:
            self.do_X_N_n()

        for items in self._result:
            yield [_srcs[x] for x in items]

    def _X_N_n(self, result, src, dst, N, n):
        for idx, a in enumerate(src):

            dst.append(a)
            if n == 1:
                self.push_one_result(result, dst)
                dst.pop(-1)
            else:
                self._X_N_n(result, src[0:idx] + src[idx+1:], dst, N-1, n-1)
        dst.pop(-1)

    def do_X_N_n(self):

        result = set()
        src = [a for a in range(self._N)]

        for idx, a in enumerate(src):
            dst=[a]

            if self._n == 1:
                self.push_one_result(result, dst)
            else:
                self._X_N_n(result, src[0:idx] + src[idx+1:], dst, self._N -1, self._n-1)

        for res in sorted(result):
            self._result.append([int(x) for x in res.split(',')])

class C_N_n(X_N_n):
    def push_one_result(self, result, dst):
        key = ",".join([str(x) for x in sorted(dst)])
        result.add(key)

class A_N_n(X_N_n):
    def push_one_result(self, result, dst):
        key = ",".join([str(x) for x in dst])
        result.add(key)

class Pow_X_N_1(A_N_n):
    def __init__(self, N, _p):
        super().__init__(N, 1)
        self._pow = _p
        if self._pow <= 0:
            raise InvalidArgument(f'power : {_p} <= 0')

    def __len__(self):
        return pow(self._N, 1)

    def do_X_N_n(self):
        assert self._n == 1, "Never show up this line"

        result = set()
        srcs = [x for x in range(self._N)]

        for idx,val in enumerate(srcs):
            dst = [val]

            if self._pow == 1:
                self.push_one_result(result, dst)
            else:
                self.pow_x_n(result, srcs, dst, self._pow - 1)

        for res in sorted(result):
            self._result.append([int(x) for x in res.split(',')])

    def pow_x_n(self, result, srcs, dst, _pow):
        for idx,val in enumerate(srcs):
            dst.append(val)

            if _pow == 1:
                self.push_one_result(result, dst)
                dst.pop(-1)
            else:
                self.pow_x_n(result, srcs, dst, _pow - 1)
        dst.pop(-1)

def main():

    CNn = C_N_n(5, 4)
    ANn = A_N_n(5, 4)
    PNn = Pow_X_N_1(4,3)

    C42 = C_N_n(4, 2)

    for items in PNn.parse('+-*/'):
        print(items)

    for items in PNn.parse([x for x in '+-*/']):
        print(items)

        for pos in C42.parse(0, 1, 2, 3):
            items.insert(pos[1], ')')
            items.insert(pos[0], '(')
            print(items)
            items.pop(pos[0])
            items.pop(pos[1])

    A44 = A_N_n(4, 4)
    for items in A44.parse(0, 1, 2, 3):
        print(items)



#   for items in PNn.parse('+', '-', '*', '/'):
#       print(items)
#
#   for items in PNn.parse(*[x for x in '+-*/']):
#       print(items)
#
#   for items in CNn.parse(10, 20, 30, 40, 50):
#       print(items)
#
    

#   for items in ANn.parse(10, 20, 30, 40, 50):
#       print(items)

#   for items in CNn.parse(10, 20, 30, 40, 50):
#       print(items)
#
#   sources = [ x for x in range(1000) if x % 11 == 0 ]
#   CNn.transform(len(sources), 3)
#
#   for items in CNn.parse(*sources):
#       print(items)

if __name__ == '__main__':
    main()

