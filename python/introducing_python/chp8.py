#!/usr/bin/env python3
# -*- coding: utf-8 -*-

mammoth = """
    We have seen thee, queen of cheese,
    Lying quietly at your ease,
    Gently fanned by evening breeze,
    Thy fair form no flies dare seize.

    All gaily dressed soon you'll go
    To the great Provincial show,
    To be admired by many a beau
    In the city of Toronto.

    Cows numerous as a swarm of bees,
    Or as the leaves upon the trees,
    It did require to make thee please,
    And stand unrivalled, queen of cheese.

    May you not receive a scar as
    We have heard that Mr. Harris
    Intends to send you off as far as
    The great world's show at Paris.

    Of the youth beware of these,
    For some of them might rudely squeeze
    And bite your cheek, then songs or glees
    We could not sing, oh! queen of cheese.

    We'rt thou suspended from balloon,
    You'd cast a shade even at noon,
    Folks would think it was the moon
    About to fall and crush them soon.
"""


print(len(mammoth))

#fout = open("/tmp/output", "wt")
#if fout:
#    res = fout.write(mammoth)
#    print("write result: ", res)
#    fout.close()

#fout = open("/tmp/output", "wt")
#if fout:
#    print(mammoth, file=fout)
#    fout.close()

#fout = open("/tmp/output", "wt")
#if fout:
#    print(mammoth, file=fout, sep='', end='')
#    fout.close()

#try:
#    fout = open("/tmp/output", "wt")
#
#except Exception as e:
#    print(e)
#    import os
#    os._exit(0)
#
#offset = 0
#chunk  = 10
#id     = 0
#size   = len(mammoth)
#
#
#while offset < size:
#    res = fout.write(mammoth[offset:offset + chunk])
#    print("{} write result {}".format(id, res))
#    id += 1
#    offset += res
#
#fout.close()
#print("total size: ", size)

#try:
#    fin = open("/tmp/output", "rt")
#
#    poem = fin.read()
#
#    fin.close()
#
#    print(len(poem))
#
#    if poem == mammoth:
#        print("match!!!")
#    else:
#        print("not match!!!")
#
#except Exception as e:
#    print(e)
#    import os
#    os._exit(0)


#try:
#    fin = open("/tmp/output", "rt")
#
#    chunk = 10
#    poem = ""
#
#    while True:
#        frag = fin.read(chunk)
#        if not frag:
#            break
#
#        poem += frag
#
#    fin.close()
#
#    print(len(poem))
#
#    if poem == mammoth:
#        print("match!!!")
#    else:
#        print("not match!!!")
#
#except Exception as e:
#    print(e)
#    import os
#    os._exit(0)

#try:
#    fin = open("/tmp/output", "rt")
#
#    poem = ""
#
#    while True:
#        line = fin.readline()
#        if not line:
#            break
#
#        poem += line
#
#    fin.close()
#
#    print(len(poem))
#
#    if poem == mammoth:
#        print("match!!!")
#    else:
#        print("not match!!!")
#
#except Exception as e:
#    print(e)
#    import os
#    os._exit(0)

try:
    fin = open("/tmp/output", "rt")

    poem = ""

    for line in fin:
        poem += line

    fin.close()

    print(len(poem))

    if poem == mammoth:
        print("match!!!")
    else:
        print("not match!!!")

except Exception as e:
    print(e)
    import os
    os._exit(0)
