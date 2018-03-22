
package exp3;

import java.io.*;
import java.sql.*;
import java.util.*;

public class UserInfo {
	private int id = 0;
	private String name = null;
	private String pass = null;

	public int getID() {
		return this.id;
	}

	public void setID(int id) {
		this.id = id;
	}

	public String getName() {
		return this.name;
	}

	public void setName(String name) {
		this.name = name;
	}

	public String getPass() {
		return this.pass;
	}

	public void setPass(String pass) {
		this.pass = pass;
	}
}
