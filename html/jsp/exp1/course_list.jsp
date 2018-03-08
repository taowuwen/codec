<%@page language="java" contentType="text/html; charset=ISO-8859-1" pageEncoding="ISO-8859-1"%>

<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=ISO-8859-1">
	<title>Insert title here</title>
</head>

<body>

<table border="1">

<%
int i = 1;
int j = 1;

for (i = 1; i <= 9; i++) {
	out.println("<tr>");
	for (j = 1; j <= i; j++) {
		out.println("<td>" + i + "*" + j + "=" + (i*j) + "</td>");
	}
	out.println("<br/></tr>");
}
%>

</table>

</body>
</html>
