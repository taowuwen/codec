#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import random


class Result:

    def __init__(self, name, val):
        self.name = name
        self.val = val

    def __str__(self):
        return self.name

    def __eq__(self, other):
        return self.val == other.val

Result.WIN  = Result("Win",  0)
Result.LOSE = Result("Lose", 1)
Result.DRAW = Result("Draw", 2)

class Item:
    def __str__(self):
        return self.__class__.__name__


class Rock(Item):

    def compete(self, item):
        return item.evalRock(self)

    def evalRock(self, other):
        return Result.DRAW

    def evalPaper(self, item):
        return Result.WIN

    def evalScissor(self, item):
        return Result.LOSE


class Paper(Item):
    def compete(self, item):
        return item.evalPaper(self)

    def evalPaper(self, other):
        return Result.DRAW

    def evalRock(self, other):
        return Result.LOSE

    def evalScissor(self, item):
        return Result.WIN

class Scissor(Item):
    def compete(self, item):
        return item.evalScissor(self)

    def evalPaper(self, other):
        return Result.LOSE

    def evalRock(self, other):
        return Result.WIN

    def evalScissor(self, item):
        return Result.DRAW


def match(item1, item2):
    print("{:>10} <---> {:<10} : {:^10}".format(str(item1), str(item2), str(item1.compete(item2))))


def gen_items_pairs(n):

    _cls = Item.__subclasses__()

    for i in range(n):

        yield ( random.choice(_cls)(), random.choice(_cls)())


if __name__ == '__main__':
    print("testing paper scissor rock game...")

    for item1, item2 in gen_items_pairs(20):
        match(item1, item2)


