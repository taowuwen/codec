<%@page language="java" contentType="text/html; charset=utf-8" pageEncoding="utf-8"%>

<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
	<title>课程信息列表</title>
</head>

<body>

<table width=100%>
	<thead>
	<tr> 
		<th colspan=7> 课程信息列表 </th>
	</tr>
	<tr align="left"> 
		<th align="left">名称</th>
		<th>性质</th>
		<th>学分</th>
		<th>开设年级</th>
		<th>开设专业</th>
		<th></th>
		<th></th>
	</tr>
	</thead>
	<tbody align="left">
	<tr> 
		<th align="left">Java程序设计</th>
		<th>专业必修课</th>
		<th>3</th>
		<th>2015</th>
		<th>软件工程</th>
		<th align="right"><a href="course_edit.jsp">编辑</a></th>
		<th align="left"><a href="course_del.jsp">删除</a></th>
	</tr>
	<tr> 
		<th align="left">Web应用程序设计</th>
		<th>专业必修课</th>
		<th>3</th>
		<th>2015</th>
		<th>软件工程</th>
		<th align="right"><a href="course_edit.jsp">编辑</a></th>
		<th align="left"><a href="course_del.jsp">删除</a></th>
	</tr>
	<tr> 
		<th align="left">Web应用程序设计</th>
		<th>专业必修课</th>
		<th>3</th>
		<th>2015</th>
		<th>软件工程</th>
		<th align="right"><a href="course_edit.jsp">编辑</a></th>
		<th align="left"><a href="course_del.jsp">删除</a></th>
	</tr>
	</tbody>

	<tfoot align="left">
	<tr> 
		<th align="left"><a href="course_add.jsp">添加</a></th>
		<th></th>
		<th></th>
		<th></th>
		<th><a href="reload.html">刷新数据列表</a></th>
		<th></th>
		<th></th>
	</tr>
	</tfoot>

</table>

</body>
</html>
