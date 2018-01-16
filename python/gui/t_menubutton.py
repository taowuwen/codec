#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import tkinter
from t_text import ScrolledText
import sys
import os
from tkinter.filedialog import askopenfilename

st = None
mbutton = None
cur = 1

def do_import():

    global st

    fl = askopenfilename()
    if fl:
        st.set_text(file=fl)


def do_setting(root):

    top = tkinter.Toplevel(root)

    top.geometry('512x52')

    tkinter.Button(top, text='Quit', command=top.destroy).pack()

    top.focus_set()
    top.grab_set()
    top.wait_window()


def make_menu(win):
    global mbutton

    mbutton = tkinter.Menubutton(win, text='...')

    picks = tkinter.Menu(mbutton)
    picks.add_command(label='import file', command=do_import)
    picks.add_command(label='setting', command=lambda win=win: do_setting(win))
    picks.add_command(label='Quit', command=win.quit)

    mbutton.config(menu=picks)
    mbutton.pack(side=tkinter.TOP, anchor=tkinter.E)
    mbutton.config(bg='white', bd=4)

def menu_visible(event):

    global mbutton
    global cur
    cur += 1

    if cur % 2:
        mbutton.pack(side=tkinter.TOP, anchor=tkinter.E)
    else:
        mbutton.pack_forget()



if __name__ == '__main__':

    root = tkinter.Tk()

    root.title('test import file and menubutton')
    root.geometry('1024x768')

    make_menu(root)

    st = ScrolledText(root, text='\n PRESS "m" to hide/show menubutton\n\nhello, world')
    st.pack(side=tkinter.BOTTOM)

    root.bind('m', menu_visible)

    root.mainloop()

