#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import random

class ShapeFactory:

    factories = {}

    @staticmethod
    def add_factory(_id, shape_factory):
        ShapeFactory.factories[_id] = shape_factory

    @staticmethod
    def create_shape(_id):

        if not _id in ShapeFactory.factories:
            #ShapeFactory.factories[_id] = eval( _id + ".Factory()")
            ShapeFactory.add_factory(_id, eval( _id + ".Factory()"))

        return ShapeFactory.factories[_id].create()


class Shape:

    def draw(self):
        print("draw ---> {}".format(self.__class__.__name__))

    def erase(self):
        print("Erase >>>>>> {}".format(self.__class__.__name__))


class Circle(Shape):
    class Factory:
        def create(self):
            return Circle()

class Square(Shape):
    class Factory:
        def create(self):
            return Square()


def shape_gen_name(n):

    names = Shape.__subclasses__()

    for i in range(n):
        yield random.choice(names).__name__


if __name__ == '__main__':

    shps = [ ShapeFactory.create_shape(n) for n in shape_gen_name(7) ]
    for shp in shps:
        shp.draw()
        shp.erase()
