#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class MouseAction:

    def __init__(self, action):
        self.action = action

    def __str__(self):
        return self.action

    def __eq__(self, other):
        return self.action == other.action

    def __hash__(self):
        return hash(self.action)



MouseAction.appears = MouseAction("mouse appears")
MouseAction.run_away = MouseAction("mouse run away")
MouseAction.enters = MouseAction("mouse enters trap")
MouseAction.escaps = MouseAction("mouse escaps")
MouseAction.trapped = MouseAction("mouse trapped")
MouseAction.removed = MouseAction("mouse removed")



if __name__ == '__main__':

    a = MouseAction("mouse appears")

    print(a)
    print(MouseAction.appears)
    print(a == MouseAction.appears)

    print(id(a), hash(a))
    print(id(MouseAction.appears), hash(MouseAction.appears))

