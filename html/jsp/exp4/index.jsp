
<%@page language="java" contentType="text/html; charset=utf-8" pageEncoding="utf-8"%>
<%
	String path=request.getContextPath();
	response.sendRedirect(path + "/CourseListAction");
%>
