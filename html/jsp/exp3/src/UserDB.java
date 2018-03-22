
package exp3;

import java.io.*;
import java.sql.*;
import java.util.*;

public class UserDB {

	private Connection con = null;

	public UserInfo getUserByName(String name) {
		System.out.println("getUserByName " + name );
		UserInfo user = null;
		Statement stmt = null;

		try {
			con = DBConnection.getConnection();

			stmt = con.prepareStatment("select * from t_user where name=?");
			stmp.setString(1, name);

			ResultSet res = stmt.executeQuery();

			if (res.next()) {
				user = new UserInfo();

				user.setID(res.getInt("id"));
				user.setName(res.getString("name"));
				user.setPass(res.getString("pass"));

			}

			res.close();
			stmt.close();

		} catch(Exception e) {
			e.printStackTrace();
		} finally {
			DBConnection.closeConnection();
		}

		return user;
	}
}
