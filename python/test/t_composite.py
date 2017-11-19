#!/usr/bin/env python3
# -*- coding: utf-8 -*-


class Componet:
	def __init__(self, name):
		self.name = name

	def move(self, new_path):
		new_folder = get_path(new_path)
		del self.parent.children[self.name]
		new_folder.children[self.name] = self
		self.parent = new_folder

	def delete(self):
		del self.parent.children[self.name]

	def __str__(self):
		return self.name


class Folder(Componet):
	def __init__(self, name):
		super().__init__(name)
		self.children = {}

	def add_child(self, child):
		child.parent = self
		self.children[child.name] = child

	def copy(self, new_path):

		new_root = get_path(new_path)

		new_folder = Folder(self.name)
		new_root.add_child(new_folder)

		for child in self.children.values():
			child.copy(new_path + "/" + new_folder.name)


	def __str__(self):
		return "/" if self.name == "/" else self.name + "/"

	@property
	def files(self):
		return " ".join([str(v) for v in self.children.values()])

class File(Componet):
	def __init__(self, name, contents):
		super().__init__(name)

		self.contents = contents

	def copy(self, new_path):
		""" well, for now, we are not going to check it is 
			a new folder or new file or not, complete this later maybe"""
		new_folder = get_path(new_path)
		if isinstance(new_folder, Folder):
			new_folder.add_child(File(self.name, self.contents))


	def __str__(self):
		return "{}".format(self.name)

	@property
	def cat(self):
		return self.contents



def get_path(path):
	node = root
	names = path.split("/")[1:]

	for name in names:
		try:	
			node = node.children[name]
		except KeyError:
			raise FileNotFoundError

	return node

def print_path(path):
	node = get_path(path)

	if isinstance(node, Folder):
		print("{} ==> {}".format(node, node.files))

	else:
		print("{} --> {}".format(node, node.cat))



root = Folder("/")

def main():
	print(u"hello, testing for design mode ---- composite")

	folder1 = Folder("etc")
	folder2 = Folder("usr")

	root.add_child(folder1)
	root.add_child(folder2)

	folder1.add_child(Folder("init.d"))

	fl1 = File("sshd.conf", "Port: 22")

	folder1.add_child(fl1)
	folder2.add_child(Folder("bin"))
	folder2.add_child(File("test", "just test for /usr"))

	print(root)
	print(root.files)

	print(folder1)
	print(folder2)
	print(folder1.files)
	print(folder2.files)

	fold = get_path("/etc/init.d")

	get_path("/usr/bin").add_child(File("ls", "do ls files"))

	print("{}".format(get_path("/usr/bin").files))

	fold = get_path("/usr/bin")
	fold.move("/etc/init.d")
	print_path("/etc/init.d")
	print_path("/usr")
	print_path("/etc/init.d/bin")
	get_path("/etc/init.d/bin/ls").copy("/usr")
	print_path("/usr")

	get_path("/etc/init.d").copy("/usr")
	print_path("/usr")
	print_path("/usr/init.d")
	print_path("/usr/init.d/bin")
	print_path("/usr/init.d/bin/ls")






if '__main__' == __name__:
	main()
