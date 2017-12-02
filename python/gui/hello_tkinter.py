#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import tkinter
from functools import wraps
import os
from time import sleep

ui = None
count = 0


class OnlyOneTestCouldBeRun(Exception):
    pass


def only_one_run(func):
    @wraps(func)
    def wrapper(*args, **kwargs):
        global count

        if count != 0:
            raise OnlyOneTestCouldBeRun("only one test method "
                                        "could be run for testing")

        count = 1
        res = func(*args, **kwargs)

        return res
    return wrapper


@only_one_run
def test_first(top):
    label = tkinter.Label(top, text="Hello, world, gui program --> Label")
    label.pack()

    quit = tkinter.Button(top, text="hello,quit", command=top.quit)
    quit.pack()

    quit_1 = tkinter.Button(top, text="QUIT", command=top.quit,
                            bg='red', fg='white')
    quit_1.pack(fill=tkinter.X, expand=1)


class MyGUI:
    def __init__(self, top):
        self.top = top

        self.label = tkinter.Label(top, text="hello, my gui")
        self.label.pack(fill=tkinter.Y, expand=1)

        self.scale = tkinter.Scale(top, from_=10, to=40,
                                   orient=tkinter.HORIZONTAL,
                                   command=MyGUI.resize)
        self.scale.set(12)
        self.scale.pack(fill=tkinter.X, expand=1)

        self.quit = tkinter.Button(top, text="QUIT",
                                   command=top.quit,
                                   activeforeground='white',
                                   activebackground='red')
        self.quit.pack()

    @staticmethod
    def resize(ev=None):
        global ui
        ui.label.config(font='Helvetica -{} bold'.format(ui.scale.get()))


@only_one_run
def test_second(top):
    global ui
    ui = MyGUI(top)


@only_one_run
def test_three(top):

    from functools import partial as pto
    from tkinter.messagebox import showinfo, showwarning, showerror

    WARN = 'warn'
    CRIT = 'crit'
    REGU = 'regu'

    SIGNS = {
            "do not enter": CRIT,
            "railrod crossing": WARN,
            "55\nspeed limit": REGU,
            "wrong way": CRIT,
            "merging traffic": WARN,
            "one way": REGU,
    }

    critCB = lambda: showerror('Error', "Error button pressed!!!")
    warnCB = lambda: showwarning('Warning', "Warning Button Pressed!!!")
    infoCB = lambda: showinfo('Info', 'Info button PRESSED!!!')

    tkinter.Button(top, text='QUIT',
                   command=top.quit, bg='red', fg='white').pack()

    btn = pto(tkinter.Button, top)
    CritButton = pto(btn, command=critCB, bg='white', fg='red')
    WarnButton = pto(btn, command=warnCB, bg='goldenrod1')
    ReguButton = pto(btn, command=infoCB, bg='white')

    for eachSign in SIGNS:
        signType = SIGNS[eachSign]

        cmd = '%sButton(text=%r%s).pack(fill=tkinter.X, expand=True)' % (
                signType.title(), eachSign,
                '.upper()' if signType == CRIT else '.title()'
            )
        eval(cmd)


class DirList:

    def __init__(self, top, initdir=None):
        self.top = top
        self.label = tkinter.Label(top, text="Directory Listener v1.1")
        self.label.pack()

        self.cwd = tkinter.StringVar(top)

        self.dirl = tkinter.Label(top,
                                  fg='blue', font=('Helvetica', 12, 'bold'))
        self.dirl.pack()

        self.dirfm = tkinter.Frame(top)

        self.dirsb = tkinter.Scrollbar(self.dirfm)
        self.dirsb.pack(side=tkinter.RIGHT, fill=tkinter.Y)

        self.dirs = tkinter.Listbox(self.dirfm, height=15, width=50,
                                    yscrollcommand=self.dirsb.set)
        self.dirs.bind('<Double-1>', self.set_and_go)

        self.dirsb.config(command=self.dirs.yview)
        self.dirs.pack(side=tkinter.LEFT, fill=tkinter.BOTH)

        self.dirfm.pack()

        self.dirn = tkinter.Entry(self.top, width=50, textvariable=self.cwd)
        self.dirn.bind('<Return>', self.do_ls)
        self.dirn.pack()

        self.bfm = tkinter.Frame(top)

        self.cls = tkinter.Button(self.bfm, text='Clear',
                                  command=self.cls_dir,
                                  activeforeground='white',
                                  activebackground='blue')

        self.ls = tkinter.Button(self.bfm, text='List Directory',
                                 command=self.do_ls,
                                 activeforeground='white',
                                 activebackground='green')

        self.quit = tkinter.Button(self.bfm, text='Quit',
                                   command=self.top.quit,
                                   activeforeground='white',
                                   activebackground='red')

        self.cls.pack(side=tkinter.LEFT)
        self.ls.pack(side=tkinter.LEFT)
        self.quit.pack(side=tkinter.LEFT)

        self.bfm.pack()

        if initdir:
            self.cwd.set(os.curdir)
            self.do_ls()

    def cls_dir(self, ev=None):
        self.cwd.set('')

    def set_and_go(self, ev=None):
        self.last = self.cwd.get()
        self.dirs.config(selectbackground='red')

        check = self.dirs.get(self.dirs.curselection())
        if not check:
            check = os.curdir

        self.cwd.set(check)
        self.do_ls()

    def do_ls(self, ev=None):
        error = None
        tdir = self.cwd.get()

        if not tdir:
            tdir = os.curdir

        if not os.path.exists(tdir):
            error = tdir + ': No such file'
        elif not os.path.isdir(tdir):
            error = tdir + ': No a directory'

        if error:
            self.cwd.set(error)

            self.top.update()

            sleep(2)

            if not (hasattr(self, 'last') and self.last):
                self.last = os.curdir

            self.cwd.set(self.last)
            self.dirs.config(selectbackground='LightSkyBlue')

            self.top.update()
            return

        self.cwd.set('FETCHING DIRECTORY CONTENTS...')
        self.top.update()

        dirlist = os.listdir(tdir)
        dirlist.sort()
        os.chdir(tdir)

        self.dirl.config(text=os.getcwd())
        self.dirs.delete(0, tkinter.END)
        self.dirs.insert(tkinter.END, os.curdir)
        self.dirs.insert(tkinter.END, os.pardir)

        for fl in dirlist:
            self.dirs.insert(tkinter.END, fl)

        self.cwd.set(os.curdir)
        self.dirs.config(selectbackground='LightSkyBlue')


@only_one_run
def test_dirlist(top):
    DirList(top, os.curdir)


def main():
    print("hello, test for Tkinter")

    top = tkinter.Tk()
    top.geometry('1024x768')
    top.title('my test for gui programming or components...')

#   test_first(top)
#   test_second(top)
#   test_three(top)
    test_dirlist(top)

    tkinter.mainloop()


if '__main__' == __name__:
    main()
