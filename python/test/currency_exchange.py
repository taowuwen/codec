#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import os

config_money = [
    ('RMB', 'China'),
    ('D', 'USA'),
    ('EUR', 'Europe')
]

config_rate = [
    ('RMB', 'D', '7.01'), 
    ('RMB', 'EUR', '10.021')
]


class Currency:

    def __init__(self, name="RMB", country="CN", num=100):
        self.num     = num
        self.name    = name
        self.country = country

    @property
    def number(self):
        return self.num

    def __str__(self):
        return self.name

    @property
    def country(self):
        return self.country

    def get_money(self, cur = 'RMB'):
        pass


class ExchangeRate:
    def __init__(self, c_from, c_to, rate):

        self.currency_one = c_from
        self.currency_two = c_to
        self.num_one, self.num_two = float(rate).as_integer_ratio()

        assert self.num_one != 0 and self.num_two != 0, "num should never be zore"


    @property
    def rate(self):
        return self.num_one / self.num_two







class CurrencyExchange:

    def __init__(self):
        pass


    def exchange(self, from ,to ,rate):
        pass




if __name__ == '__main__':
    pass





