import tkinter
import queue
from dbgmsg import DbgMessage
from dbgconfig import config

try:
    import tkFont
except ImportError:
    import tkinter.font as tkFont

class DbgView:
    def __init__(self, *kargs, **kwargs):
        self.root = tkinter.Tk()
        self.root.geometry('1024x768')
        self.root.title('DebugView by taowuwen@gmail.com')
        self.datactl, self.cfgctl, self.mq_gui, self.actionctl, *_ = kargs

        self.font_cfg= {
                'font':'systemSystemFont',
                'fg':'black',
                'bg':'white'
            }

        self.do_init_widget()

    def check_msg_queue(self):
        while True:
            try:
                _id = self.mq_gui.get_nowait()
            except queue.Empty:
                self.root.after(100, self.check_msg_queue)
                break
            else:
                if isinstance(_id, str):
                    msg = _id
                    self.listbox.insert(tkinter.END, msg)
                elif isinstance(_id, int):
                    msg = self.datactl.get(_id)
                    self.actionctl.gui_action(msg, self.listbox)
                else:
                    print(f"warnning, unkown id: {_id}")
                    continue

                self.listbox.yview(tkinter.END)

    def run(self):
        self.root.after(100, self.check_msg_queue)
        return self.root.mainloop()

    def do_init_widget(self):
        self.prepare_menu()
        self.frame    = tkinter.Frame(self.root)
        self.listbox  = tkinter.Listbox(self.frame)
        self.y_scroll = tkinter.Scrollbar(self.root, orient=tkinter.VERTICAL)
        self.x_scroll = tkinter.Scrollbar(self.frame, orient=tkinter.HORIZONTAL)

        self.frame.pack(side=tkinter.LEFT, fill=tkinter.BOTH, expand=tkinter.YES)

        self.listbox.pack(side=tkinter.TOP, fill = tkinter.BOTH, expand=tkinter.YES)
        self.x_scroll.pack(side=tkinter.TOP, fill=tkinter.X)

        self.y_scroll.pack(side=tkinter.LEFT, fill=tkinter.Y)

        kwargs = {
            "selectmode":     tkinter.EXTENDED,
            "yscrollcommand": self.y_scroll.set,
            "xscrollcommand": self.x_scroll.set,
        }

        kwargs.update(config.gui_font)

        self.listbox.config(kwargs)
    #   (
    #       fg               = 'black',
    #       selectforeground = 'white',
    #       selectbackground = '#33B397',
    #       height           = 10,
    #       width            = 10,
    #       font             = 'systemSystemFont',
    #   )

        self.y_scroll.config(command = self.listbox.yview)
        self.x_scroll.config(command = self.listbox.xview)

    def do_test_widget(self):
        for item in ["one", "two", "three", "four", "five", "sex", "testing...."]:
            self.listbox.insert("end", item)

        tmpfont = tkFont.Font(family='Helvetica', size=20, weight='normal')
        self.listbox.config(font='systemSystemFont')

        self.listbox.itemconfig(1, {'bg':'red'})
        self.listbox.itemconfig(3, {'fg':'blue'})
        self.listbox.itemconfig(2, bg='green')
        self.listbox.itemconfig(0, foreground="purple")
        self.listbox.itemconfig(tkinter.END, {'bg':'black'})

    def command_cb(self):
        pass


    def clear_screen(self):
        self.listbox.delete(0, tkinter.END)

    def prepare_menu(self):

        self.menu_root = tkinter.Menu(self.root, **self.font_cfg)

        # file menu
        filemenu = tkinter.Menu(self.menu_root, tearoff=0, **self.font_cfg)
        filemenu.add_command(label='Open', command=self.command_cb)
        filemenu.add_command(label='Edit', command = self.command_cb)
        filemenu.add_separator()
        filemenu.add_command(label='Exit', command=self.root.quit)
        self.menu_root.add_cascade(label='File', menu=filemenu)

        # edit menu
        editmenu = tkinter.Menu(self.menu_root, tearoff = 0, **self.font_cfg)

        editmenu.add_command(label="Cut", command=self.command_cb)
        editmenu.add_command(label="Copy", command=self.command_cb)
        editmenu.add_command(label="Paste", command=self.command_cb)
        editmenu.add_command(label="clear", command=self.clear_screen)
        self.menu_root.add_cascade(label="Edit", menu=editmenu)
         
        # help menu
        helpmenu = tkinter.Menu(self.menu_root, tearoff=0, **self.font_cfg)
        helpmenu.add_command(label="About", command=self.command_cb)
        self.menu_root.add_cascade(label="Help", menu=helpmenu)

        self.root.config(menu=self.menu_root)


