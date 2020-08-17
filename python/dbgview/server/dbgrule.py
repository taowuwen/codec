#!/usr/bin/env python3
# -*- coding: utf-8 -*-

from dbgactiondef import ActionType, ActionTarget, ActionFilterType, action_filter_type, action_target_type

class Rule:
    def __init__(self, rule = None, ignorecase = False, **kwargs):
        self._rule = rule
        self._ignorecase = ignorecase
        self._stat_match = 0
        self._cfg = kwargs

    def match(self, match_action, msg):
        return match_action(self._rule, msg)

    def __str__(self):
        return f'{self.__class__.__name__} [rule: "{self._rule}", ignorecase: {self._ignorecase}, count: {self._stat_match}]'

    def __repr__(self):
        return self.__str__()

    def matched(self):
        self._stat_match += 1

    def not_matched(self):
        return ActionTarget.CONTINUE

    @property
    def match_condition(self):
        return self._cfg.get('match_condition')

    @property
    def ignorecase(self):
        return self._ignorecase

    @property
    def name(self):
        return self._cfg.get('name', 'ERROR_NAME_MISSING')

class FilterRule(Rule):

    def __init__(self, *args, **kwargs):
        super().__init__(**kwargs)
        self._tgt = action_target_type(kwargs.get('target'))

    def matched(self):
        super().matched()
        return self._tgt

    def __str__(self):
        return super().__str__() + f' --> target: {self._tgt.name}, cfg: {self._cfg}'


class ColorRule(Rule):
    def __init__(self, *args, **kwargs):
        super().__init__(**kwargs)

        self.build_cfg()

    def matched(self):
        super().matched()

    def __str__(self):
        return super().__str__() + f' --> cfg: {self._cfg}'

    def build_cfg(self):
        '''
         Valid resource names: background, bg, foreground, fg, selectbackground, selectforeground.
        '''
        cfg = {}

        for key in ('background', 'bg', 'foreground', 'fg', 'selectbackground', 'selectforeground'):
            val = self._cfg.get(key, None)
            if val:
                cfg[key] = val

        self._itemconfig = cfg

    @property
    def itemconfig(self):
        return self._itemconfig

