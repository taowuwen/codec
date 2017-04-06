#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import random, math, pygame, sys, time
from pygame.locals import *


pygame.init()

screen = pygame.display.set_mode((1024, 650))
pygame.display.set_caption("Orbit Demo")
pygame.key.set_repeat(100)

font1 = pygame.font.Font(None, 40)
font2 = pygame.font.Font(None, 24)


white = 255, 255, 255
cyan = 0, 255, 255
yellow = 255, 255, 0
purple = 255, 0, 255
green = 0, 255, 0
red = 255, 0, 0

def print_text(font, x, y, text, color=(255, 255, 255)):
	screen.blit(font.render(text, True, color), (x, y))



class Point():
	def __init__(self, x, y):
		self.__x = x
		self.__y = y

	def getx(self):
		return self.__x

	def setx(self, x):
		self.__x = x

	x = property(getx, setx)

	@property
	def y(self):
		return self.__y

	@y.setter
	def y(self, y):
		self.__y = y
	
	def __str__(self):
		return "{" +  "X: {:.0f} , Y: {:.0f}".format(self.__x, self.__y) +  "}"



# background
#space = pygame.image.load("sources/9.jpg").convert()
space = pygame.image.load("sources/32.jpg").convert()
space = pygame.transform.smoothscale(space, (800, 500))

# planet
#planet = pygame.image.load("sources/OceanPlanet2.jpg").convert_alpha()
#planet = pygame.transform.smoothscale(planet, (800, 500))

# spaceship

spaceship = pygame.image.load("sources/spaceship_02.png").convert_alpha()
w, h = spaceship.get_size()
spaceship = pygame.transform.smoothscale(spaceship, (w // 4, h // 4))


pos = Point(0, 0)
old_pos = Point(0, 0)


angle = 0
radius = 250

step = 0.1

def wrap_angle(angle):
	return (angle % 360)


def  move_ship():
	global angle

	angle = wrap_angle(angle - step)
	pos.x = math.sin( math.radians(angle) ) * radius
	pos.y = math.cos( math.radians(angle) ) * radius

	delta_x = (pos.x - old_pos.x)
	delta_y = (pos.y - old_pos.y)

	rangle = math.atan2(delta_y, delta_x)
	rangled = wrap_angle( -math.degrees(rangle))
	scratch_ship = pygame.transform.rotate(spaceship, rangled)


	w, h = scratch_ship.get_size()
	x = 512 + pos.x - w // 4
	y = 325 + pos.y - h // 4

	screen.blit(scratch_ship, (x, y))


while True:
	for evt in pygame.event.get():
		if evt.type == QUIT:
			sys.exit()
		elif evt.type == KEYDOWN:
			if evt.key == pygame.K_UP:
				step += 0.1
				if step > 1:
					step = 1
			elif evt.key == pygame.K_DOWN:
				step -= 0.1
				if step < 0.1:
					step = 0.1

	keys = pygame.key.get_pressed()
	if keys[K_ESCAPE]:
		sys.exit()

	screen.fill((0,0,0))


	screen.blit(space, (512-400, 325 - 250))

	move_ship()

	print_text(font1, 0, 0, "Angle: {:0.01f}".format(angle))
	print_text(font2, 0, 40, "X: {:0.01f}, Y: {:0.01f}".format(old_pos.x, old_pos.y), cyan)
	print_text(font2, 200, 40, "X: {:0.01f}, Y: {:0.01f}".format(pos.x, pos.y), cyan)

	print_text(font1, 0, 80, "Current speed: {:0.0f}X".format(step * 10))

#	time.sleep(0.1)

	old_pos.x = pos.x
	old_pos.y = pos.y

	pygame.display.update()
