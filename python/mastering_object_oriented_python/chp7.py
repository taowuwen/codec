#!/usr/bin/env python3


import sys

#def trace(frame, event, arg):
#	if frame.f_code.co_name.startswith("__"):
#		print( frame.f_code.co_name, frame.f_code.co_filename, event)
#
#sys.settrace(trace)
#
#
#class noisyfloat(float):
#	def __add__(self, other):
#		print(self, " + ", other)
#		return super().__add__(other)
#
#	def __radd__(self, other):
#		print(self, " radd ", other)
#		return super().__radd__(other)


import numbers
import math




class FixedPoint(numbers.Rational):
	__slots__ = ("value", "scale", "default_format")

	def __new__(cls, value, scale=100):
		self = super().__new__(cls)

		if scale == 0:
			raise "scale could not be 0"

		
		if isinstance(value, FixedPoint):
			self.value = value.value
			self.scale = value.scale

		elif isinstance(value, int):
			self.value = value
			self.scale = scale

		elif isinstance(value, float):
			self.value = int(scale * value + .5)
			self.scale = scale

		else:
			raise TypeError

		digits = int(math.log10(scale))

		self.default_format = "{{0:.{digits}f}}".format(digits=digits)

		return self

	def __str__(self):
		return self.__format__(self.default_format)

	def __repr__(self):
		return "{__class__.__name__:s}({value:d},{scale:d})".format(
			__class__=self.__class__, value=self.value, scale=self.scale)

	def __format__(self, spec):
		if spec == "":
			spec = self.default_format

		return spec.format(self.value/self.scale)

	def numerator(self):
		return self.value

	def denominator(self):
		return self.scale


	def __add__(self, other):
		if not isinstance(other, FixedPoint):
			new_scale = self.scale
			new_value = self.value + other * self.scale


		else:
			new_scale = max(self.scale, other.scale)

			new_value = (self.value * (new_scale // self.scale) + 
					other.value * (new_scale //other.scale))

		return FixedPoint(int(new_value), scale=new_scale)

	def __sub__(self, other):
		if not isinstance(other, FixedPoint):
			new_scale = self.scale
			new_value = self.value - other * self.scale

		else:
			new_scale = max(self.scale, other.scale)

			new_value = (self.value * (new_scale // self.scale) -
					other.value * (new_scale //other.scale))

		return FixedPoint(int(new_value), scale=new_scale)

	def __mul__(self, other):
		if not isinstance(other, FixedPoint):
			new_scale = self.scale
			new_value = self.value * other

		else:
			new_scale = self.scale * other.scale
			new_value = self.value * other.value

		return FixedPoint(int(new_value), scale=new_scale)


	def __truediv__(self, other):
		if not isinstance(other, FixedPoint):
			new_value = int(self.value / other)
		else:
			new_value = int(self.value / (other.value / other.scale))

		return FixedPoint(new_value, scale=self.scale)

	def __floordiv__(self, other):
		if not isinstance(other, FixedPoint):
			new_value = int(self.value // other)

		else:
			new_value = int(self.value // (other.value / other.scale))

		return FixedPoint(new_value, scale=self.scale)

	def __mod__(self, other):
		if not isinstance(other, FixedPoint):
			new_value = (self.value/self.scale) % other

		else:
			new_value = self.value % (other.value / other.scale)

		return FixedPoint(new_value, scale=self.scale)

	def __pow__(self, other):
		if not isinstance(other, FixedPoint):
			new_value = (self.value/self.scale) ** other

		else:
			new_value = (self.value/self.scale) ** (other.value/other.scale)

		return FixedPoint(int(new_value) * self.scale, scale=self.scale)


	def __abs__(self):
		return FixedPoint(abs(self.value), self.scale)

	def __float__(self):
		return self.value / self.scale

	def __int__(self):
		return int(self.value/self.scale)

	def __trunc__(self):
		return FixedPoint(math.trunc(self.value/self.scale), self.scale)

	def __ceil__(self):
		return FixedPoint(math.ceil(self.value/self.scale), self.scale)

	def __floor__(self):
		return FixedPoint(math.floor(self.value/self.scale), self.scale)

	def __round__(self, ndigits=0):
		return FixedPoint(round(self.value/self.scale, ndigits=0), self.scale)

	def __neg__(self):
		return FixedPoint(-self.value, self.scale)

	def __pos__(self):
		return self

	def __radd__(self, other):
		if not isinstance(other, FixedPoint):
			new_scale = self.scale
			new_value = other * self.scale + self.value

		else:
			new_scale = max(self.scale, other.scale)
			new_value = (other.value * (new_scale//other.scale) + 
					self.value * (new_scale // self.scale))

		return FixedPoint(int(new_value), scale = new_scale)

	def __rsub__(self, other):
		if not isinstance(other, FixedPoint):
			new_scale = self.scale
			new_value = other * self.scale - self.value

		else:
			new_scale = max(self.scale, other.scale)
			new_value = (other.value * (new_scale//other.scale) -
					self.value * (new_scale // self.scale))

		return FixedPoint(int(new_value), scale = new_scale)
	

	def __rmul__(self, other):
		if not isinstance(other, FixedPoint):
			new_scale = self.scale
			new_value = other * self.value

		else:
			new_scale = self.scale * other.scale
			new_value = self.value * other.value


		return FixedPoint(int(new_value), scale = new_scale)

	def __rtruediv__(self, other):
		if not isinstance(other, FixedPoint):
			new_value = self.scale * int(other / (self.value / self.scale))

		else:
			new_value = int( (other.value/other.scale) / self.value)

		return FixedPoint(new_value, scale = self.scale)

	def __rfloordiv__(self, other):
		if not isinstance(other, FixedPoint):
			new_value = self.scale * int(other//(self.value/self.scale))

		else:
			new_value = int((other.value/other.scale) // self.value)

		return FixedPoint( new_value, scale=self.scale)

	def __rmod__(self, other):
		if not isinstance(other, FixedPoint):
			new_value = other % (self.value/self.scale)

		else:
			new_value = (other.value/other.scale) % (self.value/self.scale)

		return FixedPoint(new_value, scale=self.scale)



	def __rpow__(self, other):
		if not isinstance(other, FixedPoint):
			new_value = other ** (self.value / self.scale)

		else:
			new_value = (other.value / other.scale) ** (self.value / selv.scale)

		return FixedPoint(int(new_value ) * self.scale, scale = self.scale)


	def __eq__(self, other):
		if not isinstance(other, FixedPoint):
			return abs(self.value/self.scale - float(other)) < 0.5/ self.scale

		else:
			if self.scale == other.scale:
				return self.value == other.value

			else:
				return self.value * other.scale // self.scale == other.value


	def __ne__(self, other):
		return not (self == other)

	def __le__(self, other):
		return self.value / self.scale <= float(other)

	def __lt__(self, other):
		return self.value / self.scale <  float(other)

	def __ge__(self, other):
		return self.value / self.scale >= float(other)

	def __gt__(self, other):
		return self.value / self.scale >  float(other)


	def __hash__(self):

		p = sys.hash_info.modulus

		m, n = self.value, self.scale

		while m % p == n % p == 0:
			m, n = m // p, n // p

		if n % p == 0:
			hash_ = sys.hash_info.inf
		else:
			hash_ = (abs(m) % p) * pow(n, p -2, p) % p


		if m < 0:
			hash_ = -hash_

		if hash_ == -1:
			hash_ = -2

		return hash_

	def round_to(self, new_scale):
		f = new_scale / self.scale
		return FixedPoint(int(self.value * f + .5), scale = new_scale)









