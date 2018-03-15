<%@page language="java" contentType="text/html; charset=utf-8" pageEncoding="utf-8"%>

<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
	<title>用户登录结果</title>
</head>

<body>

	<%
		request.setCharacterEncoding("gb2312");

		String user = request.getParameter("username");
		String pass = request.getParameter("password");

		if (user != null && user.length() > 0) {
			if (pass != null && pass.length() > 0) {
				
				if (user.equals("admin")){
					if (pass.equals("Abc123#")) {
						out.println("<p>管理员登录成功</p>");
					} else {
						out.println("<p>管理员密码错误</p>");
					}
				} else {
					if (pass.equals(user)){
						session.setAttribute("username", user);
						response.sendRedirect("user_info.jsp");
					} else {
						out.println("<p>用户名或密码错误</p>");
					}
				}

			}
			else {
				out.println("<p>password invalid</p>");
			}
		} else {
			out.println("<p>username invalid</p>");
		}

	%>

</body>
</html>
