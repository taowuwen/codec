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


def print_text(font, x, y, text, color=(255, 255, 255), shadow=True):
	if shadow:
		img = font.render(text, True, (0, 0, 0))
		screen.blit(img, (x-2, y-2))
	img = font.render(text, True, color)
	screen.blit(img, (x, y))




key_flag = False
correct_answer = 97 # 'a'
seconds = 11
score = 0
clock_start = 0
game_over = True

while True:
	for evt in pygame.event.get():
		if evt.type == QUIT:
			sys.exit()
		elif evt.type == KEYDOWN:
			key_flag = True
		elif evt.type == KEYUP:
			key_flag = False

	keys = pygame.key.get_pressed()
	if keys[K_ESCAPE]:
		sys.exit()

	if keys[K_RETURN]:
		if game_over:
			game_over = False
			score = 0
			seconds = 11
			clock_start = time.clock()

	current = time.clock() - clock_start
	speed = score * 6

	if seconds - current < 0:
		game_over = True

	elif current <= 10:
		if keys[correct_answer]:
			correct_answer = random.randint(97, 122)
			score += 1

	screen.fill((0, 100, 0))
	print_text(font1, 0, 0, "Let's see how fast you can type!")
	print_text(font1, 0, 20, "Try to keep up for 10 seconds...")


	if key_flag:
		print_text(font1, 500, 0, "<keydown>")

	if not game_over:
		print_text(font1, 0, 80, "Time: " + str(int(seconds - current)))

	print_text(font1, 0, 100, "Speed: " + str(speed) + " letters/min")


	if game_over:
		print_text(font1, 0, 160, "Press Enter to start...")

	print_text(font2, 250, 240, chr(correct_answer - 32), yellow)

	pygame.display.update()
	


