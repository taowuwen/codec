#!/usr/bin/env bash

./dptrp1.py --client-id conf/deviceid.dat --key conf/privatekey.dat --addr digitalpaper $*
#echo ./dptrp1.py --client-id conf/deviceid.dat --key conf/privatekey.dat --serial  `cat conf/workspace.dat` $*
#./dptrp1.py --client-id conf/deviceid.dat --key conf/privatekey.dat --serial  `cat conf/workspace.dat` $*
