
package exp4;

import java.io.*;
import javax.servlet.*;
import javax.servlet.http.*;

public class CourseInsertAction extends HttpServlet {

	public void init() throws ServletException {
		// do nothing
	}

	public void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException,IOException {
		
		CourseInfo course = new CourseInfo();

		String name = request.getParameter("name");
		float credit = Float.parseFloat(request.getParameter("credit"));
		int property = Integer.parseInt(request.getParameter("property"));
		int major    = Integer.parseInt(request.getParameter("major"));
		int grade    = Integer.parseInt(request.getParameter("grade"));
		String detail = request.getParameter("detail");

		course.setName(MyTools.toChinese(name));
		course.setCredit(credit);
		course.setType(property);
		course.setMajor(major);
		course.setDetail(MyTools.toChinese(detail));
		course.setGrade(grade);

		CourseDB db = new CourseDB();
		try {
			db.insertCourseInfo(course);
		} catch (Exception e) {
			e.printStackTrace();
		}

		response.sendRedirect(request.getContextPath() + "/CourseListAction");
	}

	public void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException,IOException {
		doPost(request, response);
	}

	public void destroy() {
		// do nothing
	}
}
