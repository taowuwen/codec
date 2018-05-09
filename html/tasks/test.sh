#!/usr/bin/env bash

encrypt_password()
{
	echo -en "$1" | md5sum | awk '{print $1}'
}

pass="123456"

encrypt_password "$pass"
