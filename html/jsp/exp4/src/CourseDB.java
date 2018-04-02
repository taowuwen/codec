
package exp4;

import java.io.*;
import java.sql.*;
import java.util.*;

public class CourseDB {

	private Connection con = null;

	public ArrayList<CourseInfo> getAll() throws Exception {

		CourseInfo course = null;

		ArrayList <CourseInfo> course_list = new ArrayList<CourseInfo>();

		try {
			con = DBConnection.getConnection();

			String sql = "select * from t_course";
			
			PreparedStatement stmt = con.prepareStatement(sql);
			//Statement stmt = con.createStatement();
			ResultSet res = stmt.executeQuery(sql);

			while (res.next()) {

				course = new CourseInfo();

				course.setID(res.getInt("id"));
				course.setName(res.getString("name"));
				course.setType(res.getInt("type"));
				course.setCredit(res.getFloat("credit"));
				course.setGrade(res.getInt("grade"));
				course.setMajor(res.getInt("major"));
				course.setDetail(res.getString("detail"));

				course_list.add(course);
			}

			res.close();
			stmt.close();

		} catch(Exception e) {
			e.printStackTrace();
		} finally {
			DBConnection.closeConnection();
			con = null;
		}

		return course_list;
	}

	public int insertCourseInfo(CourseInfo course) throws Exception {
		int res = 0;

		try {
			con = DBConnection.getConnection();

			String sql = "insert into t_course(name, type, credit, grade, major, detail) values(?, ?, ?, ?, ?, ?)";

			PreparedStatement stmt = con.prepareStatement(sql);
			stmt.setString(1, course.getName());
			stmt.setInt(2, course.getType());
			stmt.setFloat(3, course.getCredit());
			stmt.setInt(4, course.getGrade());
			stmt.setInt(5, course.getMajor());
			stmt.setString(6, course.getDetail());

			stmt.executeUpdate();

			stmt.close();

		} catch(Exception e) {
			e.printStackTrace();
			res = -1;
		} finally {
			DBConnection.closeConnection();
			con = null;
		}

		return res;
	}
}
