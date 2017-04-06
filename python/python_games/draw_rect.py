#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import time
import pygame

from pygame.locals import *

pygame.init()

screen = pygame.display.set_mode((600,500))
myfont = pygame.font.Font(None, 60)


white = 255,255,255
blue = 0,0,255

textImage = myfont.render("Hello Pygame", True, white)
pygame.display.set_caption("Drawing Rectangles")

pos_x = 300
pos_y = 250
vel_x = 2
vel_y = 1

while True:
	for evt in pygame.event.get():
		if evt.type in (QUIT, KEYDOWN):
			sys.exit()

	time.sleep(0.01)

	screen.fill((0, 0, 200))
	screen.blit(textImage, (100, 100))

	pos_x += vel_x
	pos_y += vel_y

	if pos_x > 500 or pos_x < 0:
		vel_x = -vel_x

	if pos_y >400 or pos_y < 0:
		vel_y = -vel_y
	

	color = 255, 255, 0
	width = 0
	pos = pos_x, pos_y, 100, 100

	pygame.draw.rect(screen, color, pos, width)
	pygame.display.update()

