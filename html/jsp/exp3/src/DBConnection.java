
package exp3;

import java.io.*;
import java.sql.*;
import java.util.*;
import java.lang.*;

public class DBConnection {
	private static Connection con = null;

	public static Connection getConnection() throws Exception {
		if (con != null)
			return con;

		try {
			String user="root";
			String pass="123456";
			String url = "jdbc:mysql://localhost:3306/db_student";

			Class.forName("com.mysql.jdbc.Driver").newInstance();

			con = DriverManager.getConnection(url, user, pass);
		}
		catch(SQLException e) {
			e.printStackTrace();
		}

		return con;
	}

	public static void closeConnection() throws Exception {
		if (con != null) {
			con.close();
			con = null;
		}
	}
}
