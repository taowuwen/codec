
import java.io.IOException;
import java.io.*;


class Demo
{
	public static void main(String [] args)
	{
		Exception_test et = new Exception_test();

		System.out.println("start...");

		et.L1();


		try {
			test_throw();

		} catch (Exception e) {
			System.out.println("Exception error " + e);
			e.printStackTrace();
		}

		/*

		try {
			BufferedReader dis = new BufferedReader(new InputStreamReader(System.in));
			System.out.print("Exter X : ");
			String s = dis.readLine();

			double x = Double.valueOf(s.trim()).doubleValue();

			System.out.println("x = " + x);
		} catch (Exception e) {
			System.out.println("Exception error " + e);
			e.printStackTrace();
		} 
		*/


		Incrementor f1 = new Incrementor();
		Incrementor f2 = new Incrementor();

		f1.inc_count();
		System.out.println("count = " + f1.get_count());

		f2.inc_count();
		System.out.println("count = " + f2.get_count());

		System.out.println("stopped...");
	}

	public static void test_throw() throws DieNow {
		DieNow dn = new DieNow("hello, let die...");
		throw dn;
	}
}

class Exception_test
{
	public void L3()
	{
		System.out.println("L3 beginning...");
		System.out.println(10/0);
		System.out.println("L3 ending");
	}

	public void L2()
	{
		System.out.println("L2 beginning...");
		L3();
		System.out.println("L2 ending");
	}

	public void L1()
	{
		System.out.println("L1 beginning...");
		try {
			L2();
		} catch (ArithmeticException problem) {
			System.out.println("Problem catching ...");
			problem.printStackTrace();
		}
		System.out.println("L1 ending");
	}
}


class DieNow extends IOException {
	DieNow(String msg) {
		super(msg);
	}
}


class Incrementor {
	public static int i = 0;

	void inc_count() {
		i++;
	}

	int get_count() {
		return i;
	}
}
