#!/usr/bin/env python3
# -*- coding:utf-8 -*-


class DrinkComponent:
    def get_desc(self):
        return self.__class__.__name__

    def get_total(self):
        return self.__class__.cost

class Mug(DrinkComponent):
    cost = 0.0


class Decortor(DrinkComponent):
    
    def __init__(self, drink_component):

        self.component = drink_component

    def get_desc(self):
        return self.component.get_desc() + ' ' + super().get_desc()

    def get_total(self):
        return self.component.get_total() + super().get_total()


class Espresso(Decortor):
    cost = 0.75


class Decaf(Decortor):
    cost = 0.0

class FoamedMild(Decortor):
    cost = 0.25


class StreamedMilk(Decortor):
    cost = 0.25

class Whipped(Decortor):
    cost = 0.25

class Chocolate(Decortor):
    cost = 0.25


a = Espresso(Mug())
print("{}, total: {}".format(a.get_desc(), a.get_total()))

a = Espresso(FoamedMild(Mug()))
print("{}, total: {}".format(a.get_desc(), a.get_total()))

a = Espresso(FoamedMild(Decaf(Mug())))
print("{}, total: {}".format(a.get_desc(), a.get_total()))



a = Espresso(StreamedMilk(Chocolate(FoamedMild(Decaf(Mug())))))
print("{}, total: {}".format(a.get_desc(), a.get_total()))
