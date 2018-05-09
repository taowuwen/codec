
http_main_head()
{
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
  float: right;
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
  <a class="active" href="#home">主页</a>
  <a href="#news">消息</a>
  <a href="/tasks/setting">设置</a>
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
</dif>

</body>
</html>
EOF

}


