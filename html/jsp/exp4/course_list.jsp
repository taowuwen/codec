<%@page language="java" contentType="text/html; charset=utf-8" pageEncoding="utf-8"%>
<%@page import="exp4.*" %>
<%@page import="java.util.*" %>
<%@page import="java.io.*" %>

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
	<%
		ArrayList <CourseInfo> course_list = (ArrayList<CourseInfo>)session.getAttribute("CourseList");
		CourseInfo info = null;

		if (course_list != null) {
			for (int i = 0; i < course_list.size(); i++) {
				info = course_list.get(i);

				String type = (info.getType() == 5)?"专业必修课":"公共课" ;
				String major = (info.getMajor() == 1)?"软件工程":"通信工程";
out.println("<tr> ");
out.println("	<th align=\"left\">" +info.getName() + "</th>");
out.println("	<th>" + type + "</th>");
out.println("	<th>" + info.getCredit() + "</th>");
out.println("	<th>" + info.getGrade() + "</th>");
out.println("	<th>" + major + "</th>");
out.println("	<th align=\"right\"><a href=\"course_edit.jsp\">编辑</a></th>");
out.println("	<th align=\"left\"><a href=\"course_del.jsp\">删除</a></th>");
out.println("</tr>");
			}
		} else 
			out.println("<tr><th colspan=7>NULL</th></tr>");
	%>
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
