#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Hear:
    def __init__(self, color='black', **kwargs):
        print("\tHear {}".format(color))
        super().__init__(**kwargs)

    def color(self):
        print("color BLACK")

class Nose:
    def __init__(self, shape='high', **kwargs):
        print("\tNose {}".format(shape))
        super().__init__(**kwargs)

    def color(self):
        print("color YELLOW")


class Head(Hear, Nose):
    def __init__(self, **kwargs):
        print("Head: ")
        super().__init__(**kwargs)

class Body:
    def __init__(self, **kwargs):
        print("Body")

    def color(self):
        print("COLOR RED")

class Person(Head, Body):
    def __init__(self, **kwargs):
        print("Person Info: {}".format(kwargs))
        super().__init__(**kwargs)


if __name__ == '__main__':
    print("test for Person")

    import pprint
    pprint.pprint(Person.__mro__)

    p = Person(shape='low', color='blue')
    p.color()

