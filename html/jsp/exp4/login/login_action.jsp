
<%@page language="java" import="java.util.*" pageEncoding="utf-8" %>
<%@page import="java.sql.*" %>
<%@page import="exp3.*" %>


<%
String path = request.getContextPath();
String username = request.getParameter("username");
String password = request.getParameter("password");

int flag=0;

if (username != null && password != null && !username.equals("") && !password.equals("")) {
	UserDB db = new UserDB();
	UserInfo user = db.getUserByName(username);
	session.setAttribute("username", username);

	if (user == null)
		flag = 1;
	else {
		if (!user.getPass().equals(password))
			flag = 2;
		else
			flag = 0;

	}
	
} else
	flag = 1;

if (flag == 0)
	response.sendRedirect("user_info.jsp");
else 
	response.sendRedirect("login.jsp?loginflag="+flag);

%>
