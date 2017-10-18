#!/usr/bin/env python3
# -*- coding: utf-8 -*-




def main():
	
	vagetables = [
		"cauliflower",
		"broccoli",
		"cabbage",
		"brussels sprouts",
		"watercress",
		"lettuce",
		"escarole",
		"spinach",
		"herb(s)",
		"celery",
		"artichoke",
		"(ear of) corn & cob",
		"kidney bean(s)",
		"black bean(s)",
		"string bean(s)",
		"lima bean(s)",
		"pea & pod",
		"asparagus"
	]


	chinese = [
		u"花菜",
		u"兰花菜",
		u"白菜",
		u"菜头",
		u"西洋菜",
		u"生菜",
		u"莴苣菜",
		u"波菜",
		u"药草",
		u"芹菜",
		u"朝鲜YU，洋姜",
		u"玉米（壳）& 玉米棒",
		u"猫腰豆",
		u"黑豆",
		u"四季豆",
		u"蚕豆",
		u"豌豆 & 豆角",
		u"芦笋"
	]


	for ind, vag in enumerate(vagetables):
		print(u"{0:>10d} {1:>20s}".format(ind+1, vag))

	for ind, vag in enumerate(chinese):
		print(u"{0:>10d} {1:\u3000>20} --> {2:>10}".format(ind+1, vag, len(vag)))


	for ind, vag in enumerate(zip(vagetables, chinese)):
		print(u"{0:>10d} {1:>20s} <-> {2:<20s}".format(ind+1, vag[0], vag[1]))

	for ind, vag in enumerate(zip(chinese, vagetables)):
		print(u"{0:>10d} {1:\u3000>20s} <-> {2:<20s}".format(ind+1, vag[0], vag[1]))


if __name__ == '__main__':
	main()

