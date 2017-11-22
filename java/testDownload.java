
import java.util.*;
import java.net.*;
import java.io.BufferedOutputStream;

class testDownload {

	public static void main(String[] args) {

		String url = "http://www.freakingnews.com/pictures/28000/Shipwreck-at-the-Beach--28308.jpg";
		String FileName = "/tmp/jm.jpg";

		try{
			java.io.BufferedInputStream in = new java.io.BufferedInputStream(new java.net.URL(url).openStream());
			java.io.FileOutputStream fos = new java.io.FileOutputStream(FileName);
			java.io.BufferedOutputStream bout = new BufferedOutputStream(fos,1024);
			byte[] data = new byte[1024];
			int x=0;

			while((x=in.read(data,0,1024))>=0){
				bout.write(data,0,x);               
			}

			fos.flush();
			bout.flush();
			fos.close();
			bout.close();
			in.close();

		} catch (Exception e){
			/* Display any Error to the GUI. */
			e.printStackTrace();
		}
		System.out.println("finished...");

	}
}
