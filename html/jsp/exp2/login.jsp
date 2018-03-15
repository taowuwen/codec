<%@page language="java" contentType="text/html; charset=utf-8" pageEncoding="utf-8"%>

<html>
<head>
	<meta http-equiv="Content-Type" content="text/html; charset=utf-8">
	<title>用户登录</title>
</head>

<script type="text/javascript" src="js/datacheck.js"></script>

<script language="JavaScript">
function do_submit(){

var frm = document.getElementById("frm_login");

if (frm.user.value.length <=0){
	alert("用户名不能为空");
	frm.user.focus();
	return;
}

if (frm.pass.value.length <=0){
	alert("用户密码不能为空");
	frm.pass.focus();
	return;
}

frm.submit();
}

function do_reset() {
	var frm = document.getElementById('frm_login');
	frm.user.value = "";
	frm.pass.value = "";
	frm.user.focus();
}
</script>

<body>
	<form id="frm_login" action="login_action.jsp">
	<table width=500 align="center" border=1> 
		<tr> <td colspan=2>用户登录信息录入，*为必填</td> </tr>
		<tr> <td>用户名: </td><td><input id="user" type="text" name="username" />*</td></tr>
		<tr> <td>密码: </td><td><input id="pass" type="password" name="password" />*</td></tr>
		<tr>
			<td colspan=2 align="center"> 
				<button type="button" onClick="javascript:do_submit()" >提交</button> 
				<button type="button" onClick="javascript:do_reset()" >重置</button>  
			</td> 
		</tr>
	</table>
	</form>
</body>
</html>
