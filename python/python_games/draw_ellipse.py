#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import time
import pygame
import math

from pygame.locals import *


pygame.init()
screen = pygame.display.set_mode((600, 500))
pygame.display.set_caption("Draw Arcs")


def draw_arc():
	color = 255, 0, 255
	width = 8
	pos = 200, 150, 200, 100

	start_angle = math.radians(0)
	end_angle = math.radians(150)

	pygame.draw.rect(screen, (100, 100, 100), pos, width)
	pygame.draw.ellipse(screen, color, pos, width)

while True:
	for evt in pygame.event.get():
		if evt.type in (QUIT, KEYDOWN):
			sys.exit()

	screen.fill((0, 0, 120))

	draw_arc()

	pygame.display.update()




