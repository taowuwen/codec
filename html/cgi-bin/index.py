#!/usr/bin/env python3

import os
import sys

_s = """
<html> 
<head>
<title> 
nothing
</title>
</head>
<body>
<p> Hello, World!!! </p>
</body>
</html>
"""

print("Content-Type: text/html\r\n\r\n")
print(_s)
print(dir(env))

