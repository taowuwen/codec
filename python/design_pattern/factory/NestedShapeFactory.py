#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from __future__ import generators
import random

class Shape:
    types = []


def factory(ty):

    class Circle(Shape):

        def draw(self):
            print("Draw {} (0)".format(self.__class__.__name__))

        def erase(self):
            print("Erase {} (0)".format(self.__class__.__name__))

    class Square(Shape):
        def draw(self):
            print("Draw {}[=]".format(self.__class__.__name__))

        def erase(self):
            print("Erase {}[]".format(self.__class__.__name__))

    class Triangle(Shape):
        def draw(self):
            print("Draw {}/\\".format(self.__class__.__name__))

        def erase(self):
            print("Erase {}/\\".format(self.__class__.__name__))


    _cls = { "Circle": Circle,
             "Square": Square,
             "Triangle": Triangle
           }.get(ty, None)

    return _cls() if _cls else None


def shape_gen(n):
    for i in range(n):
        yield factory(random.choice(['Circle', 'Square', 'Triangle']))


if __name__ == '__main__':

    for s in shape_gen(7):
        s.draw()
        s.erase()

