
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
		int property = request.getParameter("property");

		course.setName(name);



		response.sendRedirect(request.getContextPath() + MyTools.prefix_path() + "/course_list.jsp");
	}

	public void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException,IOException {
		doPost(request, response);
	}

	public void destroy() {
		// do nothing
	}
}
