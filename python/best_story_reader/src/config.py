#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from BSRLogger import *

import os
import json
import sys
from collections import defaultdict


# config's in many files.
class BSRConfig(defaultdict):
	"""
	BSR config loading, 
		file format in json 

	file: config file name
	path: load all configs in that path

	"""
	def __init__(self, *args, default=dict, file=None, path="/tmp/bsr/config"):
		super().__init__(default, *args)

		self.config_path = path

		log_debug("file = {file}, path = {path}".format(path=path, file=file))

		self.load_default()
		self.load_path()

		print(self)
#		self.dump()

	def load_default(self):

		self["ui"] = dict(
			bg_color = "#ffffff",
			fg_color = "#000000",
		)

		self["book"] = dict(
			path = ["/tmp/bsr/books"],
		)

		self["sqlite"] = dict(
			sql = "/tmp/bsr/bsr.db",
		)

		self["config"] = dict(
			path = "/tmp/bsr/config",
		)


	def load(self, fl = None):
		if fl == None or not fl.endswith(".json"):
			return None

		log_debug('do loading file : ' + fl)

		obj = None

		with open(fl, "r", encoding = "utf-8") as src:
			obj = json.load(src)

		return obj
			

	def load_path(self):

		if not self.config_path:
			return  None

		keys = list(self.keys())

		for key in keys:
			jobj = self.load(self.config_path + "/" + key + ".json")

			if jobj:
				self[key].update(jobj)


	def dump(self):

		try:
			if not os.path.exists(self.config_path):
				os.system("mkdir -p " + self.config_path)
		except:
			raise "can't mkdir " + self.config_path

		keys = list(self.keys())

		for key in keys:
			fl = self.config_path + "/" + key + ".json"
			with open(fl, "w", encoding = "utf-8") as target:
				json.dump(self[key], target, indent=4)


if __name__ == '__main__':
	cfg = BSRConfig(path="../config")


