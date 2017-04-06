#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import time
import pygame
import math

from pygame.locals import *


pygame.init()
screen = pygame.display.set_mode((600, 500))
pygame.display.set_caption("Draw Arcs")


print("sys.version", sys.version)
print("sys.copyright", sys.copyright)


def draw_arc():
	color = 255, 0, 255
	width = 8
	pos = 200, 150, 200, 200

	start_angle = math.radians(0)
	end_angle = math.radians(150)

	pygame.draw.rect(screen, (100, 100, 100), pos, width)
	pygame.draw.arc(screen, color, pos, start_angle, end_angle, width)
	pygame.draw.line(screen, (100, 200, 0), (200, 250), (400, 250), width)

while True:
	for evt in pygame.event.get():
		if evt.type in (QUIT, KEYDOWN):
			sys.exit()

	screen.fill((0, 0, 120))

	draw_arc()

	pygame.display.update()




