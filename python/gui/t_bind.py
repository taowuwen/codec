#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tkinter
import sys
import os

v_label = None
v_label_hello = None

def showPosEvent(event):
    global v_label_hello
    v_label_hello.set("Widget={} X={} Y={}".format(event.widget, event.x, event.y))


def showAllEvent(event):
    print(event)
    for attr in dir(event):
        if not attr.startswith('__'):
            print(attr, '==>', getattr(event, attr))


def onKeyPress(event):
    v_label.set('Got key press: {}'.format(event.char))


def onLeftClick(event):
    v_label.set('Got LEFT <<<<')
    showPosEvent(event)

def onMiddleClick(event):
    v_label.set('Got middle ^^^^^')
    showPosEvent(event)
    showAllEvent(event)

def onRightClick(event):
    v_label.set('Got RIGHT >>>>>')
    showPosEvent(event)

def onDoubleLeftClick(event):
    v_label.set('Got Double clicked <<<<<>>>>>')
    showPosEvent(event)
    sys.exit(0)

def onLeftDrag(event):
    v_label.set('Got LEFT DRAP <<<')
    showPosEvent(event)

def onArrowKey(event):
    v_label.set('Got ARROW KEY')
    v_label_hello.set('')

def onReturnKey(event):
    v_label.set('Got RETRUN KEY')
    v_label_hello.set('')


def do_bind(top):

    labelfont = ('courier', 20)

    global v_label_hello
    v_label_hello = tkinter.StringVar()

    label = tkinter.Label(top, font=labelfont, textvariable=v_label_hello, bg='#aabbcc', fg='blue')
    label.pack(side=tkinter.TOP, expand=tkinter.YES, fill=tkinter.BOTH)

    v_label_hello.set('HELLO, do some typing and mouse moving')



    label.bind('<Button-1>', onLeftClick)
    label.bind('<Button-2>', onMiddleClick)
    label.bind('<Button-3>', onRightClick)

    label.bind('<Double-1>', onDoubleLeftClick)
    label.bind('<B1-Motion>', onLeftDrag)

    label.bind('<KeyPress>', onKeyPress)
    label.bind('<Up>', onArrowKey)
    label.bind('<Return>', onReturnKey)

    label.focus()


if __name__ == '__main__':

    top = tkinter.Tk()
    top.geometry('1024x768')
    top.title('hello, bind')

    labelfont = ('courier', 20, 'bold')

    v_label = tkinter.StringVar()

    tkinter.Button(top, text='QUIT', fg='red', command=top.quit).pack(side=tkinter.RIGHT, expand=tkinter.NO, padx=5, pady=5, anchor=tkinter.N)
    tkinter.Label(top, font=labelfont, textvariable=v_label).pack(side=tkinter.TOP, expand=tkinter.NO, fill=tkinter.X)
    v_label.set('hello, binding test')

    frame = tkinter.Frame(top)
    frame.pack(side=tkinter.TOP, expand=tkinter.YES, fill=tkinter.BOTH)
    do_bind(frame)

    top.mainloop()
