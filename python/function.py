#!/usr/bin/env python


class OopsException(Exception):
	pass

try:
	raise OopsException('Caught an oops')
except OopsException as err:
	print(err)


class CTestProperty():
	def __init__(self):
		self._x = 100

	@property
	def x(self):
		return self._x

	@x.setter
	def x(self, value):
		self._x = value

class CTestProperty_v2(object):
	def __init__(self):
		self._x = 100

	@property
	def x(self):
		return self._x

	@x.setter
	def x(self, value):
		self._x = value

	@x.deleter
	def x(self):
		del self._x

def test(func):
	def num_function(*args, **kwargs):
		print(func.__name__, "  start")
		result=func(*args, **kwargs)
		print(func.__name__, "  stopped")
		return result

	return num_function


@test
def myanyargs(*arg):
	print("len(arg): ", len(arg))
	for item in arg:
		print(item)



def get_odds():
	for num in range(10):
		if num % 2 == 1:
			yield num



def main():

	myanyargs(1,2,3)
	myanyargs()
	myanyargs("hello, world")


	print(list(get_odds()))

	for item in get_odds():
		print(item)

	p = CTestProperty()

	print("x is: %d"%(p.x))

	p.x = 10
	print("x is: %d"%(p.x))

	p = CTestProperty_v2()

	print("x is: %d"%(p.x))

	p.x = 10
	print("x is: %d"%(p.x))

	del p.x


	print()
	print("--------------------------------------------------------------------------------")


	titles=['Creature of Habit', 'Crewel Fate']
	plots = ['A num turns into a moster', 'A haunted yarn shop']

	movies = dict(zip(titles, plots))

	print(movies)
	print(titles, plots)


	print("--------------------------------------------------------------------------------")
	print()



if __name__ == '__main__':
	main()



