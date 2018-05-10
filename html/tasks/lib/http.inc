
# active node
http_main_head()
{
	local active_${1:-nothing}=1

	cat <<EOF
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
<style>
body {
  margin: 0;
  font-family: Arial, Helvetica, sans-serif;
}

.topnav {
  overflow: hidden;
  background-color: #333;
}

.topnav a {
  float: left;
  color: #f2f2f2;
  text-align: center;
  padding: 14px 16px;
  text-decoration: none;
  font-size: 17px;
}

.topnav a:hover {
  background-color: #ddd;
  color: black;
}

.topnav a.active {
  background-color: #4CAF50;
  color: white;
}
</style>
</head>
<body>

<div class="topnav">
  <a `[ "$active_home" = "1" ] && echo "class=\"active\""` href="/tasks/task_main">主页</a>
  <a `[ "$active_notify" = "1" ] && echo "class=\"active\""` href="/tasks/task_main?action=notification">消息</a>
  <a `[ "$active_setting" = "1" ] && echo "class=\"active\""` href="/tasks/task_main?action=setting">`[ $USER_ISADMIN -eq 1 ] && echo "管理员" || echo "普通用户"`(${USER_NAME})</a>
  <a href="/tasks/logout">退出</a>
</div>

<div style="padding-left:16px">

EOF
}


http_main_tail()
{
	cat <<EOF
</div>

<div style="padding-left:16px">
	<p align="center"> Developped by taowuwen@gmail.com</p>
</div>

</body>
</html>
EOF

}

