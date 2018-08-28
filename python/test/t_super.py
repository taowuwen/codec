#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Mammal:
    def __init__(self, arg):
        print(arg, "Class for Mammal")

class Dog(Mammal):

    def __init__(self, arg):
        print("Class for Dog: {}".format(arg))
        super().__init__(arg)
        print("END for DOG")



class Animal:
    def __init__(self, arg):
        print(arg, "Init in Animal")


class Mammal1(Animal):
    def __init__(self, arg):
        print(arg, "Init in Mammal1...")
        print("Mammal1 END1")
        super().__init__(arg)
        print("Mammal1 END")

class NonWingedMammal(Mammal1):
    def __init__(self, arg):
        print(arg, " can't fly")
        print("NonWingedMammal END1")
        super().__init__(arg)
        print("NonWingedMammal END")


class NonMarrineMammal(Mammal1):
    def __init__(self, arg):
        print(arg, " can't swim")
        print("NonMarrineMammal END1")
        super().__init__(arg)
        print("NonMarrineMammal END")


class Dog1(NonWingedMammal, NonMarrineMammal):
    def __init__(self, arg):
        print("Dog has 4 legs")
        super().__init__(arg)
        print("Dog END")








if __name__ == '__main__':

    print("test super")

    d = Dog('DDDDD')
    d = Dog1('AAA')

    for r in Dog1.__mro__:
        print(r)

