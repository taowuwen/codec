
<%@page import="mytool.*" %>
<%@page import="java.util.*" %>

<html>
<head><title>Hello World</title></head>

<body>
Hello World!<br/>
<%


	String name=request.getParameter("username");
	String pass=request.getParameter("password");
	out.println(System.getenv("JAVA_HOME"));
	out.println(System.getenv("CLASSPATH"));
	out.println(System.getenv("CLASSPATH"));

	Map<String, String> env = System.getenv();
	for (String envName : env.keySet()) {
		out.println(envName + " = " + env.get(envName) + "<br/>");
	}
	out.println(MyTools.change(name));
	out.println(pass);
%>
</body>
</html>
