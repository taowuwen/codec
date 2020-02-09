#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import time
import tkinter
from pprint import pprint

from dbgfactory import BuildFactoryAutoRegister, BuildFactory
from dbgconfig import config
from dbgrule import FilterRule, ColorRule

from dbgactiondef import ActionType, ActionTarget, ActionFilterType, action_filter_type, action_target_type



class Action:

    _type_act = None

    def __call__(self, *args, **kwargs):
        pass

    def __str__(self):
        return f'{self.__class__.__name__}'

    def __repr__(self):
        return self.__str__()

'''
    actions for common tables
'''
class ActionCommon(Action):
    _type_act = ActionType.common
    pass


class ActionCommonShowLineNumber(ActionCommon):
    _config = "show_line_number"
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        msg.prefix += '{:0>5d}   '.format(listbox.size())

class ActionCommonShowTimeStamp(ActionCommon):
    _config = "show_timestamp"
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        msg.prefix += f' {time.ctime(msg.tm)}'

'''
string format
    "{:<10s}{:>10d}".format(word, number)
'''

'''
    actions for post config
'''
class ActionPostConfig(Action):
    _type_act = ActionType.postconfig
    pass

class ActionPostListBoxInsert(ActionPostConfig):
    _config = "set_listbox"
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        listbox.insert(tkinter.END, str(msg))

class ActionPostEnableLog(ActionPostConfig):
    _config = "enable_log"
    def __call__(self, *args, **kwargs):
        msg, listbox, *_ = args
        print(msg)

'''
    action for colors
'''
class ActionColor(Action):
    _type_act = ActionType.color

    def __init__(self, *args, **kwargs):
        self._rule, *_ = args
        super().__init__(*args[1:], **kwargs)

    def __call__(self, *args, **kwargs):

        msg, listbox, *_ = args

        if self._rule.match(self.match, msg):
            self._rule.matched()
            listbox.itemconfig(tkinter.END, **self._rule.itemconfig)


class ActionColorMatch(ActionColor):
    _config = "color"

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule in msg


class ActionColorMatchIgnorecase(ActionColor):
    _config = "color_ignorecase"

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule.lower() in msg.lower()


class ActionFilter(Action):
    _type_act = ActionType.filter

    def __init__(self, *args, **kwargs):
        self._rule, *_ = args

        super().__init__(*args[1:], **kwargs)

    def __call__(self, *args, **kwargs):
        if self._rule.match(self.match, *args, **kwargs):
            return self._rule.matched()
        else:
            return self._rule.not_matched()

    def match(self, *args, **kwargs):
        return ActionTarget.DROP

class ActionFilterEqual(ActionFilter):
    _config = ActionFilterType.equal.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule == msg


class ActionFilterNotEqual(ActionFilter):
    _config = ActionFilterType.not_equal.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule != msg

class ActionFilterContain(ActionFilter):
    _config = ActionFilterType.contain.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule in msg

class ActionFilterNotContain(ActionFilter):
    _config = ActionFilterType.not_contain.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule not in msg

class ActionFilterEqualIgnorecase(ActionFilter):
    _config = ActionFilterType.equal_ic.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule.lower() == msg.lower()

class ActionFilterNotEqualIgnorecase(ActionFilter):
    _config = ActionFilterType.not_equal_ic.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule.lower() != msg.lower()

class ActionFilterContainIgnorecase(ActionFilter):
    _config = ActionFilterType.contain_ic.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule.lower() in msg.lower()

class ActionFilterNotContainIgnorecase(ActionFilter):
    _config = ActionFilterType.not_contain_ic.name

    def match(self, *args, **kwargs):
        rule, msg, *_ = args
        return rule.lower() not in msg.lower()

class ActionFactoryBuilder(BuildFactory):

    def __init__(self, name):
        super().__init__(name)

        self.do_register()

    def do_register(self):

        for klass in Action.__subclasses__():
            for _cls in klass.__subclasses__():

                if hasattr(_cls, '_config'):
                    self.register(_cls._config, _cls)
                else:
                    self.register(_cls.__name__, _cls)

        pprint(self.products)


class ActionManagement:
    def __init__(self):
        self.action_builder = ActionFactoryBuilder('Action')
        self._build_action_table()

    def _build_action_table(self):

        self.build_action_table_common()
        self.build_action_table_post()
        self.build_action_table_filter()
        self.build_action_table_color()

    def build_action_table_common(self):

        self.action_table_common = []

        for key in config.common:
            if config.common.get(key, False):
                self.action_table_common.append(self.action_builder.create(key))

        print("common_table: ", self.action_table_common)

    def build_action_table_post(self):
        self.action_table_post = []

        for key in config.postconfig:
            if config.postconfig.get(key, False):
                self.action_table_post.append(self.action_builder.create(key))

        print("action_table: ", self.action_table_post)


    def build_action_table_color(self):
        self.action_table_color = []

        for key in config.color:
            rule = ColorRule(**key)

            if rule.ignorecase:
                act = self.action_builder.create('color_ignorecase', rule)
            else:
                act = self.action_builder.create('color', rule)

            self.action_table_color.append(act)
            print(rule)

        print("color_table", self.action_table_color)

    def build_action_table_filter(self):
        self.action_table_filter = []

        for key in config.filter:

            rule = FilterRule(**key)
            act = self.action_builder.create(action_filter_type(rule.match_condition, rule.ignorecase), rule)
            self.action_table_filter.append(act)
            print(rule)
        print("filter_table", self.action_table_filter)

    def gui_action(self, msg, listbox, **kwargs):

        for act in self.action_table_common + \
                   self.action_table_post + \
                   self.action_table_color:
            act(msg, listbox, **kwargs)


    def filter_action(self, msg, *args, **kwargs):
        
        for act in self.action_table_filter:

            tgt = act(msg, *args, **kwargs)
            if tgt is not ActionTarget.CONTINUE:
                return tgt

        return ActionTarget.DROP
                
if __name__ == '__main__':
    am = ActionManagement()
