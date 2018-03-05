<html>
<head><title>Hello World</title></head>

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
