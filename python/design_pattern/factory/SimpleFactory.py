#!/usr/bin/env python3
# -*- coding: utf-8 -*-


from __future__ import generators
import random

class Shape:

    @staticmethod
    def factory(shape):

        _cls = {
                "Circle": Circle,
                "Square": Square,
                "Triangle": Triangle,
        }.get(shape, None)

        return _cls() if _cls else None


    def draw(self):
        print("Draw {}".format(self.__class__.__name__))

    def erase(self):
        print("Erase {}".format(self.__class__.__name__))


class Circle(Shape): pass
class Square(Shape): pass
class Triangle(Shape): pass


def gen_shape(n):

    _typs = Shape.__subclasses__()

    for i in range(n):
        yield random.choice(_typs).__name__ 

shps = [Shape.factory(i) for i in gen_shape(7)]

for shp in shps:
    shp.draw()
    shp.erase()
