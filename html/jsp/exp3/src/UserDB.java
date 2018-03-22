
package exp3;

import java.io.*;
import java.sql.*;
import java.util.*;

public class UserDB {

	private Connection con = null;

	public UserInfo getUserByName(String name) throws Exception {
		System.out.println("getUserByName " + name );
		UserInfo user = null;
		PreparedStatement stmt = null;

		try {
			con = DBConnection.getConnection();

			stmt = con.prepareStatement("select * from t_user where name=?");
			stmt.setString(1, name);

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
