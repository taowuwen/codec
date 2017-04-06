#!/usr/bin/env python3
# -*- coding: utf-8 -*-

#actor = "Richard Gnre"
#cat   = "Chester"
#weight = 8
#
#
#print("my Wife's favorite actor is %s" % actor)
#print("Our cat %s weight %s pounds" %(cat, weight))
#
#
#n = 42
#f = 7.03
#s = 'string cheese'
#
#print("OLD --- default --- %d %f %s" %(n, f, s))
#print("OLD --- right   --- %10d %10f %10s" %(n, f, s))
#print("OLD ---- left   --- %-10d %-10f %-10s" %(n, f, s))
#
#print("OLD ---- right  --- %10.4d %10.4f %10.4s" %(n, f, s))
#print("OLD ---- left   --- %-10.4d %-10.4f %-10.4s" %(n, f, s))
#
#print("OLD ---- right  --- %*.*d %*.*f %*.*s" %(10, 4, n, 10, 4, f, 10, 4, s))
#
#
#
#print("NEW --- default --- {} {} {}".format(n, f, s))
#print("{2} {0} {1}".format(f, s, n))
#print("{n} {f} {s}".format(n = 42, f = 7.03, s = 'string cheese new'))
#
#d = { 'n' : 222, 'f' : 7.1234, 's' : 'string cheese dict' }
#
#print("{n} {f} {s}".format(**d))
#print("{0[n]} {0[f]} {0[s]} {1}".format(d, "other"))
#
#
#print("{0:d} {1:f} {2:s}".format(n, f, s))
#print("{n:d} {f:f} {s:s}".format(n = n + 23, f = f * 10, s = s + "hello, world"))
#
#print("{0:10d} {1:10f} {2:10s}".format(n, f, s))
#
#print("NEW - right - {0:>10d} {1:>10f} {2:>10s}".format(n, f, s))
#print("NEW - left  - {0:<10d} {1:<10f} {2:<10s}".format(n, f, s))
#print("NEW - center- {0:^10d} {1:^10f} {2:^10s}".format(n, f, s))
#
#print("NEW - center- {0:^10d} {1:^10.4f} {2:^10.4s}".format(n, f, s))
#
#print("NEW - center- {0:*^10d} {1:*^10.4f} {2:*^10.4s}".format(n, f, s))




#import re
#source = "Young Frankenstein"
#
#def do_match(reg, s):
#    m = re.match(reg, s)
#    if m:
#        print(m.group())
#    else:
#        print("NOT FOUND!!!(reg = {0}, s = {1})".format(reg, s))
#
#def do_match_pattn(pattn, s):
#    m = pattn.match(s)
#    if m:
#        print(m.group())
#    else:
#        print("pattern mode NOT FOUND!!!(%s, %s".format(str(pattn), s))


#do_match("You", source)
#do_match("^You", source)
#do_match("Frank", source)
#do_match_pattn(re.compile("You"), source)


#m = re.search("Frank", source)
#if m:
#    print(m.group())
#
#
#m = re.search(".*Frank", source)
#if m:
#    print(m.group())


#print(re.findall('n', source))
#print(re.findall('F.', source))
#print(re.findall('n.?', source))
#
#print(re.split('n', source))
#print(re.sub('n','-', source))

#import string
#printable = string.printable
#print(len(printable), printable)
#print(re.findall("\w", printable))
#print(re.findall("\W", printable))
#print(re.findall("\d", printable))
#print(re.findall("\D", printable))
#print(re.findall("\s", printable))
#print(re.findall("\S", printable))


#x ='abc'+'-/*' + '\u00ea' + '\u0115'
#print(re.findall('\w', x))



#import re
#
#source = '''I wish i may, I wish I might
#Have a dish of fish tonight.'''
#
#print(re.findall("wish", source))
#print(re.findall("wish|fish", source))
#print(re.findall("[wf]ish", source))
#print(re.findall("^wish", source))
#print(re.findall("^I wish", source))
#print(re.findall("fish$", source))
#print(re.findall("fish tonight.$", source))
#print(re.findall("fish tonight\.$", source))
#
#
#print(re.findall("[wsh]+", source))
#print(re.findall("ght\W", source))
#
#
#print(re.findall("I (?=wish)", source))
#print(re.findall("I (?!wish)", source))
#
#print(re.findall("(?<=I) wish", source))
#print(re.findall(r"(?<!I) wish", source))
#
#print(re.findall(r"\b[wf]+ish\b", source))
#
#
#
#m = re.search(r"(.dish\b).*(\bfish)", source)
#print(m.group())
#print(m.groups())
#
#for item in m.groups():
#    print(item.strip())
#
#
#m = re.search(r"(?P<DISH>.dish\b).*(?P<FISH>\bfish)", source)
#print(m.group(), type(m.group))
#print(m.groups(), type(m.groups))
#
#for item in m.groups():
#    print(item.strip())
#
#print(m.group('DISH'))
#print(m.group('FISH'))



