<html>
<head><title>Hello World</title></head>

<body>
Hello World!<br/>
<%

	String name=request.getParameter("username");
	String pass=request.getParameter("password");
	out.println(name);
	out.println(pass);
%>
</body>
</html>
