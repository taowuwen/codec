#!/usr/bin/env bash



do_load_user_main_page()
{
	echo -en "Content-Type: text/html\r\n\r\n"

	http_main_head

	cat <<EOF
<h1 align="center"> 用户${USER_NAME} _ ${USER_EMAIL} 登录成功"!!! </h1> 
EOF
	http_main_tail

	return 0

}
