#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from test_utils import timeit


from urllib.request import urlopen
from bs4 import BeautifulSoup

@timeit
def do_download_url(url):
    return urlopen(url).read()



def get_node(bs, *key, **kwargs):

    vals = bs.find_all(*key, **kwargs)

    return vals[0] if vals else None



def parse_23us(bs = None):
    #print(bs.prettify())


    amain = get_node(bs, id='amain')

    # get title
    if amain:
        print(amain.dl.dd.h1.string.strip())

    # get content
    obj = get_node(amain, id='contents')
    if obj:
        for ln, c in enumerate(obj.children):
            if c.string and str(c.string).strip():
                print(ln, "\t" + str(c.string).strip())

    obj = get_node(amain, id='footlink')

    if obj:
        urls = obj.find_all('a')

        last_page = urls[0]
        menu = urls[1]
        next_page = urls[2]

        print("last page is : {}".format(last_page['href']))
        print("menu page is : {}".format(menu['href']))
        print("next page is : {}".format(next_page['href']))


        for url in obj.find_all('a'):
            print(url.get("href"))

        for ln, c in enumerate(obj.children):
            if c.string and str(c.string).strip():
                print(ln, "\t" + str(c.string).strip())



def parse_def(obj = None):

    for l in obj.find_all(id="content"):
        print(l.contents, len(l.contents))
        _id = l.attrs.get('id', None)
        if _id == "contents":
            for ln, c in enumerate(l.children):
                if c.string and str(c.string).strip():
                    print(ln, "\t" + str(c.string).strip())

    
   #for l in obj.find_all('div'):

   #    if l.get('id', None):
   #        print(l.contents)

   #        for ind, child in enumerate(l):
   #            if not str(child).strip() in ("", "<br/>"):
   #                print("{}: {}".format(ind, child))


    print("links as follows:")
    for l in obj.find_all('a'):
        print(l.get('href'))

if __name__ == '__main__':
    print("hello, test beautiful soup")

    url='https://www.bequge.com/28_28784/12755056.html'
    fl='/tmp/index.html'
    #fl='/tmp/3213012.html'

    #obj = BeautifulSoup(do_download_url('http://www.biquge.lu/book/9806/4241761.html'), "html.parser")
    #obj = BeautifulSoup(open("/tmp/4241761.html", "r", encoding='gb2312').read(), "html.parser")
    #obj = BeautifulSoup(open("/tmp/index.html", "r").read(), "html.parser")

    #obj = BeautifulSoup(do_download_url(url), "html.parser")
    obj = BeautifulSoup(open(fl, "r").read(), "html.parser")

    #print(obj.prettify())

    print("attrs: {}".format(obj.body.div.attrs))

    parse_23us(obj)
#    parse_def(obj)

