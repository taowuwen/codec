#!/usr/bin/env bash


cgi_root=$(cd `dirname $0`/../; pwd)

. db.inc

#get_user_by_telephone 18108080776
#get_user_by_name tww
#get_user_by_name test
#get_user_by_email taowuwen@gmail.com

load_users_in_db()
{
	db_list_all_users |
	awk -v webroot="$WEBROOT/task_main" -F\| '{
		printf("<tr>\n")
		printf("\t<td>%s</td>\n", $1)
		printf("\t<td>%s</td>\n", $2)
		printf("\t<td>%s</td>\n", $4)
		printf("\t<td>%s</td>\n", $5)

		st="正常"
		if ($6 == 0)
			st="已锁定"
		
		printf("\t<td>%s</td>\n", st);

		isadmin="否";
		if ($7  == 1)
			isadmin="是";

		printf("\t<td>%s</td>\n", isadmin);

		notify="是";
		if ($8 == 0)
			notify="否"
		printf("\t<td>%s</td>\n", $8);

		isadmin=$7

		if (isadmin == 1) {
			printf("\t<td></td>\n");
		} else {
			printf("\t<td>");

			if ($6 == 1) {
				printf("<a href=\"%s?action=lock&userid=%d\">锁定</a>", webroot, $1);
			} else {
				printf("<a href=\"%s?action=unlock&userid=%d\">解锁</a>", webroot, $1);
			}
			
			printf("|<a href=\"%s?action=delete_user&userid=%d\">删除</a>\n", webroot, $1);
			printf("</td>\n");
		}
		printf("</tr>\n");
	}'
}

load_users_in_db
