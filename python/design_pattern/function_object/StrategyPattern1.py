#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Message: pass

class Result:

    def __init__(self):
        self.succeeded = 0

    def isSuccessful(self):
        return self.succeeded

    def setSuccessful(self, success):
        self.succeeded = success


class Strategy:
    def __call__(self, message):
        pass

    def __str__(self):
        return "Trying " + self.__class__.__name__ + "algorithm"


class ChainLink:
    def __init__(self, chain, strategy):
        self.strategy = strategy
        self.chain = chain
        self.chain.append(self)

    def next(self):

        location = self.chain.index(self)

        if not self.end():
            return self.chain[location + 1]

    def end(self):
        return (self.chain.index(self) + 1 >= len(self.chain))

    def __call__(self, messenger):

        r = self.strategy(messenger)

        if r.isSuccessful() or self.end(): 
            return r

        return self.next()(messenger)


class LineData(Result, Message):
    def __init__(self, data):
        self.data = data
        super().__init__()

    def __str__(self):
        return str(self.data)


class LeastSquares(Strategy):

    def __call__(self, messenger):
        print(self)
        linedata = messenger

        result = LineData([1.1, 2.2])
        result.setSuccessful(0)
        return result


class NewtonsMethod(Strategy):

    def __call__(self, messenger):
        print(self)
        linedata = messenger

        result = LineData([3.3, 4.4])
        result.setSuccessful(0)
        return result


class Bisection(Strategy):
    def __call__(self, messenger):
        print(self)

        linedata = messenger

        result = LineData([5.5, 6.6])
        result.setSuccessful(1)
        return result

class ConjugateGradient(Strategy):
    def __call__(self, messenger):
        print(self)

        linedata = messenger

        result = LineData([7.7, 8.8])
        result.setSuccessful(1)
        return result


if __name__ == '__main__':
    print("testing for strategy pattern")

    solutions = []

    ChainLink(solutions, LeastSquares())
    ChainLink(solutions, NewtonsMethod())
    ChainLink(solutions, Bisection())
    ChainLink(solutions, ConjugateGradient())

    line = LineData([1.1, 2.0, 3.1, 4.0, 5.4, 6.0])

    print(solutions[0](line))
