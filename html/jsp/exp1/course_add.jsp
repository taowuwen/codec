<%@page language="java" contentType="text/html; charset=utf-8" pageEncoding="utf-8"%>

<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
	<title>添加课程信息</title>
</head>

<body>

<script type="text/javascript" src="js/datacheck.js"></script>

<script language="JavaScript">
function check(){

var frm = document.getElementById("my_form");

if (frm.name.value.length < 5 || frm.name.value.length >20) {
	alert("课程名称不为空，且长度为5~20");
	frm.name.focus();
	return;
}

if (frm.credit.value.length <= 0 || isNaN(frm.credit.value)){
	alert("课程学分不为空，且为一数字");
	frm.credit.focus();
	return;
}

if (frm.grade.value.length != 4 || !IsInt(frm.grade.value)) {
	alert("请给开设年级输入4位数的年份！");
	frm.grade.focus();
	return;
}

frm.submit();
}
</script>
<form id="my_form" action="course_list.jsp">
<table width="600" align="center" border=1>
	<tr> <td colspan=2>课程信息录入, *为必填选项 </td>
	</tr>
	<tr> 
		<td width=100>名称 </td>
		<td> <input id="name" type="text" name="username" />*20位字符</td>
	</tr>
	<tr> 
		<td width=100>性质 </td>
		<td>
		<select name="property">
			<option value="2">公共课必修课</option>
			<option value="1">专业课必修课</option>
		</select>*
		</td>
			
	</tr>
	<tr> 
		<td width=100>学分</td>
		<td> <input id="credit" type="text" name="username"  />*数值</td>
	</tr>
	<tr> 
		<td width=100>开设年级</td>
		<td> <input id="grade" type="text" name="username"  />*4位数年份</td>
	</tr>
	<tr> 
		<td width=100>开设专业</td>
		<td> <input type="radio" name="major" value="software" checked="checked" />软件工程
		<input type="radio" name="major" value="space" />空间线性</td>
	</tr>
	<tr> 
		<td width=100>备注</td>
		<td>
			<textarea name="textarea"  cols="40"></textarea>200个字以内
		</td>
	</tr>
	<tr align="center">
		<td colspan=2> <button type="button" onClick="javascript:check()">提交</button> </td>
	</tr>
</table>
</form>

</body>
</html>
