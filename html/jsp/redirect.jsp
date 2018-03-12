<%@page language="java" import="java.util.*" pageEncoding="utf-8"%>
<%
	out.println("hello");
	session.setAttribute("information", "hello, information");
	session.setAttribute("aaa", "hello, aaa");
	response.sendRedirect("redirect_forward.jsp");
%>


