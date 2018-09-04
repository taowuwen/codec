#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Kitty:
    def interactWith(self, obstacle):
        print("Kitty has encountered a {}".format(obstacle.action()))

class KungFuGuy:
    def interactWith(self, obstacle):
        print("KungFuGuy now battles a {}".format(obstacle.action()))


class Puzzle:
    def action(self):
        return "Puzzle"

class NastyWeapon:
    def action(self):
        return "NastyWeapon"

class KittiesAndPuzzles:
    def makeCharacter(self):
        return Kitty()

    def makeObstacle(self):
        return Puzzle()

class KillAndDismember:
    def makeCharacter(self):
        return KungFuGuy()

    def makeObstacle(self):
        return NastyWeapon()


class GameEnvironment:
    def __init__(self, factory):
        self.factory = factory

        self.p = factory.makeCharacter()
        self.ob = factory.makeObstacle()


    def play(self):
        self.p.interactWith(self.ob)


if __name__ == '__main__':

    print("test game2")

    g1 = GameEnvironment(KittiesAndPuzzles())
    g2 = GameEnvironment(KillAndDismember())

    g1.play()
    g2.play()
