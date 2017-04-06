#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import time
import pygame

from pygame.locals import *


pygame.init()
screen = pygame.display.set_mode((600, 500))
pygame.display.set_caption("Draw lines")


def draw_lines():
	color = 100, 0, 0
	width = 8

	print("color" + str(color))
	pygame.draw.line(screen, color, (100, 100), (500, 400), width)

while True:
	for evt in pygame.event.get():
		if evt.type in (QUIT, KEYDOWN):
			sys.exit()

	screen.fill((0, 0, 120))

	draw_lines()

	pygame.display.update()




