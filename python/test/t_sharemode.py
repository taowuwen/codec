#!/usr/bin/env python3
# -*- coding: utf-8 -*-


import weakref

class CarModel:

	_models = weakref.WeakValueDictionary()

	def __new__(cls, model_name, *args, **kwargs):
		model = cls._models.get(model_name)

		if not model:
			model = super().__new__(cls)
			cls._models[model_name] = model

		return model


	def __init__(self, model_name, 
			air = False,
			tilt = False, 
			cruise_control = False,
			power_locks = False,
			alloy_wheels = False,
			usb_charger = False):
		if not hasattr(self, "initted"):
			self.model_name = model_name
			self.air = air
			self.tilt = tilt
			self.cruise_control = cruise_control
			self.power_locks = power_locks
			self.alloy_wheels = alloy_wheels
			self.usb_charger = usb_charger
			self.initted = True

	def check_serial(self, serial_number):
		print("Sorroy, we are unable to check the serial number {0} and the {1} at this time".format(serial_number, self.model_name))


class Car:
	def __init__(self, model, color, serial):
		self.model = model
		self.color = color
		self.serial = serial


	def check_serial(self):
		return self.model.check_serial(self.serial)



def main():
	print("hello, test share mode")


	dx = CarModel("FIT DX")
	lx = CarModel("FIT LX", air = True, cruise_control = True, power_locks= True, tilt = True)

	car1 = Car(dx, "blue", "123456")
	car2 = Car(dx, "green", "123654")
	car3 = Car(lx, "red", "123465")

	print("id(dx) = {}".format(id(dx)))
	print("id(lx) = {}".format(id(lx)))

	del lx
	del car3
	import gc
	print(gc.collect())

	lx = CarModel("FIT LX", air = True, cruise_control = True, power_locks= True, tilt = True)

	print("id(dx) = {}".format(id(dx)))
	print("id(lx) = {}".format(id(lx)))

	lx = CarModel("FIT LX")

	print("id(dx) = {}".format(id(dx)))
	print("id(lx) = {}".format(id(lx)))

	print("car air: {} ".format(lx.air))





if __name__ == '__main__':
	main()
