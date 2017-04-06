#!/usr/bin/env bash

#set -x


export PATH=$PATH:"/usr/bin/ems_c/bin"
. /usr/bin/ems_c/bin/jshn.sh


ctx=`cat /tmp/ems_result.res`

json_load "$ctx"

json_get_var id id 1
json_get_var aaa  total 100
json_get_var online online 100
json_get_var offline offline 100


echo "$id, $aaa, $online, $offline"


getcontent()
{
	echo '{ "jsonrpc": "2.0", "method": "getConf", "params": 
		{ 
			"nasInfos": [ 
				{ "configNumber": -1, "sn": "00017abbbb0f", "devType": "Yingke WiFi G1", "softVer": "300.100.00.03-1118" },
				{ "configNumber": 1, "sn": "00017abbbb1f", "devType": "Yingke WiFi G1", "softVer": "300.100.00.03-1118" } ,
				{ "configNumber": 1, "sn": "00017abbbb2f", "devType": "Yingke WiFi G1", "softVer": "300.100.00.03-1118" }, 
				{ "configNumber": 1, "sn": "00017abbbb3f", "devType": "Yingke WiFi G1", "softVer": "300.100.00.03-1118" }, 
			] 
		}, 
		"id": 3 
	}'
}

json_set_namespace REQUEST

json_load "`getcontent`"


json_select params
json_select nasInfos

arylen()
{
	l=0
	while true
	do
		[ -z "$1" ] && break
		l=$(( $l + 1))
		shift
	done

	echo $l
}

echo "length:  `arylen $keys`"

json_get_keys keys


echo "length:  `arylen $keys`"

for key in $keys
do
	json_select $key

	json_get_vars sn devType softVer configNumber

	echo "$key --> $sn $devType $softVer $configNumber"
	json_select ".."
done





#json_select test
#json_get_var another another "no"
#json_select another

# keys
#print_all_item()
#{
#	local val
#
#	for key in $keys
#	do
#		json_get_var val "$key" 0
#		
#		[ -n "$val" ] && echo "$key ---> $val"
#	done
#}


#json_get_keys keys
#print_all_item keys

#json_select 
#json_get_var mth method "not found"





#echo $method, $params $sn, $mth $another


# namespace for response data
json_set_namespace RESPONSE
json_init
json_add_string jsonrpc "2.0"
json_add_int    id      "$id"

json_set_namespace RESPONSE
json_dump


