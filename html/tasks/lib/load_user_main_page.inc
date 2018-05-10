#!/usr/bin/env bash


load_user_task()
{
	cat <<EOF
<div >
	<fieldset style="width:80%; margin:  0px auto;">
	<legend align="center"> <h2>${USER_NAME}的个人任务事项</h2> </legend>
	<table width=100% align=center border=1> 
		<thead>
			<tr>
				<th width=5%>ID</th>
				<th width=50%>标题</th>
				<th width=10%>开始时间</th>
				<th width=10%>结束时间</th>
				<th width=10%>重复方式</th>
				<th width=5%>类型</th>
				<th width=10% align="center"></th> <!-- 操作，详情/删除-->
			</tr>
		</thead>
		<tbody align="center">
			<tr>
				<th> 1</th>
				<td align="left"> <a href="detail">go shopping tomorrow</a></td>
				<td> 12/3/5 2018</td>
				<td> 12/4/5 2018</td>
				<td> 不重复</td>
				<td> 默认</td>
				<td> <a href="nothing">已完成</a>|<a href="nothing">删除</a></td>
			</tr>
		</tbody>
		<tfoot>
			<tr>
				<td> </td>
				<td> aaa </td>
				<td> bbb </td>
			</tr>
		</tfoot>
	</table>
	</fieldset>
</div>
EOF
	return 0
}

load_groups_in_db()
{
	db_list_group |
	awk  -v ticket=$USER_COOKIE -v webroot="$WEBROOT/task_main" -F\| '{
		printf("<tr>\n")
		printf("\t<td>%s</td>\n", $1)
		printf("\t<td>%s</td>\n", $2)
		printf("\t<td>")
		printf("<a href=\"%s?action=group_detail&grpid=%d\">编辑</a>", webroot, $1);
		printf("|<a href=\"%s?action=group_delete&grpid=%d\">删除</a>", webroot, $1);
		printf("\t</td>\n")
		printf("</tr\n");
	'}

	return 0;
}

load_groups() 
{
	cat <<EOF
<div >
	<fieldset style="width:80%; margin:  0px auto;">
	<legend align="center"> <h2>组信息列表</h2> </legend>

	<div>
	<input type="image" src="images/plus.png" width=20 height=20 onClick="window.location.href='/tasks/task_main?action=group_create&userid=${USER_ID}'" /> 添加组
	</div>

	<div>
	<table width=100% align=center border=1> 
		<thead>
			<tr>
				<th width=10%>ID</th>
				<th width=60%>组名</th>
				<th width=30% align="center"></th> <!-- 操作，详情/删除-->
			</tr>
		</thead>
		<tbody align="center">
			`load_groups_in_db`
		</tbody>
		<tfoot>
		</tfoot>
	</table>
	</div>
	</fieldset>
</div>
EOF
	return 0
}

load_group_tasks()
{
	cat <<EOF
<div >
	<fieldset style="width:80%; margin:  0px auto;">
	<legend align="center"> <h2>用户任务事项</h2> </legend>
	<table width=100% align=center border=1> 
		<thead>
			<tr>
				<th width=5%>ID</th>
				<th width=50%>标题</th>
				<th width=10%>开始时间</th>
				<th width=10%>结束时间</th>
				<th width=10%>重复方式</th>
				<th width=5%>类型</th>
				<th width=10% align="center"></th> <!-- 操作，详情/删除-->
			</tr>
		</thead>
		<tbody align="center">
			<tr>
				<th> 1</th>
				<td align="left"> <a href="detail">go shopping tomorrow</a></td>
				<td> 12/3/5 2018</td>
				<td> 12/4/5 2018</td>
				<td> 不重复</td>
				<td> 默认</td>
				<td> <a href="nothing">已完成</a>|<a href="nothing">删除</a></td>
			</tr>
		</tbody>
		<tfoot>
			<tr>
				<td> </td>
				<td> aaa </td>
				<td> bbb </td>
			</tr>
		</tfoot>
	</table>
	</fieldset>
</div>
EOF
	return 0
}


