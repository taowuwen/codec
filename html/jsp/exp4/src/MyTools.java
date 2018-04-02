
package exp4;

import java.io.*;

public class MyTools {
	public static String change(String str) {
		return str.replace("<", "&lt;").replace(">", "&gt;");
	}

	public static String prefix_path() {
		return "/codec/jsp/exp4";
	}

	public static String toChinese(String str) {
		String value = null;

		try {
			byte ptext[] = str.getBytes("ISO-8859-1"); 
			value = new String(ptext, "UTF-8"); 
		} catch(Exception e) {
			e.printStackTrace();
		}

		return value;
	}
}
