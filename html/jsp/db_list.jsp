<%@page language="java" import="java.util.*" pageEncoding="utf-8"%>
<%@page import="java.sql.*" %>
<%@page import="common.*" %>

<html>
	<head>
		<title> mysql connection and ctrl </title>
	</head>
<body>
	<p> connect database on localhost:3306 </p>
	<table width=100%>
		<thead align='left'>
			<tr> <th>id</th> <th>student num</th> <th>name</th> <th>gender</th> <th>date</th> <th>major</th> </tr>
		</thead>
<%

	try {
		Connection con = null;
		String url = "jdbc:mysql://localhost:3306/db_student";
		Class.forName("com.mysql.jdbc.Driver").newInstance();
		con = DriverManager.getConnection(url, "root", "123456");

		if (!con.isClosed()) {
			out.println("connect success");
		}

		Statement stmt = con.createStatement();
		String sql = "select * from t_student";
		ResultSet res = stmt.executeQuery(sql);

		int id;
		String num;
		String name;
		int gender;
		int grade;
		int major;

		while (res.next()){
			id  = res.getInt(1);
			num = res.getString(2);
			name = res.getString(3);
			gender = res.getInt(4);
			grade  = res.getInt(5);
			major  = res.getInt(6);

			out.println("<tr>"+"<td>"+id+"</td>"+"<td>"+num+"</td>"+"<td>"+name+"</td>"+"<td>"+gender+"</td>"+"<td>"+grade+"</td>"+"<td>"+major+"</td>"+"</tr>");
		}

		res.close();
		stmt.close();
		con.close();
	}
	catch(SQLException e) {
		out.println("SQLException caught: " + e.getMessage());
	}
%>
</tbody>
<tfoot>
<tr><td colspan=7> finished</td> </tr>
</tfoot>
</table>
</body>
</html>