load_users_in_db()
{
	db_list_all_users |
	awk  -v ticket=$USER_COOKIE -v webroot="$WEBROOT/task_main" -F\| '{
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
				printf("<a href=\"%s?action=user_lock&userid=%d\">锁定</a>", webroot, $1);
			} else {
				printf("<a href=\"%s?action=user_unlock&userid=%d\">解锁</a>", webroot, $1);
			}
			
			printf("|<a href=\"%s?action=user_delete&userid=%d\">删除</a>\n", webroot, $1);
			printf("</td>\n");
		}
		printf("</tr>\n");
	}'
}

load_users()
{
	cat <<EOF
<div >
	<fieldset style="width:80%; margin:  0px auto;">
	<legend align="center"> <h2>用户列表</h2> </legend>
	<table width=100% align=center border=1> 
		<thead>
			<tr>
				<th width=10%>ID</th>
				<th width=10%>用户名</th>
				<th width=20%>email</th>
				<th width=10%>电话号码</th>
				<th width=10%>状态</th>
				<th width=10%>管理员</th>
				<th width=10%>启用通知</th>
				<th width=20% align="center"></th> <!-- 操作，锁定/解锁/删除-->
			</tr>
		</thead>
		<tbody align="center">
			`load_users_in_db`
		</tbody>
		<tfoot>
		</tfoot>
	</table>
	</fieldset>
</div>
EOF
}

default_cb()
{
	cat <<EOF
	<p> YOU SHOULD PROVIDE A CALLBACK METHOD </p>
EOF
}

do_load_page()
{
	local cb=${1:-default_cb}
	local goback=${2:-1}
	local active=${3:-nothing}

	echo -en "Content-Type: text/html\r\n\r\n"

	http_main_head $active

	log_trace "$USER_NAME, $USER_COOKIE, $USER_ID, $USER_ISADMIN"

	[ $goback -eq 1 ] && {
	cat <<EOF
	<div style="width:80%; margin:  0px auto;">
	<p><a href="javascript:history.back()"> <img src="images/go_back.jpg" width=100/></a> </p>
	</div>
EOF
	}

	$cb

	http_main_tail

	return 0

}


do_load_user_main_page()
{
	load_main_page() {
		load_user_task

		[ $USER_ISADMIN -eq 1 ] && {
			load_groups
			load_users
		}
	}

	do_load_page load_main_page 0 home

	return 0
}

load_group_create()
{
	cat <<EOF

	<div>
<script type="text/javascript">

function check_form(frm){

	var frm = document.getElementById("frm_reg");

	//var re = new RegExp('^[a-zA-Z0-9_]\+$');
	var re = new RegExp("^\w+$");
	if(!re.test(frm.user.value)) {
		alert("组名不合法");
		frm.user.focus();
		return false;
	}

	return true;
}

</script>

	<form action="/tasks/task_main?action=group_add" method=POST onsubmit="return check_form(this);">
	<table width=600 align="center" border=1>
	<tr align="center"> <td colspan=2> 添加组 </td> </tr>
	<tr> 
		<td width=30%> 组名 </td> 
		<td width=60%> <input id="grp_id" type="text" name="groupname" required="reqired"/>*</td>
	</tr>
	<tr> 
		<td>  </td> 
		<td> <input type="submit" value="确定"/>
	</tr>
	</table>
	</form>
	</div>
EOF
	return 0
}


load_group_detail()
{
	_grp_detail() {
		cat <<EOF
		<p> tasks....</p>
		<p> Users </p>
EOF

	}
	do_load_page _grp_detail
	return 0
}

load_user_notification()
{
	_user_notification() {
		cat <<EOF
		<p> user notifications........</p>
EOF

	}
	do_load_page _user_notification 0 notify
	return 0
}

load_user_details()
{
	_user_detail() {
		cat <<EOF
	<div style="width:80%; margin:  0px auto;">
		<p> user details </p>
		<p> user details........</p>
	</div>
EOF
	}
	do_load_page _user_detail 0 setting
	return 0
}
