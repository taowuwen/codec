
package mytool;

import java.io.*;

public class MyTools {
	public static String change(String str) {
		return str.replace("<", "&lt;").replace(">", "&gt;");
	}
}
