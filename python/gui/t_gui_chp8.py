#!/usr/bin/env python3
# -*- coding:utf-8 -*-

import sys
import tkinter
import tkinter.messagebox as msgbox
from tkinter.filedialog import askopenfilename
from tkinter.colorchooser import askcolor
from tkinter.simpledialog import askfloat


class HelloButton(tkinter.Button):

    def __init__(self, parent, **configs):
        
        super(HelloButton, self).__init__(parent, **configs)

        self['bg'] = 'black'
        self['fg'] = 'white'
        self['padx'] = 20
        self['pady'] = 20

        if 'command' not in configs:
            self['command'] = sys.exit

        self.pack()


def _test_1():
    win1 = tkinter.Toplevel()
    win2 = tkinter.Toplevel()

    win1.geometry('200x100')
    win2.geometry('200x100')

    HelloButton(win1, text='spam')
    HelloButton(win2, text='spam')

    tkinter.Label(text='popups').pack()

    win1.mainloop()



class TestMsg:
    def __init__(self, parent=None, **configs):

        if not parent:
            parent = tkinter.Tk()

        self.top = parent

        self.top.geometry('1024x768')
        self.top.title('test for testmsg')

        self.prepare()


    def mainloop(self):
        self.top.mainloop()

    def callback(self):

        if msgbox.askyesno('Verify', 'Do you really want to quit?'):
            msgbox.showwarning('Yes', 'but quit not yes implemented, HAHAHA...')
        else:
            msgbox.showinfo('No', 'Quit has been cancelled')

    def prepare(self):
        errmsg = 'Sorry, no Spam allowed'
        HelloButton(self.top, text='quit', command=self.callback).pack(fill=tkinter.X)
        HelloButton(self.top, text='SPAM', command=lambda: msgbox.showerror('Spam', errmsg)).pack(fill=tkinter.X)
        HelloButton(self.top, text='QUIT......').pack(fill=tkinter.X)


demos = {
        'Open': askopenfilename,
        'Color': askcolor,
        'Query': lambda: msgbox.askquestion('Warning!', 'You typed "rm -rf" \n Confirmed?'),
        'Error': lambda: msgbox.showerror('Error!!!', "he's dead, jim"),
        'Input': lambda: askfloat('Entry', 'Enter credit card number')
}


class TestDemo(tkinter.Frame):

    def __init__(self, parent=None, **options):

        if not parent:
            parent = tkinter.Tk()

        super(TestDemo, self).__init__(parent, **options)
        self.pack(expand=tkinter.YES, fill=tkinter.BOTH)
        parent.geometry('1024x768')

        self.v_label = tkinter.StringVar()
        self.v_label.set('Basic Demos')
        self.label = tkinter.Label(self, textvariable=self.v_label)
        self.label.pack()

#        for k, v in demos.items():
#            HelloButton(self, text=k, command=v).pack(side=tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)

        for k in demos:
            HelloButton(self, text=k, command=(lambda k=k: self.print_result(k))).pack(side=tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)
        HelloButton(self, text='QUIT').pack(side=tkinter.TOP, fill=tkinter.BOTH, expand=tkinter.YES)

    def print_result(self, key):

        res = demos[key]()
        self.v_label.set('{}'.format(res))

        if key is 'Color' and res[-1]:
            self.label.config(fg=res[-1])
        


from tkinter.dialog import Dialog

class OldDialogDemo(tkinter.Frame):

    def __init__(self, parent=None, **configs):
        super(OldDialogDemo, self).__init__(parent, **configs)
        self.pack()

        HelloButton(self, text='Pop1', command=self.dialog1).pack(fill=tkinter.BOTH)
        HelloButton(self, text='Pop2', command=self.dialog2).pack(fill=tkinter.BOTH)

    def dialog1(self):
        ans = Dialog(self,
                title='Popup Fun!',
                text = 'An example of a popup-dialog'
                        'box, using older "dialog.py"?',
                bitmap = 'questhead',
                default = 0, strings=('yes', 'no', 'cancel'))

        if ans.num == 0: self.dialog2()

    def dialog2(self):
        Dialog(self, title='HAL-9000', text="Im afraid i can't let you do that, Dave...",
                bitmap='questhead',
                default=0, strings=('spam', 'SPAM'))




makemodal = (len(sys.argv) > 1)

def dialog():

    win = tkinter.Toplevel()
    tkinter.Label(win, text='hard drive reformatted!!!').pack()
    tkinter.Button(win, text='OK', command=win.destroy).pack()

    if makemodal:
        win.focus_set()
        win.grab_set()
        win.wait_window()

    print('dialog exit...')

if __name__ == '__main__':
    print('hello, test for chp8')
#    _test_1()

#    TestMsg().mainloop()
#    TestDemo().mainloop()

    root = tkinter.Tk()
    root.geometry('1024x768')

  #  OldDialogDemo(root)
    HelloButton(root, text='QUIT', command=dialog).pack(fill=tkinter.BOTH, expand=tkinter.YES)

    root.mainloop()


