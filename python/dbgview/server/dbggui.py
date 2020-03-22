import tkinter
import queue
from dbgmsg import DbgMessage
from dbgconfig import config, DbgDict
from dbgctl import dbg_controller 
from dbgactiondef import CtrlModID, cfg_table_module_common, cfg_table_module_post, dbg_print
from functools import partial
from dbgintf import dbg_intf_show
from dbgctl import dbg_ctrl_show

try:
    import tkFont
except ImportError:
    import tkinter.font as tkFont

class DbgView:
    def __init__(self, *kargs, **kwargs):
        self.root = tkinter.Tk()
        self.root.geometry('1024x768')
        self.root.title('DebugView by taowuwen@gmail.com')
        self.datactl, self.mq_gui, self.actionctl, self.app, *_ = kargs
        self.cache = DbgDict()

        self.font_cfg= {
                'font':'systemSystemFont',
                'fg':'black',
                'bg':'white'
            }

        self.do_init_widget()

        self.history = []
        self.history_pos = -1

        ctrl = dbg_controller(CtrlModID.Filter)
        self.rule_filter_table = {
                "show": ctrl.show,
                "add":  ctrl.add,
                "upt":  ctrl.upt,
                "del":  ctrl.delete,
            }

        ctrl = dbg_controller(CtrlModID.Color)
        self.rule_color_table = {
                "show": ctrl.show,
                "add":  ctrl.add,
                "upt":  ctrl.upt,
                "del":  ctrl.delete,
            }


        self._cmd_table = {
                "help":        self.cmd_show_help,
                "rule.filter": self.cmd_rule_filter,
                "rule.color":  self.cmd_rule_color,
                "filter":      self.cmd_filter,
                "server":      self.cmd_server,
                "data":        self.cmd_data,
                "show.ctrl":   self.cmd_show_ctrl,
                "show.intf":   self.cmd_show_intf,
                "show.action": self.cmd_show_action_table,
                "exit":        self.cmd_exit
            }

    def check_msg_queue(self):
        while True:
            try:
                _id = self.mq_gui.get_nowait()
            except queue.Empty:
                self.root.after(100, self.check_msg_queue)
                break
            else:
                if isinstance(_id, str):
                    #msg = _id
                    #self.listbox.insert(tkinter.END, msg)
                    msg = DbgMessage("local_ID", "from_self" , "server_is_self", _id)
                    self.actionctl.gui_action(msg, self.listbox)
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


    # cmd
    def cmd_entry_evt(self, evt = None):
        '''
        cmd args,args,k=v,k=x,....
        '''
        if evt.keysym in ("Up", "Down"):
            self.cmd_entry.delete(0, tkinter.END)
            if not self.history:
                return

            if evt.keysym == "Up":
                self.history_pos -= 1
                if self.history_pos < -len(self.history):
                    self.history_pos = -len(self.history)

                self.cmd_entry.insert(0, self.history[self.history_pos])
                return

            if evt.keysym == "Down":
                self.history_pos += 1
                if self.history_pos >= 0:
                    self.history_pos = 0
                    return

                self.cmd_entry.insert(0, self.history[self.history_pos])
                return

        if not self.cmd_entry.get().strip():
            return 0

        cmds = self.cmd_entry.get().split()

        if not cmds:
            self.cmd_entry.delete(0, tkinter.END)
            return 1

        _cmd = cmds[0].strip()
        args = []
        kwargs = {}

        if len(_cmd) > 1:
            ln_args = "".join(cmds[1:])

            for ln_args in cmds[1:]:
                for item in ln_args.split(','):

                    if not item:
                        continue

                    items = item.split('=')

                    if len(items) > 2:
                        dbg_print(f"ERROR args: {items}")
                        return 0

                    if len(items) == 2:
                        kwargs[items[0]] = items[1]

                    if len(items) == 1:
                        args.append(items[0])


        self.history.append(" ".join(cmds))
        self.history_pos = 0

        dbg_print(f">>> {_cmd} <<< {args} , {kwargs}")
        self.get_cmd(_cmd)(*args, **kwargs)

        self.cmd_entry.delete(0, tkinter.END)

    def cmd_error(self, *args, **kwargs):
        dbg_print("Command Error. input 'help' to get supported commands list.")

    def cmd_show_help(self, *args, **kwargs):
        "                   show help lists"
        dbg_print("Current support cmds:")

        for key,val in self._cmd_table.items():
            dbg_print(f"        {key} {str(val.__doc__)}")

    def cmd_rule_filter(self, *args, **kwargs):
        "show [id] | add | upt | del rulename           filter rule control"

        if len(args) <= 0:
            return

        cmd = args[0]
        cmd_entry = self.rule_filter_table.get(cmd, None)

        if not cmd_entry:
            dbg_print(f"error: rule filter argument '{cmd}' invalid")
            return False

        return cmd_entry(*args[1:], **kwargs)

    def cmd_rule_color(self, *args, **kwargs):
        "show [id] | add | del id"

        if len(args) <= 0:
            return

        cmd = args[0]
        cmd_entry = self.rule_color_table.get(cmd, None)

        if not cmd_entry:
            dbg_print(f"error: rule color argument '{cmd}' invalid")
            return False

        return cmd_entry(*args[1:], **kwargs)


    def cmd_filter(self, *args, **kwargs):
        "show | clear"

        if len(args) <= 0:
            return

        self.app.app_exec(f"filter.{args[0]}")

    def cmd_server(self, *args, **kwargs):
        "show | clear"
        if len(args) <= 0:
            return

        self.app.app_exec(f"server.{args[0]}")

    def cmd_data(self, *args, **kwargs):
        "show | clear | delete"

        if len(args) <= 0:
            return

        cmd = args[0]
        if cmd == "show":
            return self.datactl.show()

        if cmd == "clear":
            return self.datactl.clear()

        if cmd == "delete":
            for k in args[1:]:
                self.datactl.delete(k)


    def cmd_show_ctrl(self, *args, **kwargs):
        "[id]"
        dbg_ctrl_show(*args, **kwargs)

    def cmd_show_intf(self, *args, **kwargs):
        "[id]"
        dbg_intf_show(*args, **kwargs)

    def cmd_show_action_table(self, *args, **kwargs):
        "[table]"
        self.actionctl.show(*args, **kwargs)

    def cmd_exit(self, *args, **kwargs):
        ""
        return self.root.quit()

    def get_cmd(self, cmd=None):

        return self._cmd_table.get(cmd, self.cmd_error)

    def do_init_widget(self):
        self.prepare_menu()
        self.frame    = tkinter.Frame(self.root)
        self.frame_cmd = tkinter.Frame(self.frame)
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

        self.frame_cmd.pack(side=tkinter.TOP, fill=tkinter.X)
        self.cmd_label = tkinter.Label(self.frame_cmd, text='   Command: ', **self.font_cfg)
        self.cmd_entry = tkinter.Entry(self.frame_cmd, **self.font_cfg)
        self.cmd_entry.bind('<Return>', self.cmd_entry_evt)
        self.cmd_entry.bind('<Up>', self.cmd_entry_evt)
        self.cmd_entry.bind('<Down>', self.cmd_entry_evt)
        # up
        # down
        self.cmd_entry.focus()


        self.cmd_label.pack(side=tkinter.LEFT)
        self.cmd_entry.pack(side=tkinter.LEFT, fill=tkinter.X, expand=tkinter.YES)

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
        self.datactl.clear()

    def prepare_menu(self):

        self.menu_root = tkinter.Menu(self.root, **self.font_cfg)

        self.menu_file_prepare()
        self.menu_edit_prepare()
        self.menu_color_prepare()
        self.menu_filter_prepare()
        self.menu_help_prepare()

        self.root.config(menu=self.menu_root)

    def menu_file_prepare(self):

        # file menu
        filemenu = tkinter.Menu(self.menu_root, tearoff=0, **self.font_cfg)
        filemenu.add_command(label='Open', command=self.command_cb)
        filemenu.add_separator()
        filemenu.add_command(label='Exit', command=self.root.quit)

        self.menu_root.add_cascade(label='File', menu=filemenu)
        self.filemenu = filemenu

    def edit_show_or_not(self, mod):
        ctrl = dbg_controller(mod)
        ctrl.enable = self.cache.get(mod.name).get()

    def menu_edit_prepare(self):

        # edit menu
        editmenu = tkinter.Menu(self.menu_root, tearoff = 0, **self.font_cfg)

        edit_show_cb = {mod.name: partial(lambda x: self.edit_show_or_not(x), x=mod) for mod in cfg_table_module_common + cfg_table_module_post}

        for mod in cfg_table_module_common + cfg_table_module_post:

            val = tkinter.BooleanVar()
            editmenu.add_checkbutton(label=mod.name, onvalue=True, offvalue=False, variable=val, command=edit_show_cb.get(mod.name))

            self.cache[mod.name] = val
            self.cache[mod.name].set(dbg_controller(mod).enable)

        # clear
        editmenu.add_separator()
        editmenu.add_command(label="clear", command=self.clear_screen)
        self.menu_root.add_cascade(label="Edit", menu=editmenu)

        self.editmenu = editmenu

    def menu_color_cb(self):

        self.colormenu.delete(0, tkinter.END)

        for color in config.Color:
            self.colormenu.add_checkbutton(label=color.get('name'), command=self.command_cb, onvalue=True, offvalue=False)
            self.colormenu.entryconfig(self.colormenu.index(color.get('name')), foreground=color.get('fg'))

        self.colormenu.add_separator()
        self.colormenu.add_checkbutton(label="Config", command=self.command_cb, onvalue=True, offvalue=False)

    def menu_color_prepare(self):

        # color menu
        colormenu = tkinter.Menu(self.menu_root, tearoff = 0, **self.font_cfg, postcommand = self.menu_color_cb)

        self.menu_root.add_cascade(label="Color", menu=colormenu)
        self.colormenu = colormenu


    def menu_filter_cb(self):
        self.filtermenu.delete(0, tkinter.END)

        for item in config.Filter:
            self.filtermenu.add_checkbutton(label=item.get('name'), command=self.command_cb, onvalue=True, offvalue=False)
            if item.get('target').lower() == 'drop':
                self.filtermenu.entryconfig(self.filtermenu.index(item.get('name')), foreground='red')
            else:
                self.filtermenu.entryconfig(self.filtermenu.index(item.get('name')), foreground='green')

        self.filtermenu.add_separator()
        self.filtermenu.add_checkbutton(label="Config", command=self.command_cb, onvalue=True, offvalue=False)

    def menu_filter_prepare(self):
        # filter menu
        filtermenu = tkinter.Menu(self.menu_root, tearoff = 0, **self.font_cfg, postcommand = self.menu_filter_cb)

        self.menu_root.add_cascade(label="Filter", menu=filtermenu)
        self.filtermenu = filtermenu

    def menu_help_prepare(self):
        # help menu
        helpmenu = tkinter.Menu(self.menu_root, tearoff=0, **self.font_cfg)
        helpmenu.add_command(label="About", command=self.command_cb)
        self.menu_root.add_cascade(label="Help", menu=helpmenu)
        self.helpmenu = helpmenu
