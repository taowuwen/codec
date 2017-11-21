

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;


class testJsoup {
	private static final String FILENAME = "/tmp/tmp.html";

	public static void main(String[] args) {

		BufferedReader br = null;
		FileReader fr = null;
		String html = "";

		try {

			fr = new FileReader(FILENAME);
			br = new BufferedReader(fr);

			String sCurrentLine;

			while ((sCurrentLine = br.readLine()) != null) {
				System.out.println(sCurrentLine);
				html += sCurrentLine;
			}

		} catch (IOException e) {
			e.printStackTrace();
		} finally {

			try {

				if (br != null)
					br.close();

				if (fr != null)
					fr.close();

			} catch (IOException ex) {
				ex.printStackTrace();
			}
		}

		System.out.println("result is: " + parse_content(html));
	}

	public static String parse_content(String content) {

		int s = content.indexOf("list2");
		int start = content.indexOf("<li>", s);
		int end   = content.indexOf("</ul>", start);

		System.out.println("s = " + s + " start = " + start + "  end = " + end);

		String new_string = new String(content.substring(start, end));

		return new_string;
	}
}
