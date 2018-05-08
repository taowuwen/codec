#!/usr/bin/env bash


cgi_root=$(cd `dirname $0`/../; pwd)

. db.inc

#get_user_by_telephone 18108080776
#get_user_by_name tww
#get_user_by_name test
#get_user_by_email taowuwen@gmail.com

add_user taowuwen 123456 a@gmail.com && echo "success" || echo "failed"
