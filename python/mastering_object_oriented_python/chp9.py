#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import json
import datetime
class Post:
	def __init__(self, date, title, rst_text, tags):
		self.date = date
		self.title = title
		self.rst_text = rst_text
		self.tags = tags

	def as_dict(self):
		return dict(
			date = str(self.date),
			title = self.title,
			underline = "-" * len(self.title),
			rst_text = self.rst_text,
			tag_text = " ".join(self.tags),
		)

	def _json(self):
		return dict(
			__class__ = self.__class__.__name__,

			__kw__ = dict(
				date = self.date,
				title = self.title,
				rst_text = self.rst_text,
				tags = self.tags,
			),

			__args__ = []
		)


from collections import defaultdict
class Blog:
	def __init__(self, title, posts = None):
		self.title = title
		self.entries = posts if posts is not None else []

	def append(self, post):
		self.entries.append(post)


	def by_tag(self):
		tag_index = defaultdict(list)

		for post in self.entries:
			for tag in post.tags:
				tag_index[tag].append(post.as_dict())

		return tag_index
	def as_dict(self):
		return dict(
			title = self.title,
			underline = "="*len(self.title),
			entries = [p.as_dict() for p in self.entries],
		)

	def _json(self):
		return dict(
			__class__ = self.__class__.__name__,
			__kw__ = {},
			__args__ = [self.title, self.entries]
		)



travel = Blog("Travel")

travel.append( Post(
		date = datetime.datetime(2013, 11, 14, 17, 25),
		title = "Hard Aground",
		rst_text = """ 
		Some embarrassing revelation. Including AAA and BBB
		""",
		tags = ( "#RedRanger", "#Whitby42", "#ICW")
	)
)

travel.append( Post(
		date = datetime.datetime(2013, 11, 18, 11, 30),
		title = "Anchor Follies",
		rst_text = """ 
		Some Anchor Follies CCCCC
		""",
		tags = ( "#RedRanger", "#Whitby42", "#Mistakes")
	)
)



from jinja2 import Template

blog_template = Template("""
{{title}}
{{underline}}

{% for e in entries %}
{{e.title}}
{{e.underline}}
{{e.rst_text}}
:date: {{e.date}}
:tags: {{e.tag_text}}
{% endfor %}
Tag Index
========
{% for t in tags %}
* {{t}}
	{% for post in tags[t] %}
	- '{{post.title}}' -
	{% endfor %}
{% endfor %}
""")
print(blog_template.render(tags = travel.by_tag(), **travel.as_dict()))



def blog_encode_1(object):
	if isinstance(object, datetime.datetime):
		return dict(
				__class__="datetime.datetime",
				__args__ = [],
				__kw__ = dict(
					year = object.year,
					month = object.month,
					day = object.day,
					hour = object.hour,
					minute = object.minute,
					second = object.second,
				)
			)

	elif isinstance(object, Post):
		return dict(
				__class__ = "Post",
				__args__ = [],
				__kw__ = dict(
					date = object.date,
					title = object.title,
					rst_text = object.rst_text,
					tags = object.tags,

				)
			)

	elif isinstance(object, Blog):
		return dict(
				__class__ = "Blog",
				__args__ = [
					object.title,
					object.entries,
				],
				__kw__ = {}
			)
	else:
		return json.JSONEncoder.default(object)



def blog_encode_2 (object):
	if isinstance(object, datetime.datetime):
		return dict(
				__class__="datetime.datetime",
				__args__ = [],
				__kw__ = dict(
					year = object.year,
					month = object.month,
					day = object.day,
					hour = object.hour,
					minute = object.minute,
					second = object.second,
				)
			)

	else:
		try:
			encoding = object._json()
		except AtrributeError:
			encoding = json.JSONEncoder.default(object)
		return encoding


def blog_encode_3 (object):
	if isinstance(object, datetime.datetime):
		fmt = "%Y-%m-%dT%H:%M:%S"
		return dict(
				__class__="datetime.datetime.strptime",
				__args__ = [object.strftime(fmt), fmt],
				__kw__ = {}
			)

	else:
		try:
			encoding = object._json()
		except AtrributeError:
			encoding = json.JSONEncoder.default(object)
		return encoding


def blog_decode(some_dict):
	if set(some_dict.keys()) == set( ["__class__", "__args__", "__kw__"]):
		class_ = eval(some_dict['__class__'])

		return class_(*some_dict['__args__'], **some_dict['__kw__'])

	else:
		return some_dict

#blog_encode = blog_encode_2
blog_encode = blog_encode_3

text = json.dumps(travel, indent=4, default=blog_encode)
blog_data = json.loads(text, object_hook=blog_decode)


# dumps

fl="/tmp/temp.json"

with open(fl, "w", encoding="UTF-8") as target:
	json.dump(travel, target, indent=4, default=blog_encode)


# load
with open(fl, "r", encoding="utf-8") as source:
	blog = json.load(source, object_hook=blog_decode)



# YAML's testing
# should install yaml  : pip install pyYAML --user


import yaml
text1 = yaml.dump(travel)
print(text1)

fl = "/tmp/temp.yaml"
with open(fl, "w", encoding = "utf-8") as target:
	yaml.dump(travel, target)


with open(fl, "r", encoding = "utf-8") as source:
	blog1 = yaml.load(source)

print(type(blog1))





