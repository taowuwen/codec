
<%@page language="java" contentType="text/html; charset=utf-8" pageEncoding="utf-8"%>
<%@page import ="java.utils.*" %>


<html>
	<head>
		<title>登录结果</title>
	</head>
<body>

<%
	String user = (String)session.getAttribute("username");
	out.println("普通用户" + user + "登录成功");
%>
</body>
</html>
