
package exp4;

public class CourseInfo {
	private int id = 0;
	private int type = 0;
	private float credit = 0;
	private int grade = 0;
	private int major = 0;
	private String detail = null;
	private String name = null;

	public int getID() {
		return this.id;
	}

	public void setID(int id) {
		this.id = id;
	}

	public int getType() {
		return this.type;
	}

	public void setType(int type) {
		this.type = type;
	}

	public float getCredit() {
		return this.credit;
	}

	public void setCredit(int credit) {
		this.credit = credit;
	}

	public int getGrade() {
		return this.grade;
	}

	public void setGrade(int grade) {
		this.grade = grade;
	}

	public int getMajor() {
		return this.major;
	}

	public void setMajor(int major) {
		this.major = major;
	}

	public String getDetail() {
		return this.detail;
	}

	public void setDetail(String detail) {
		this.detail = detail;
	}

	public String getName() {
		return this.name;
	}

	public void setName(String name) {
		this.name = name;
	}
}
