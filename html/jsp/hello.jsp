<html>
<head><title>Hello World</title></head>

<body>
Hello World!<br/>
<%
out.println("Your IP address is " + request.getRemoteAddr());
out.println(System.getenv());
%>
</body>
</html>
