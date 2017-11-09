#!/usr/bin/env python

import os
import sys
import json


print("hello, json testing....");

tmp = {
	"hello": "world",
	"a": "a_v",
	"d": {
		"h": "h_v",
		"i": "i_v"
	},
	"b": "b_v",
	"c": "c_v",
}


print(json.dumps(['foo', {'bar': ('baz', None, 1.0, 2)}]))
print(json.dumps(tmp, separators=(',', ':'), sort_keys=True, indent=8))

def parse_object_cb(obj):
	print("type(obj) = {}, obj = {}".format(type(obj), str(obj)))
	print(obj.get('a', "nothing got"));
	print(obj.get('jj', "nothing got"));
	return obj

json.loads(json.dumps(tmp), object_hook = parse_object_cb)



