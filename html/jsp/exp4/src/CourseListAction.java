
package exp4;

import java.io.*;
import javax.servlet.*;
import javax.servlet.http.*;
import java.util.*;

public class CourseListAction extends HttpServlet {

	public void init() throws ServletException {
		// do nothing
	}

	public void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException,IOException {

		CourseDB db = new CourseDB();
		ArrayList<CourseInfo> list = null;
	
		try {
			list = db.getAll();
		} catch (Exception e) {
			e.printStackTrace();
		}

		request.getSession().setAttribute("CourseList", list);

		response.sendRedirect(request.getContextPath() + MyTools.prefix_path() + "/course_list.jsp");
	}

	public void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException,IOException {
		doPost(request, response);
	}

	public void destroy() {
		// do nothing
	}
}
