
package exp4;

import java.io.*;
import javax.servlet.*;
import javax.servlet.http.*;

public class CourseInsertAction extends HttpServlet {

	public void init() throws ServletException {
		// do nothing
	}

	public void doPost(HttpServletRequest request, HttpServletResponse response) throws ServletException,IOException {
		response.setContentType("text/html");
	}

	public void doGet(HttpServletRequest request, HttpServletResponse response) throws ServletException,IOException {
		doPost(request, response);
	}

	public void destroy() {
		// do nothing
	}
}
