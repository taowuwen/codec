
<%@page language="java" import="java.util.*" pageEncoding="utf-8" %>
<%@page import="java.sql.*" %>


<html>
<head><title>登录</title></head>

<body>

<script>

<%
String loginFlag = request.getParameter("loginflag");

if (loginFlag==null) {
	loginFlag="";
}

if (loginFlag.equals("1")) {
	String userName = (String)session.getAttribute("username");
	out.println("alert('用户 [" + userName + "]不存在！')");
} else 
if (loginFlag.equals("2")) {
	out.println("alert('密码错误')");
}


%>
</script>

<form action="login_action.jsp" method="POST">
<table width=600 align=center>
	<tr> 
		<td width=10% align=left> 用户名 </td> 
		<td align=left><input type="text" name="username" required="required" />*</td> 
	</tr>

	<tr> 
		<td width=10% align=left> 密&nbsp;&nbsp;码 </td> 
		<td align=left><input type="password" name="password" required="required" />*</td> 
	</tr>

	<tr> 
		<td width=10% align=left></td> 
		<td align=left>
			<input type="submit" name="login" value="登录" />
		</td>
	</tr>
</table>
</form>

</body>
</html>
