
class Parent {

	public String setup(String str) {
		System.out.println("Parent setup: for " + str);

		another_setup(str);

		return "nothing";
	}

	public void another_setup(String str) {
		System.out.println("Parent Another Setup for: " + str);
	}
}

class Child extends Parent {
	@Override
	public void another_setup(String str) {
		System.out.println("Child Another Setup for: " + str);
	}
}



class A {

	public static void main(String [] args) {

		Parent p = new Child();

		p.setup("setup in child instance");

		p.another_setup("call in Child??");
	}
}
