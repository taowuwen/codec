#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import pygame

from pygame.locals import *

pygame.init()

screen = pygame.display.set_mode((600,500))
myfont = pygame.font.Font(None, 60)


white = 255,255,255
blue = 0,0,255

textImage = myfont.render("Hello Pygame", True, white)
pygame.display.set_caption("Drawing Circles")

while True:
	for evt in pygame.event.get():
		if evt.type in (QUIT, KEYDOWN):
			sys.exit()

	screen.fill((0, 0, 200))
#	screen.fill(blue)
	screen.blit(textImage, (100, 100))

	color = 255, 255, 0
	position = 300, 250
	radius = 100
	width = 10

	pygame.draw.circle(screen, color, position, radius, width)
	pygame.display.update()

