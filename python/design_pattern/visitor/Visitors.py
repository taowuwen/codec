#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import random

class Flower:

    def accept(self, visitor):
        return visitor.visit(self)

    def pollinate(self, pollinate):
        print(" {} pollinate by {}".format(self, pollinate))

    def eat(self, eater):
        print(" {} eatten by {}".format(self, eater))

    def __str__(self):
        return self.__class__.__name__

class Gladiolus(Flower): pass
class Runuculus(Flower): pass
class Chrysanmthemum(Flower): pass


class Visitor:
    def __str__(self):
        return self.__class__.__name__

    def visit(self):
        pass

class Bus(Visitor):pass
class Pollinator(Visitor):pass
class Predator(Visitor): pass

class Bee(Pollinator):

    def visit(self, flower):
        flower.pollinate(self)

class Fly(Pollinator):
    def visit(self, flower):
        flower.pollinate(self)

class Worm(Predator):
    def visit(self, flower):
        flower.eat(self)

def flowerGen(n):

    flowers = Flower.__subclasses__()
    
    for i in range(n):
        yield random.choice(flowers)()

if __name__ == '__main__':
    print("testing for visitors...")


    bee = Bee()
    fly = Fly()
    wrm = Worm()

    for f in flowerGen(10):
        f.accept(bee)
        f.accept(fly)
        f.accept(wrm)

