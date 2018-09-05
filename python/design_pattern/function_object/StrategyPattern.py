#!/usr/bin/env python3
# -*- coding: utf-8 -*-



class FindMinima:
    def algorithm(self, line): pass


class LeastSquares(FindMinima):
    def algorithm(self, line):
        return [1.1, 2.2]

class NewtonsMethod(FindMinima):
    def algorithm(self, line):
        return [3.3, 4.4]

class Bisection(FindMinima):
    def algorithm(self, line):
        return [5.5, 6.6]


class MinimaSolver:

    def __init__(self, strategy):
        self.strategy = strategy

    def minima(self, line):
        return self.strategy.algorithm(line)

    def change_algorithm(self, strategy):
        self.strategy = strategy



if __name__ == '__main__':
    print("Strategy Pattern testing...")

    m = MinimaSolver(LeastSquares())
    print(m.minima([1.1, 1.2, 1.3, 1.4, 1.5]))

    m.change_algorithm(Bisection())
    print(m.minima([1.1, 1.2, 1.3, 1.4, 1.5]))
