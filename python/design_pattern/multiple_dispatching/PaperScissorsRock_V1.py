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
    def compete(self, other):
        return compares[self.__class__, other.__class__]

    def __str__(self):
        return self.__class__.__name__


class Rock(Item): pass
class Paper(Item): pass
class Scissor(Item): pass

compares = {
        (Rock, Rock): Result.DRAW,
        (Rock, Paper): Result.LOSE,
        (Rock, Scissor): Result.WIN,
        (Paper, Paper): Result.DRAW,
        (Paper, Rock): Result.WIN,
        (Paper, Scissor): Result.LOSE,
        (Scissor, Scissor): Result.DRAW,
        (Scissor, Paper): Result.WIN,
        (Scissor, Rock): Result.LOSE
    }


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


