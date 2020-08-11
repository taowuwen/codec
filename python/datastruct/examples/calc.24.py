#!/usr/bin/env python3

from xixitao.math import Pow_X_N_1, C_N_n, A_N_n
from xixitao.excpt import InvalidArgument
import concurrent.futures
import copy

class Card:
    def __init__(self, card):
        self._num = {
                'T': 10,
                'J': 11,
                'Q': 12,
                'K': 13,
                'A': 1
            }.get(card, None)

        if not self._num:
            self._num = int(card)

        self._str = card

        if not 1 <= self._num <= 13:
            raise InvalidArgument('Invalid argument')

    def __str__(self):
        return self._str
    
    def __int__(self):
        return self._num

    def __repr__(self):
        return str(self)

    def __add__(self, other):
        if isinstance(other, Card):
            return self.value + other.value

        return self.value + other

    def __sub__(self, other):
        if isinstance(other, Card):
            return self.value - other.value

        return self.value - other

    def __mul__(self, other):
        if isinstance(other, Card):
            return self.value * other.value

        return self.value * other

    def __div__(self, other):
        if isinstance(other, Card):
            return self.value / other.value

        return self.value / other

    @property
    def value(self):
        return self._num



class Calc24:
    '''
    a + b + c + d
    (a + b) + c + d 
    [pos1] + [pos2] + [pos3] + [pos4]
    C(4,2) = 6
    '''

    def __init__(self):

        self.C42 = C_N_n(4, 2)
        self.P43 = Pow_X_N_1(4, 3)
        self.A44 = A_N_n(4, 4)

    def _build_expression(self, a, b, c, d):
        for num in self.A44.parse(a, b, c, d):
            for symb in self.P43.parse('+-*/'):
                # do calculate one
                yield (num, symb)

                for pos in self.C42.parse(0, 1, 2, 3):
                    symb.insert(pos[1], ')')
                    symb.insert(pos[0], '(')

                    yield (num, symb)

                    symb.pop(pos[0])
                    symb.pop(pos[1])

    def calc(self, a, b, c, d):

        success = 0
        failed  = 0

        with concurrent.futures.ThreadPoolExecutor(max_workers = 10) as executor:
            results = []
            for args in self._build_expression(a, b, c, d):
                results.append(executor.submit(self._do_calc, ",".join([ str(x) for x in args[0]]), ",".join(args[1])))

            for res in concurrent.futures.as_completed(results):
                if res.result()[1]:
                    success += 1
                    print(f'{res.result()[0]} = 24')
                else:
                    failed += 1

    def _do_calc(self, _num = '', _symb = ''):

        exp = []
        num = [ x for x in _num.split(',')]
        symb = [x for x in _symb.split(',')]

        x = 0
        tag_closed = 0

        while symb:

            s = symb.pop(0)

            if s == '(':
                exp.append(s)
                continue

            if s in '+-*/':
                if tag_closed == 1:
                    tag_closed = 0
                    exp.append(s)
                else:
                    exp.append(num.pop(0))
                    exp.append(s)

            if s == ')':
                exp.append(num.pop(0))
                exp.append(s)
                tag_closed = 1
        if num:
            assert len(num) == 1, "never show up this line"
            exp.append(num.pop(0))

        _exp = " ".join([str(x) for x in exp])

        try:
            ret = eval(_exp)
        except ZeroDivisionError:
            return (_exp, False)

        else:
            if ret in (24, 24.0, 23.99999999999999):
                return (_exp, True)

        return (_exp, False)


def main():
    exp = Calc24()

    while True:
        print("Input Numbers: ")
        cards = [Card(n) for n in input().split()]
        print(cards)
        exp.calc( *[int(card) for card in cards ])

if __name__ == '__main__':
    main()