#blist = [1, 2, 3, 255]
#
#the_bytes = bytes(blist)
#
#print(blist, the_bytes, type(the_bytes))
#
#the_byte_array = bytearray(blist)
#print(the_byte_array, type(the_byte_array))
#
#print(b'\x61')
#
#the_byte_array[2] = 127
#
#print(the_byte_array)
#
#the_bytes = bytes(range(256))
#print(the_bytes)
#
#the_byte_array = bytearray(range(256))
#print(the_byte_array)



#import struct
#
#valid_str = "你好吗?"
#valid_str_b = valid_str.encode('UTF-8')
#
#print(valid_str)
#print(valid_str_b, type(valid_str_b), len(valid_str_b))
#
#
#
#print(valid_str_b[1:2])
#
#
#print(struct.pack('>L', 0xaabbccdd))
#print(struct.pack('<L', 0xaabbccdd))
#
#print(struct.pack('>L', 154))
#print(struct.pack('<L', 141))
#
#print(struct.unpack(">4H", valid_str_b[0:8]))
#print(struct.unpack(">4xL", valid_str_b[0:8]))
#
#print(struct.unpack("<4H", valid_str_b[0:8]))
#
#
#import binascii
#
#hexlfy = binascii.hexlify(valid_str_b)
#print(hexlfy)
#
#print(binascii.unhexlify(hexlfy))
#
#a = valid_str_b[0:2]
#b = valid_str_b[2:4]
#
#
#print(a, b, type(a))
#
#a = 0b0101
#b = 0b0001
#
#print(a, b, type(a))
#
#print("a & b  = ", a & b)
#print("a | b  = ", a | b)
#print("a ^ b  = ", a ^ b)
#print("~a = ", ~a)
#print("a >> 2 = ", a >> 2)
#print("a << 2 = ", a << 2)


#import unicodedata

#mystery = '\U0001f4a9'
#print(unicodedata.name(mystery))
#print(mystery)
#
#pop_bytes = mystery.encode('UTF-8')
#print(pop_bytes)
#
#pop_string = pop_bytes.decode('utf-8')
#print(pop_string)


#print('''
#        My kitty cat likes %s,
#        My kitty cat likes %s,
#        My kitty cat fell on his %s
#        And now thinks he's a %s.
#        ''' % ("roast beef", "ham", "head", "calm"))


#letter = """
#    Dear {salutation} {name},
#
#    Thank you for your letter. We are sorry that out {product} {verbed} in your
#    {room}. Please note that it should never be used in a {room}, especially
#    near any {animals}.
#
#    Send us your receipt and {amount} for shipping and handling. We will send 
#    you andother {product} that, in sour tests, is {percent}% less likely to
#    have {verbed}.
#
#    Thank you for your support.
#
#    Sincerely,
#    {spokesman}
#    {job_title}
#"""
#
#
#response = {
#        "salutation": "SALUTATION",
#        "name": "NAME",
#        "product": "PRODUCT",
#        "verbed": "VERBED",
#        "room": "ROOM",
#        "animals": "ANIMALS",
#        "amount": "AMOUNT",
#        "percent": 50,
#        "spokesman":"SPONKENSMAN",
#        "job_title":"JOB_TITLE"
#        }
#
#print(letter.format(**response))




#mammoth = """
#    We have seen thee, queen of cheese,
#    Lying quietly at your ease,
#    Gently fanned by evening breeze,
#    Thy fair form no flies dare seize.
#
#    All gaily dressed soon you'll go
#    To the great Provincial show,
#    To be admired by many a beau
#    In the city of Toronto.
#
#    Cows numerous as a swarm of bees,
#    Or as the leaves upon the trees,
#    It did require to make thee please,
#    And stand unrivalled, queen of cheese.
#
#    May you not receive a scar as
#    We have heard that Mr. Harris
#    Intends to send you off as far as
#    The great world's show at Paris.
#
#    Of the youth beware of these,
#    For some of them might rudely squeeze
#    And bite your cheek, then songs or glees
#    We could not sing, oh! queen of cheese.
#
#    We'rt thou suspended from balloon,
#    You'd cast a shade even at noon,
#    Folks would think it was the moon
#    About to fall and crush them soon.
#"""

#print(mammoth)

#import re
#print(re.findall(r"\bc\w*\b", mammoth))
#print(re.findall(r"\bc\w{3}\b", mammoth))
#print(re.findall(r"\b\w*r\b", mammoth))
#
#print(re.findall(r"\b\w*[aoeiu]{3}[^aoeiu\s]*\w*\b", mammoth))



hexlify_str = '47494638396101000100800000000000ffffff21f9'

import binascii

gif = binascii.unhexlify(hexlify_str)

gif_legal = "GIF89a"
gif_legal_bytes = gif_legal.encode("utf-8")

print(gif, gif_legal_bytes)

if gif[:len(gif_legal_bytes)] ==  gif_legal_bytes:
    print("matched!!!")
else:
    print("not match!!!")


import struct
width, height = struct.unpack("<HH", gif[6:10])
print("width = {width}, height={height}".format(width=width, height = height))


