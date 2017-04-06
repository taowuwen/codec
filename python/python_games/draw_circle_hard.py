#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys, math, random
import pygame

from pygame.locals import *

pygame.init()

screen = pygame.display.set_mode((600,500))
myfont = pygame.font.Font(None, 60)


pos_x = 300
pos_y = 250

color = 255,255,255
blue = 0,0,255

pygame.display.set_caption("Drawing Circles")

radius = 200
angle = 360

screen.fill((0, 0, 100))

while True:
	for evt in pygame.event.get():
		if evt.type == QUIT:
			sys.exit()
	
	keys = pygame.key.get_pressed()
	if keys[K_ESCAPE]:
		sys.exit()

	
	angle += 1
	if angle >= 360:
		angle = 0
		color = (random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))


	x = math.cos( math.radians(angle) ) * radius
	y = math.sin( math.radians(angle) ) * radius

	pos = ( int(pos_x + x), int(pos_y + y) )

	pygame.draw.circle(screen, color, pos, 10, 0)
	pygame.display.update()

