#!/usr/bin/env python3
# -*- coding: utf-8 -*-



class Node:
	def __init__(self, tag_name, parent = None):
		self.parent = parent
		self.tag_name = tag_name
		self.children = []
		self.text = ""

	def __str__(self):
		if self.text:
			return "{} : {}".format(self.tag_name , self.text)
		else:
			return self.tag_name


class Parser:
	def __init__(self, parse_string):
		self.parse_string = parse_string
		self.root = None
		self.current_node = None

		self.state = FirstTag()


	def process(self, remaining_string):
		remaining = self.state.process(remaining_string, self)
		if remaining:
			self.process(remaining)

	def start(self):
		return self.process(self.parse_string)


class FirstTag:
	def process(self, remaining_string, parser):
		i_start_tag = remaining_string.find('<')
		i_end_tag = remaining_string.find('>')

		tag_name = remaining_string[i_start_tag+1:i_end_tag]
		root = Node(tag_name)
		parser.root = parser.current_node = root

		parser.state = ChildNode()
		return remaining_string[i_end_tag+1:]


class ChildNode:
	def process(self, remaining_string, parser):
		stripped = remaining_string.strip()

		if stripped.startswith("</"):
			parser.state = CloseTag()
		elif stripped.startswith("<"):
			parser.state = OpenTag()
		else:
			parser.state = Text()

		return stripped

class OpenTag:
	def process(self, remaining_string, parser):
		i_start_tag = remaining_string.find('<')
		i_end_tag   = remaining_string.find('>')

		tag_name = remaining_string[i_start_tag+1:i_end_tag]
		node = Node(tag_name, parser.current_node)

		parser.current_node.children.append(node)
		parser.current_node = node
		parser.state = ChildNode()
		return remaining_string[i_end_tag+1:]

class CloseTag:
	def process(self, remaining_string, parser):
		i_start_tag = remaining_string.find('<')
		i_end_tag   = remaining_string.find('>')

		assert remaining_string[i_start_tag+1] == '/'

		tag_name = remaining_string[i_start_tag+2:i_end_tag]

		assert tag_name == parser.current_node.tag_name, "{} != {}".format(tag_name, parser.current_node.tag_name)

		parser.current_node = parser.current_node.parent

		parser.state = ChildNode()

		return remaining_string[i_end_tag+1:].strip()

class Text:
	def process(self, remaining_string, parser):
		i_start_tag = remaining_string.find('<')

		parser.current_node.text = remaining_string[:i_start_tag]

		parser.state = ChildNode()

		return remaining_string[i_start_tag:]


def main():
	import sys


	with open("test.xml", "r") as f:
		contents = f.read()

		p = Parser(contents)

		p.start()

		nodes = [p.root]

		while nodes:
			node = nodes.pop(0)
			print(node)
			nodes = node.children + nodes
		


if __name__ == '__main__':
	main()