#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import time
import pygame
import random

from pygame.locals import *


random.seed()

pygame.init()
screen = pygame.display.set_mode((600, 500))
pygame.display.set_caption("Draw lines")

font = pygame.font.Font(None, 60)


def draw_lines(fixed=False):
	width = 8

	if fixed:
		color = 255, 255, 0
		pygame.draw.line(screen, color, (100, 100), (300, 300), width)
		return None

	color = random.randint(0, 255), random.randint(0,255), random.randint(0, 10)

	x1 = random.randint(0, 600)
	y1 = random.randint(0, 500)

	x2 = random.randint(0, 600)
	y2 = random.randint(0, 500)


	pygame.draw.line(screen, color, (x1, y1), (x2, y2), width)

line = 0
while True:
	for evt in pygame.event.get():
		if evt.type in (QUIT, KEYDOWN):
			sys.exit()

	screen.fill((0, 0, 120))

	if line <= 1000:
		draw_lines()
		line += 1
		screen.blit(
			font.render("line: {0}".format(line), True, (255, 255, 0)),
			(50, 50)
			)

		time.sleep(0.01)
	else:
		draw_lines(True)



	pygame.display.update()

