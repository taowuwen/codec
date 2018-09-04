#!/usr/bin/env python3
# -*- coding: utf-8 -*-



class DrinkComponent:

    def get_desc(self):
        return self.__class__.__name__

    def get_total(self):
        return self.__class__.cost


class Espresso(DrinkComponent):
    cost = 0.75

class CafeMocha(DrinkComponent):
    cost = 1.25

class CafeLatte(DrinkComponent):
    cost = 1.0

class Cuppuccino(DrinkComponent):
    cost = 1.0

class EspressoConPanna(DrinkComponent):
    cost = 1.0


class Decorator(DrinkComponent):

    def __init__(self, drink_component):
        self.component = drink_component


    def get_desc(self):

        return self.component.get_desc() + ' ' + super().get_desc()

    def get_total(self):
        return self.component.get_total() + super().get_total()


class ExtraEspresso(Decorator):
    cost = 0.75

class Whipped(Decorator):
    cost = 0.25

class Decaf(Decorator):
    cost = 0

class Dry(Decorator):
    cost = 0.25

class Wet(Decorator):
    cost = 0.25

class Syrup(Decorator):
    cost = 0.5

class StreamedMilk(Decorator):
    cost = 0.3

class FoamedMilk(Decorator):
    cost = 0.3


a = Cuppuccino()
print("{} , total: ${}".format(a.get_desc(), a.get_total()))

a = Decaf(Dry(Cuppuccino()))
print("{} , total: ${}".format(a.get_desc(), a.get_total()))

a = Whipped(Decaf(CafeMocha()))
print("{} , total: ${}".format(a.get_desc(), a.get_total()))

a = StreamedMilk(Syrup(Decaf(CafeMocha())))
print("{} , total: ${}".format(a.get_desc(), a.get_total()))

a = FoamedMilk(Syrup(Decaf(CafeMocha())))
print("{} , total: ${}".format(a.get_desc(), a.get_total()))
