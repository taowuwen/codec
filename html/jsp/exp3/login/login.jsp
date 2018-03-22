
<%@page language="java" import="java.util.*" pageEncoding="utf-8" %>
<%@page import="java.sql.*" %>


<html>
<head><title>登录</title></head>

<body>

<form action="login_action.jsp" method="POST">
<table width=100% align="center">
	<tr width=600> <th width=200 align=left> 用户名 <th/> <th align=left><input type="text" name="username" required="required"> </th> </tr>
	<tr width=600> <th width=200 align=left> 密&nbsp码 <th/> <th align=left><input type="password" name="username" required="required"> </th> </tr>
	<input type="submit" name="login">
</table>
</form>

</body>
</html>
