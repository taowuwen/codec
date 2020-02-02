#!/usr/bin/env python3
# -*- coding: utf-8 -*-


MatchResult = enum.Enum('MatchResult', 'equal not_equal contain not_contain')


class Rule:

    def __init__(self, rule_str = None, ignorecase = False, target = ActionTarget.DROP):
        self._rule = rule_str
        self._ignorecase = ignorecase
        self._stat_match = 0
        self._tgt = target

    def match(self, match_action, msg):
        return match_action(self._rule, msg)

    def __str__(self):
        return f'{self.__class__.__name__} [rule: {self._rule}, ignorecase: {self._ignorecase}, count: {self._stat_match}]'

    def __repr__(self):
        return self.__str__()

    def matched(self):
        self._stat_match += 1
        return self._tgt

    def not_matched(self):
        return ActionTarget.CONTINUE

class FilterRule(Rule):

    def __init__(self, *args, **kwargs):
        super().__init__(**kwargs)

    def matched(self):
        pass

    def not_matched(self):
        pass



class ColorRule(Rule):
    def __init__(self, *args, **kwargs):
        super().__init__(**kwargs)

    def matched(self):
        pass

    def not_matched(self):
        pass
