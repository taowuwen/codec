#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Ctest():
	def __init__(self, tips):
		self._tips = tips


	def show_all(self):
		self.show_tips()

	def show_tips(self):
		print("self tips: " + self._tips)

t = Ctest("hello")

t.show_all()




