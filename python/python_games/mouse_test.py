#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import sys, os, time, pygame, random
from pygame.locals import *

pygame.init()
screen = pygame.display.set_mode((600, 500))
pygame.display.set_caption("Word testing")

font1 = pygame.font.Font(None, 40)
font2 = pygame.font.Font(None, 60)


white = 255, 255, 255
cyan = 0, 255, 255
yellow = 255, 255, 0
purple = 255, 0, 255
green = 0, 255, 0
red = 255, 0, 0

mouse_x = mouse_y = 0
move_x = move_y =0
mouse_down = mouse_up = 0
mouse_down_x = mouse_down_y = 0
mouse_up_x = mouse_up_y = 0


def print_text(font, x, y, text, color=(255, 255, 255), shadow=True):
	if shadow:
		img = font.render(text, True, (0, 0, 0))
		screen.blit(img, (x-2, y-2))
	img = font.render(text, True, color)
	screen.blit(img, (x, y))


while True:
	for evt in pygame.event.get():
		if evt.type == QUIT:
			sys.exit()

		elif evt.type == MOUSEMOTION:
			mouse_x, mouse_y = evt.pos
			move_x,move_y = evt.rel
		elif evt.type == MOUSEBUTTONDOWN:
			mouse_down = evt.button
			mouse_down_x, mouse_down_y = evt.pos
		elif evt.type == MOUSEBUTTONUP:
			mouse_up = evt.button
			mouse_up_x, mouse_up_y = evt.pos

	keys = pygame.key.get_pressed()
	if keys[K_ESCAPE]:
		sys.exit()

	screen.fill((0, 100, 0))

	print_text(font1, 0, 0, "Mouse events")

	print_text(font1, 0, 20, "Mouse position: {}, {}".format(mouse_x, mouse_y))
	print_text(font1, 0, 40, "Mouse Relative: {}, {}".format(move_x, move_y))
	print_text(font1, 0, 60, "Mouse Button Down: {}, {}, {}".format(mouse_down, mouse_down_x, mouse_down_y))
	print_text(font1, 0, 80, "Mouse Button Up: {}, {}, {}".format(mouse_up, mouse_up_x, mouse_up_y))
	print_text(font1, 0, 160, "Mouse Polling")

	x,y = pygame.mouse.get_pos()
	print_text(font1, 0, 180, "Mouse Position: ({}, {})".format(x, y))


	b1, b2, b3 = pygame.mouse.get_pressed()
	print_text(font1, 0, 200, "Mouse buttons: ({}, {}, {})".format(b1, b2, b3))

	pygame.display.update()
	


