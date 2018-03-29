
package exp4;

import java.io.*;
import java.sql.*;
import java.util.*;

public class CourseDB {

	private Connection con = null;

	public ArrayList<CourseInfo> getAll() throws Exception {

		CourseInfo course = null;
		PreparedStatement stmt = null;

		ArrayList <CourseInfo> course_list = new ArrayList<CourseInfo>();

		try {
			con = DBConnection.getConnection();

			stmt = con.prepareStatement("select * from t_course");

			ResultSet res = stmt.executeQuery();

			while (res.next()) {
				course = new CourseInfo();

				course.setID(res.getInt("id"));
				course.setName(res.getString("name"));
				course.setType(res.getInt("type"));
				course.setCredit(res.getFloat("credit"));
				course.setGrade(res.getInt("grade"));
				course.setMajor(res.getInt("major"));
				course.setDetail(res.getString("pass"));

				course_list.add(course);

			}

			res.close();
			stmt.close();

		} catch(Exception e) {
			e.printStackTrace();
		} finally {
			DBConnection.closeConnection();
		}

		return course_list;
	}
}
