#!/usr/bin/env python3
# -*- coding: utf-8 -*-


#   def my_gen_char(s = 'a', e = 'z'):
#       n = s
#
#       while n < e:
#           yield n
#           tmp = ord(n) + 1
#           n   = chr(tmp) 
#
#
#
#   genchar = my_gen_char('a', 'f')
#
#   for n in genchar:
#       print(n)
#
#
#   for n in my_gen_char('e', 'i'):
#       print(n, " \t", hex(ord(n)) )


#   def document_increase(func):
#
#       def inner_increase(*keys, **kwargs):
#           print('Running Function: ', func.__name__)
#           print('Positional args: ', keys)
#           print('Keyword arguments: ', kwargs)
#           res = func(*keys, **kwargs)
#           print('Result: ', res)
#           return res
#       return inner_increase
#
#   @document_increase
#   def increase(a, s = 1):
#       return a + s
#
#
#   print(increase(1, s=2))


#   animal = 'fruitbat'
#
#   def print_global():
#       print('locals', locals())
#       print('globals', globals())
#       print("inside print_global animal", animal)
#
#
#   print_global()
#   print("1 global animal", animal, "id animal: ", id(animal))
#
#
#
#   def change_and_update():
#       animal = 'cat';
#       print("inside change_and_update animal ", animal, id(animal))
#       print('locals', locals())
#       print('globals', globals())
#
#
#   change_and_update()
#   print("2 global animal", animal, "id animal: ", id(animal))
#
#   def change_global():
#       global animal
#       animal = 'pig'
#       print("inside change_global animal ", animal, id(animal))
#       print('locals', locals())
#       print('globals', globals())
#
#   change_global()
#   print("3 global animal", animal, "id animal: ", id(animal))


#   def amazing():
#       '''hello, this is a test for amazing'''
#       print('this function is named: ', amazing.__name__)
#       print('this function docstring is: ', amazing.__doc__)
#       print('locals: ',  locals())
#       print('globals: ', globals())
#
#
#
#   amazing()
#   print(dir())


#   my_list = []
#
#   while True:
#       num = input("input a number(q for quit): ")
#
#       if num == 'q':
#           break
#
#       try:
#           my_list.append(int(num))
#
#           print('current list:', my_list)
#
#       except Exception as e:
#           print('something wrong: ', e)


#   class MyUppercaseException(Exception):
#       pass
#
#
#   words = ['eeenie', 'meeenie', 'mindfa', 'Abc', 'MO']
#
#   for word in words:
#       if word.isupper():
#           raise MyUppercaseException(word)
#

#   try:
#       raise MyUppercaseException('panic')
#   except MyUppercaseException as e:
#       print(e)


#   guess_me = 7
#
#   val = 7
#   if val < guess_me:
#       print("too low!")
#   elif val > guess_me:
#       print("too high!")
#   else:
#       print("just right")
#


#   guess_me = 7
#   start = 10
#
#   while True:
#       if start < guess_me:
#           print("too low", start, "<", guess_me)
#           start += 1
#
#       elif start == guess_me:
#           print("found it!")
#           break
#
#       else:
#           print("too high", start, ">", guess_me)
#           start -= 1


#   for n in range(3, -1, -1):
#       print(n)


#   even = [n for n in range(10) if n % 2 == 0 ]
#   print(even)


#   squares = {s: s*s for s in range(10)}
#   squares = {s: pow(s, 2) for s in range(10)}
#   print(squares)

#   odd = {o for o in range(10) if o % 2 == 1}
#   print(odd)


#   my_gen = ('Got ' + str(n) for n in range(10) )
#
#   print(type(my_gen))
#
#   for item in my_gen:
#       print(item)


#   def good():
#       return ['Harry', 'Ron', 'Hermione']
#
#   print(good())


#   def get_odds():
#       for n in range(10):
#           if n % 2 == 1:
#               yield n
#
#   for item in get_odds():
#       print(item)


#   def test(func):
#       def inner_test(*args, **kwargs):
#           print("start ", func.__name__)
#           print("args ", args)
#           print("kwargs ", kwargs)
#           res = func(*args, **kwargs)
#           print("result ", res)
#           print("end",  func.__name__)
#           return res
#       return inner_test
#
#
#   @test
#   def my_des(a, b = 1):
#       return a - b
#
#   my_des(a = 20)


class OopsExceptoin(Exception):
    pass


try:
    print("hello, OopsException")
#    raise OopsExceptoin('test')
except OopsExceptoin as e:
    print('Caught an oops')
except Exception as e:
    print('Unknown error', e)
else:
    print("hello, nothing happen")
finally:
    print("finally, nothing")

col  = ["title", "plots"]
row  = ['Creature of Habit', 'A nun turns into a mon ster']
row2 = ['Crewel Fate', 'A haunted yarn shop']


movies = {}

for t, r1, r2 in zip(col, row, row2):
    l = movies.get(t)

    if l == None:
        movies[t] = []

    movies[t].append(r1)
    movies[t].append(r2)

print(movies)

#   movies = {
#       "title": ['Creature of Habit', 'Crewel Fate'],
#       'plots': ['A nun turns into a mon ster', 'A haunted yarn shop']
#       }

