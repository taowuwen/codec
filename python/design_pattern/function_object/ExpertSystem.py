#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Result:

    def __init__(self):
        self.success = 0

    def is_succeessful(self):
        return self.success

    def set_successful(self, success):
        self.success = success


class SolverChain:

    def __init__(self, chain, solver):
        self.solver = solver
        self.chain = chain
        self.chain.append(self)

    def next(self):

        if not self.end():
            pos = self.chain.index(self)
            return self.chain[pos + 1]

        return None

    def end(self):
        return (self.chain.index(self) + 1 >= len(self.chain))

    def __call__(self, msg):

        r = self.solver(msg)

        if r.is_succeessful():
            return r

        if self.end():
            return None

        return self.next()(msg)


class Message(Result):
    def __init__(self, msg):
        self.msg = msg
        super().__init__()

    def __str__(self):
        return str(self.msg)


class Solver:
    def __str__(self):
        return "Trying {}...".format(self.__class__.__name__)

    def __call__(self, msg):
        pass


class KeySolver(Solver):
    def __call__(self, msg):
        print(self)

        r = Message(msg)

        if "key" in msg:
            r.set_successful(1)

        return r

class ValueSolver(Solver):
    def __call__(self, msg):
        print(self)

        r = Message(msg)
        if "value" in msg:
            r.set_successful(1)

        return r

class HelloSolver(Solver):
    def __call__(self, msg):
        print(self)

        r = Message(msg)
        if "hello" in msg:
            r.set_successful(1)

        return r

class WorldSolver(Solver):
    def __call__(self, msg):
        print(self)

        r = Message(msg)
        if "world" in msg:
            r.set_successful(1)

        return r



if __name__ == '__main__':
    print("testing for expert system...")

    solvers = []
    SolverChain(solvers, KeySolver())
    SolverChain(solvers, ValueSolver())
    SolverChain(solvers, HelloSolver())
    SolverChain(solvers, WorldSolver())

    print(solvers[0]("ok, it's my world"))

