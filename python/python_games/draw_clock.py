#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys, math, random
from datetime import datetime, date, time
import pygame

from pygame.locals import *

pygame.init()

screen = pygame.display.set_mode((600,500))
font1 = pygame.font.Font(None, 30)


pos_x = 300
pos_y = 250

white = 255,255,255
blue = 0,0,255
black = 0,0,0
red = 255, 0, 0
red1 = 128, 0, 0

c_hour = 0, 210, 230
c_min  = 150, 0, 133
c_sec  = 100, 111, 0


pygame.display.set_caption("Drawing Circles")

radius = 200
angle = 360

def print_text(font, x, y, text, color=(255, 255, 255)):
	screen.blit(font.render(text, True, color), (x, y))

def draw_numbers():
	for degress in range(30, 361, 30):

		angle = math.radians(degress - 90)
		x = math.cos( angle ) * (radius - 20) - 10
		y = math.sin( angle ) * (radius - 20) - 10

		print_text(font1, x + pos_x, pos_y + y, str(degress//30), red1)


def draw_time():
	today = datetime.today()

	# draw hour
	hours = today.hour % 12
	angle = math.radians( hours * 360 / 12 - 90)

	x = math.cos(angle) * (radius - 80)
	y = math.sin(angle) * (radius - 80)

	pos = (pos_x + x, pos_y + y)

	pygame.draw.line(screen, c_hour, (pos_x, pos_y), pos, 25)


	# draw minute
	minute = today.minute

	angle = math.radians ( minute * 360 / 60 - 90)
	x = math.cos(angle) * (radius - 60)
	y = math.sin(angle) * (radius - 60)

	pos = (pos_x + x, pos_y + y)

	pygame.draw.line(screen, c_min, (pos_x, pos_y), pos, 15)

	# draw second
	second = today.second

	angle = math.radians ( second * 360 / 60 - 90)
	x = math.cos(angle) * (radius - 40)
	y = math.sin(angle) * (radius - 40)

	pos = (pos_x + x, pos_y + y)

	pygame.draw.line(screen, c_sec, (pos_x, pos_y), pos, 6)


	print_text(font1, 0, 0, "{0:02d}:{1:02d}:{2:02d}".format(hours, minute, second))



def draw_center():
	pygame.draw.circle(screen, white, (pos_x, pos_y), 20)




#def draw_numbers():
#
#	angle = 420
#	step  = -30
#
#	for num in range(1, 13):
#
#		x = math.cos( math.radians(angle) ) * (radius - 20) - 10
#		y = math.sin( math.radians(angle) ) * (radius - 20) + 10
#
#		angle += step
#
#		print_text(font1, x + pos_x, pos_y - y, str(num), red1)

#def draw_numbers():
#	for n in range(1, 13):
#		angle = math.radians( n * (360/ 12) - 90)
#		x = math.cos( angle ) * (radius - 20) - 10
#		y = math.sin( angle ) * (radius - 20) - 10
#		print_text(font1, x + pos_x, y + pos_y, str(n), red1)




while True:
	for evt in pygame.event.get():
		if evt.type == QUIT:
			sys.exit()
	
	keys = pygame.key.get_pressed()
	if keys[K_ESCAPE]:
		sys.exit()

	
	screen.fill(black)

#angle += 1
#if angle >= 360:
#	angle = 0
#	color = (random.randint(0, 255), random.randint(0, 255), random.randint(0, 255))
#
#
#x = math.cos( math.radians(angle) ) * radius
#y = math.sin( math.radians(angle) ) * radius

	pygame.draw.circle(screen, red, (pos_x, pos_y), radius, 6)

	draw_numbers()
	draw_time()
	draw_center()

	pygame.display.update()

