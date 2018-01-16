#!/usr/bin/env python3
# -*- coding: utf-8 -*-



import tkinter
from tkinter.messagebox import *

def notdone():
    showerror('Not Implemented', 'Not yet available')


def makemenu(win, sub_command=False):

    top = tkinter.Menu(win)
    win.config(menu=top)

    file = tkinter.Menu(top)
    file.add_command(label='New..',  command=notdone, underline=0)
    file.add_command(label='Open..', command=notdone, underline=0)
    file.add_command(label='Quit..', command=win.destroy, underline=0)
    top.add_cascade(label='File', menu=file, underline=0)

    edit = tkinter.Menu(top)
    edit.add_command(label='Cut',  command=notdone, underline=1)
    edit.add_command(label='Copy',  command=notdone, underline=1)
    edit.add_separator()
    edit.add_command(label='Paste',  command=notdone, underline=1)
    top.add_cascade(label='Edit', menu=edit, underline=1)

    if sub_command:

        m_sub = tkinter.Menu(edit)
        m_sub.add_command(label='spam', command=notdone, underline=0)
        m_sub.add_command(label='SPAM', command=notdone, underline=0)
        edit.add_separator()
        edit.add_cascade(label='Stuff', menu=m_sub, underline=0)


def test_window(win):
    makemenu(win, sub_command=True)
    msg = tkinter.Label(win, text='Window menu basics')
    msg.config(relief=tkinter.SUNKEN, width=40, height=7, bg='beige')
    msg.pack(expand=tkinter.YES, fill=tkinter.BOTH)



def test_multi_window(root):
    for i in range(3):
        win = tkinter.Toplevel(root)

        makemenu(win)
        msg = tkinter.Label(win, text='Window menu basics')
        msg.config(relief=tkinter.SUNKEN, width=40, height=7, bg='beige')
        msg.pack(expand=tkinter.YES, fill=tkinter.BOTH)


if __name__ == '__main__':

    root = tkinter.Tk()

    root.title('menu_win')

    test_window(root)
    #test_multi_window(root)
    tkinter.Button(root, text='Quit', command=root.quit, width=200, height=10).pack()

    root.mainloop()
