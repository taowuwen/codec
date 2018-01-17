#!/usr/bin/env python3

import tkinter
import sys
import os

from chapter import Chapter
from fast_reader import ScrolledText
from fast_reader import StatusBar
from fast_reader import ButtonQuitter
from fast_reader import FastReading
from fast_reader import ButtonStart
from fast_reader import MenuConfig

class App:
    def __init__(self):

        self.top = tkinter.Tk()
        self.top.title('Speedy reader...')
        self.top.geometry('1024x768')
        self.make_widgets()
        self.chapter = Chapter()

    def make_widgets(self):
        self.top_frame  = tkinter.Frame(self.top)
        self.top_frame.pack(side=tkinter.TOP, fill=tkinter.X)

        self.status_bar = StatusBar(self.top_frame)
        self.status_bar.pack(side=tkinter.LEFT, expand=tkinter.YES, fill=tkinter.BOTH)
        self.quitter = None
        self.read_text = None

        self.scroll_text = ScrolledText(self.top)
        self.scroll_text.pack(expand=tkinter.YES, side=tkinter.TOP, fill=tkinter.BOTH)

        self.menu_frame = tkinter.Frame(self.top)
        self.menu_frame.pack(side=tkinter.BOTTOM, fill=tkinter.X)


        self.btn_start = ButtonStart(self.menu_frame, command=self._do_start)
        self.btn_menu  = MenuConfig(self.menu_frame, cb=self.command_cb)

        self.btn_start.pack(expand=tkinter.YES, side=tkinter.LEFT, anchor=tkinter.E)
        self.btn_menu.pack(expand=tkinter.YES, side=tkinter.RIGHT, anchor=tkinter.N)

        self.top.bind('<Control-Return>', (lambda event: self._do_start()))


    def _do_start(self):

        lines = self.scroll_text.gettext().split(os.linesep)
        self.chapter.reset()
        self.chapter.update_chapter(lines=lines)

        self.scroll_text.pack_forget()
        self.menu_frame.pack_forget()

        if not self.quitter:
            self.quitter = ButtonQuitter(self.top_frame)
            self.quitter.config(command=self._do_stop)

        self.quitter.pack(side=tkinter.RIGHT)
        self.top_frame.pack_forget()

        if not self.read_text:
            self.read_text = FastReading(self.top, event_cb=self.event_cb)
        self.read_text.pack(side=tkinter.BOTTOM, expand=tkinter.YES, fill=tkinter.BOTH)

        self.top.bind('+', self.read_text._speed_up)
        self.top.bind('-', self.read_text._speed_down)
        self.top.bind('<space>', self.read_text._do_run)
        self.top.bind('<Motion>', self.mouse_move)
        self._hide_id = None

        self.read_text.start()

    def mouse_move(self, evt):
        self.top_frame.pack(side=tkinter.TOP, fill=tkinter.X)

        if self._hide_id:
            self.top.after_cancel(self._hide_id)

        self._hide_id = self.top.after(3000, self.hide_status_bar)


    def _do_stop(self):

        self.top.unbind('+')
        self.top.unbind('-')
        self.top.unbind('<space>')
        self.top.unbind('<Motion>')

        if self.quitter:
            self.quitter.pack_forget()

        if self.read_text:
            self.read_text.stop()
            self.read_text.pack_forget()

        if self._hide_id:
            self.top.after_cancel(self._hide_id)
            self._hide_id = None

        self.top_frame.pack(side=tkinter.TOP, fill=tkinter.X)
        self.scroll_text.pack(expand=tkinter.YES, side=tkinter.TOP, fill=tkinter.BOTH)
        self.menu_frame.pack(side=tkinter.BOTTOM, fill=tkinter.X)



    def evt_reading(self):
        self.status_bar.status_update(status="running")

        self._hide_id = self.top.after(3000, self.hide_status_bar)

    def hide_status_bar(self):
        if self._hide_id:
            self.top.after_cancel(self._hide_id)

        self.top_frame.pack_forget()

    def evt_read_stopped(self):
        self.top_frame.pack(side=tkinter.TOP, fill=tkinter.X)
        self.status_bar.status_update(status="paused")

    def evt_next_chunk(self):

        n, data = self.chapter.next_chunk(self.read_text.chunk)

        chp_n, chp = self.chapter.current_chapter

        self.status_bar.status_update(total=self.chapter.total)
        self.status_bar.status_update(total_chpater=self.chapter.total_chapter)
        self.status_bar.status_update(current=chp_n)
        self.status_bar.status_update(chapter=chp)
        self.status_bar.status_update(speed=self.read_text.speed)

        return data


    def event_cb(self, event):
        return {'running': self.evt_reading,
                'paused':  self.evt_read_stopped,
                'next_chunk': self.evt_next_chunk
                }.get(event, 'evt_default')()


    def import_file(self):
        from tkinter.filedialog import askopenfilename

        opts = {}
        opts['defaultextension'] = '.txt'
        opts['filetypes'] = [('all files', '.*'), ('text files', '.txt')]

        fl = askopenfilename(**opts)
        if fl:
            self.scroll_text.settext(file=fl)

    def make_setting_widgets(self, top):

        tkinter.Label(top, text='Hello, not yet Implemented').pack(fill=tkinter.X)
        tkinter.Entry(top, text='Input something').pack(fill=tkinter.X)




    def setting(self):

        win = tkinter.Toplevel(self.top)
        win.geometry('500x300')
        win.title('setting...')

        frame = tkinter.Frame(win)
        frame.pack(expand=tkinter.YES, fill=tkinter.BOTH)
        self.make_setting_widgets(frame)

        tkinter.Button(win, text='OK', command=win.destroy).pack(side=tkinter.BOTTOM)

        win.focus_set()
        win.grab_set()
        win.wait_window()

    def command_cb(self, menu):
        menu.add_command(label='import file', command=self.import_file)
        menu.add_command(label='setting', command=self.setting)
        menu.add_command(label='Quit', command=self.top.quit)


    def mainloop(self):
        self.top.mainloop()


if __name__ == '__main__': App().mainloop()
