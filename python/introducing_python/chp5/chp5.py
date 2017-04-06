#!/usr/bin/env python3
# -*- coding: utf-8 -*-


#   import sys
#   print("Program arguments: ", sys.argv)

#   import report
#
#   print(report.get_description())
#   print(report.get_description_v1())


#   import report as wr
#   print(wr.get_description())
#
#   from report import get_description_v1 as reportor_v1
#
#   print(reportor_v1())

#   import sys
#   for path in sys.path:
#       print(path)
#
#   from sources import daily, weekly
#
#   print("Daily forecast:", daily.forecast())
#   print("Weekly forecast:")
#
#   for number, outlook in enumerate(weekly.forecast(), 1):
#       print(number, outlook)


#   periodic_table = {'Hydrogen': 1, "Helium": 2}
#   print(periodic_table)
#
#   carbon = periodic_table.setdefault("Carbon", 2)
#   print(carbon)
#   print(periodic_table)
#
#   helium = periodic_table.setdefault("Helium", 1111)
#   print(helium)
#   print(periodic_table)

#   from collections import defaultdict

#   periodic_table = defaultdict(int)
#
#   periodic_table['Hydrogen'] = 1
#   periodic_table['Lead']
#   print(periodic_table)
#
#
#   def no_idea():
#       return "Huh?"
#
#
#   print(id(no_idea))
#
#   best = defaultdict(no_idea)
#
#   best['aaaa'] = "hello"
#   print(best)
#   print(best["hello"])
#   print(best["world"])
#   print(best)


#   from collections import defaultdict
#
#   food_counter = defaultdict(int)
#   for food in ['spam', 'spam', 'eggs', 'spam']:
#       food_counter[food] += 1
#
#
#   print(food_counter)
#   for food, counter in food_counter.items():
#       print(food, counter)


#
#
#   from collections import Counter
#
#   breakfast = ['spam', 'spam', 'eggs', 'spam']
#   breakfast_counter = Counter(breakfast)
#
#   print(breakfast_counter)
#   print(breakfast_counter.most_common())
#   print(breakfast_counter.most_common(1))
#
#
#   lunch = ["eggs", "eggs", "bacon"]
#   lunch_counter = Counter(lunch)
#
#   print(lunch_counter)
#   print(breakfast_counter + lunch_counter)
#   print(breakfast_counter - lunch_counter)
#   print(lunch_counter - breakfast_counter)
#
#   print(breakfast_counter & lunch_counter)
#   print(breakfast_counter | lunch_counter)



#   quotes = {
#           'Moe': 'A wise guy, huh?',
#           'Larry': 'Ow!',
#           'Curly': 'Nyuk nyuk',
#           }
#
#   for q in quotes:
#       print(q)



#from collections import OrderedDict
#quotes = OrderedDict([
#    ('Moe', 'A Wise guy, huh?'),
#    ('Larry', 'Ow!'),
#    ('Curly', 'Nyuk nyuk')
#    ])
#
#   for q in quotes:
#       print(q)


#   from collections import deque
#   def palindrome(word):
#       dq = deque(word)
#       while len(dq) > 1:
#           if dq.popleft() != dq.pop():
#               return False
#
#       return True
#
#
#   print(palindrome(''))
#   print(palindrome('aaa'))
#   print(palindrome('aba'))
#   print(palindrome('daca'))
#   print(palindrome('feadaef'))
#   print(palindrome('a'))

#import itertools
#for item in itertools.chain([1,2], ['a', 'b']):
#    print(item)

#for item in itertools.cycle([1,2]):
#    print(item)


#for item in itertools.accumulate([1,2,3,4,5]):
#    print(item)


#def multiply(a,b):
#    return a * b


#for item in itertools.accumulate([1,2,3,4,5,6,], multiply):
#    print(item)


#from pprint import pprint

#print(quotes)
#pprint(quotes)

#   import zoo
#   zoo.hours()


#   import zoo as menagerie
#   menagerie.hours()


#   from zoo import hours
#   hours()


#   from zoo import hours as info
#   info()


#   plain = {
#           'a': 1,
#           'b': 2,
#           'c': 3,
#           }
#
#   print(plain)
#
#
#   from collections import OrderedDict
#
#   or_plain = OrderedDict([
#       ('a', 1),
#       ('b', 2),
#       ('c', 3),
#       ])
#
#   print(or_plain)


from collections import defaultdict

dict_of_lists = defaultdict(list)
dict_of_lists['a'].append('something for a')

print(dict_of_lists)



