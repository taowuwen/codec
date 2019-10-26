#!/usr/bin/env python

import os
import sys

import glob

print("hello, glob testing")

print(f"{glob.glob('../*')}")
print(f"dirs: {glob.glob('./*/')}")
print(f"all: {glob.glob('./*')}")
