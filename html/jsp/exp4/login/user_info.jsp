
<%@page language="java" contentType="text/html; charset=utf-8" pageEncoding="utf-8"%>
<%@page import ="java.utils.*" %>
<%@page import="java.sql.*" %>
<%@page import="exp3.*" %>


<html>
	<head>
		<title>登录结果</title>
	</head>
<body>

<%
	String user = (String)session.getAttribute("username");
	if (user.equals("admin")) {
		out.println("administrator: " + user + "登录成功");
	} else {
		out.println("普通用户" + user + "登录成功");
	}
%>
</body>
</html>
