#!/usr/bin/env bash

web_root="/home/tww/www"

configs=(
	$web_root/locuswifi/caches/configs/database.php
	$web_root/locuswifi/phpsso_server/caches/configs/database.php
)

for config in ${configs[@]}
do
	echo "config=$config"
done


echo "0=${configs[0]}"
echo "1=${configs[1]}"
