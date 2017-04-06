#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import sys
import os
import time
import pygame
import math
from pygame.locals import *

pygame.init()

screen = pygame.display.set_mode((600, 500))

pygame.display.set_caption("The Pie Game - Press 1, 2, 3,4")

g_font = pygame.font.Font(None, 60)

color = 200, 80, 60
width = 4

center_x = 300
center_y = 250
radius = 200

start_pos = center_x - radius, center_y - radius, radius * 2, radius * 2

def get_radians(start, end):
	return (math.radians(start), math.radians(end))


class CPie():
	"""
	draw one pie
	give the pos, tips and color
	pos(x, y, r, width)

	"""
	def __init__(self, 
			font=g_font, 
			pos1=(100, 100), 
			pos2=(200, 200),
			arc=get_radians(0, 90),
			width=5, 
			tips="unset", 
			color=(100, 100, 100)):
		self._show = 0
		self._x1, self._y1 = pos1
		self._x2, self._y2 = pos2
		self._s, self._e = arc
		self._width = width
		self._color = color
		self._tips = tips
		self._font = font
		self._text = self._font.render(self._tips, True, color)

	def draw_tips(self, screen):
		screen.blit(self._text,( (self._x1 + self._x2) / 2, (self._y1 + self._y2) / 2) )

	def draw_pie(self, screen):
		"""
		two line + one arc
		"""
		pygame.draw.line(screen, self._color, (self._x1, self._y1), (center_x, center_y), self._width)
		pygame.draw.line(screen, self._color, (self._x2, self._y2), (center_x, center_y), self._width)
		pygame.draw.arc( screen, self._color, start_pos, self._s, self._e, self._width)

	
	@property
	def show(self):
		return self._show

	@show.setter
	def show(self, show):
		self._show = show

	def set_color(self, color=(0, 255, 0)):
		self._color = color
		self._text = self._font.render(self._tips, True, color)

pie1 = CPie(
	pos1=(center_x, center_y - radius),
	pos2=(center_x - radius, center_y),
	arc=get_radians(90, 180),
	tips="1")

pie2 = CPie(
	pos1=(center_x, center_y - radius),
	pos2=(center_x + radius, center_y),
	arc=get_radians(0, 90),
	tips="2")

pie3 = CPie(
	pos1=(center_x - radius, center_y),
	pos2=(center_x, center_y + radius),
	arc=get_radians(180, 270),
	tips="3")

pie4 = CPie(
	pos1=(center_x, center_y + radius),
	pos2=(center_x + radius, center_y),
	arc=get_radians(270, 360),
	tips="4")


piegame = [pie1, pie2, pie3, pie4]
game_over = False


while True:
	for evt in pygame.event.get():
		if game_over:
			if evt.type in (QUIT, KEYDOWN):
				sys.exit()

			continue

		if evt.type == QUIT:
			sys.exit()

		elif evt.type == KEYUP:
			if evt.key in (pygame.K_ESCAPE, pygame.K_RETURN):
				sys.exit()
			elif evt.key == pygame.K_1:
				piegame[0].show = 1
			elif evt.key == pygame.K_2:
				piegame[1].show = 1
			elif evt.key == pygame.K_3:
				piegame[2].show = 1
			elif evt.key == pygame.K_4:
				piegame[3].show = 1
			else:
				pass


	screen.fill((0, 0, 120))

	if not game_over:
		all_in = 0

		for shift_n, pie in enumerate(piegame, 0):
			pie.draw_tips(screen)

			if pie.show:
				all_in |= (1 << shift_n)
				pie.draw_pie(screen)

		if all_in == 0x0f:
			game_over = True
			for pie in piegame:
				pie.set_color(color=(0, 255, 0))
	else:
		for pie in piegame:
			pie.draw_tips(screen)
			pie.draw_pie(screen)

	pygame.display.update()

