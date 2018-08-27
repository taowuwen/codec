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


if __name__ == '__main__':

    print("test super")

    d = Dog('DDDDD')

    
