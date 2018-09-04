#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import random

class Shape:

    def draw(self):
        print("draw ---> {}".format(self.__class__.__name__))

    def erase(self):
        print("Erase >>>>>> {}".format(self.__class__.__name__))


class ThinkCircle(Shape): pass
class ThinkSquare(Shape): pass
class ThinkTriangle(Shape): pass

class ThinCircle(Shape): pass
class ThinSquare(Shape): pass
class ThinTriangle(Shape): pass

class ThinkShapeFactory:
    def makeCircle(self):
        return ThinkCircle()

    def makeSquare(self):
        return ThinkSquare()

    def makeTriangle(self):
        return ThinkTriangle()


class ThinShapeFactory:
    def makeCircle(self):
        return ThinCircle()

    def makeSquare(self):
        return ThinSquare()

    def makeTriangle(self):
        return ThinTriangle()


class ShapeFactory:
    def __init__(self, factory):
        self.factory = factory

    def __getattr__(self, val):
        return getattr(self.factory, val)


if __name__ == '__main__':

    sf1 = ShapeFactory(ThinkShapeFactory())
    sf2 = ShapeFactory(ThinShapeFactory())

    for sf in [ sf1, sf2]:
        for s in [ sf.makeSquare(), sf.makeCircle(), sf.makeTriangle() ]:
            s.draw()
            s.erase()


        print("-----------------------")
