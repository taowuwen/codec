<%@page language="java" import="java.util.*" pageEncoding="utf-8"%>
<html>
<body>
<%
	out.println(session.getAttribute("information"));
	out.println(session.getAttribute("aaa"));
%>
</body>
</html>


