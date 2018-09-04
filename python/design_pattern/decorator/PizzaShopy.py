#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class PizzaComponent:

    def get_desc(self):
        return self.__class__.__name__

    def get_total(self):
        return self.__class__.cost


class Margherita(PizzaComponent):
    cost = 12.0

class Hawaiian(PizzaComponent):
    cost = 11.0

class Regina(PizzaComponent):
    cost = 11.5

class Vegetarian(PizzaComponent):
    cost = 11.7


class PizzaDecorator(PizzaComponent):

    def __init__(self, component):
        self.component = component

    def get_total(self):
        return self.component.get_total() + super().get_total()

    def get_desc(self):
        return self.component.get_desc() + ' ' + super().get_desc()

class Spinach(PizzaDecorator):
    cost = 0.5

class Feta(PizzaDecorator):
    cost = 1.0

class Pepperdews(PizzaDecorator):
    cost = 1.5

class Olives(PizzaDecorator):
    cost = 0.75


a = Hawaiian()
print("{}, total: {}".format(a.get_desc(), a.get_total()))

a = Spinach(Feta(Pepperdews(Olives(Margherita()))))
print("{}, total: {}".format(a.get_desc(), a.get_total()))
