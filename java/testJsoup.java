

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.AbstractMap;
import java.util.Map;


class testJsoup {
	private static final String FILENAME = "/tmp/lastpage.html";

	/*
	private static final String tag_li_open = "<li>";
	private static final String tag_li_close = "</li>";
	*/
	private static final String tag_href_open = "href=\"";
	private static final String tag_href_close = "\"";

	private static final String tag_div_pic_txt_list_open = "<div class=\"pic_txt_list\">";
	private static final String tag_div_pic_txt_list_close = "</div>";

	private	static final String tag_li_open  = "<li class=\"c3\">";
	private	static final String tag_li_close = "</li>";


	private static final String tag_title_open = "<h1 class=\"title1\">";
	private static final String tag_title_close = "</h1>";

	private static final String tag_div_content_open  = "<div id=\"content\">";
	private static final String tag_div_content_close = "</div>";

	private static final String tag_page_prev = "<p id=\"page_last\"";
	private static final String tag_page_next = "<p id=\"page_next\"";
	private static final String tag_page_menu = "<p id=\"page_dir\"";
	private static final String tag_page_close = "</p>";

	public static void main(String[] args) {

		BufferedReader br = null;
		FileReader fr = null;
		String html = "";

		try {

			fr = new FileReader(FILENAME);
			br = new BufferedReader(fr);

			String sCurrentLine;

			while ((sCurrentLine = br.readLine()) != null) {
			//	System.out.println(sCurrentLine);
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

		parse_pageinfo(html.trim());
	}

	private static String strip_ctx(String ctx) {
		return ctx.replaceAll("<[^>]*>", "");
	}

	public static String get_tag_href(String ctx) {

		int s = ctx.indexOf(tag_href_open);
		int e = ctx.indexOf(tag_href_close, s + tag_href_open.length());

		if (s == -1 || e == -1)
			return null;

		return ctx.substring(s + tag_href_open.length(), e);
	}


	private static String get_page_tilte(String ctx, Map<String, String> map) {

		int s = ctx.indexOf(tag_title_open);
		int e = ctx.indexOf(tag_title_close, s + tag_title_open.length());

		if (s == -1 || e == -1)
			return null;

		String val = strip_ctx(ctx.substring(s, e)).trim();

		map.put("title", val);

		return ctx.substring(e + tag_title_close.length());
	}

	private static String get_page_content(String ctx, Map<String, String> map) {
		int s = ctx.indexOf(tag_div_content_open);
		int e = ctx.indexOf(tag_div_content_close, s + tag_div_content_open.length());

		if (s == -1 || e == -1)
			return null;

		String val = strip_ctx(ctx.substring(s, e).replaceAll("</p>", "\r\n\r\n").replaceAll("<p>", "    ")).trim();

		map.put("content", val);

		return ctx.substring(e + tag_div_content_close.length());
	}

	private static String page_info_href(String ctx, Map<String, String> map, String tag_open, String tag_close, String key) {
		int s = ctx.indexOf(tag_open);
		int e = ctx.indexOf(tag_close, s + tag_open.length());

		if (s == -1 || e == -1)
			return null;

		String val = get_tag_href(ctx.substring(s, e));

		if (val != null)
			map.put(key, val);

		return ctx.substring(e + tag_close.length());
	}

	private static String get_page_prev(String ctx, Map<String, String> map) {
		return page_info_href(ctx, map, tag_page_prev, tag_page_close, "page_prev");
	}

	private static String get_page_menu(String ctx, Map<String, String> map) {
		return page_info_href(ctx, map, tag_page_menu, tag_page_close, "menu");
	}

	private static String get_page_next(String ctx, Map<String, String> map) {
		return page_info_href(ctx, map, tag_page_next, tag_page_close, "page_next");

	}

	public static Map <String, String> parse_pageinfo(String ctx) {

		Map <String, String> map = new HashMap<String, String>();

		if (ctx == null || ctx.length() <= 0) return null; ctx = get_page_tilte(ctx, map);
		if (ctx == null || ctx.length() <= 0) return null; ctx = get_page_content(ctx, map);
		if (ctx == null || ctx.length() <= 0) return null; ctx = get_page_prev(ctx, map);
		if (ctx == null || ctx.length() <= 0) return null; ctx = get_page_menu(ctx, map);
		if (ctx == null || ctx.length() <= 0) return null; ctx = get_page_next(ctx, map);

		for (String key : map.keySet()) {  
			System.out.println(key + " > " + map.get(key));  
		}  

		return map;
	}

	public static String menu_item_key(String ctx) {
		return strip_ctx(ctx).trim();
	}

	public static String menu_item_value(String ctx) {

		int s = ctx.indexOf(tag_href_open);
		int e = ctx.indexOf(tag_href_close, s + tag_href_open.length());

		if (s == -1 || e == -1)
			return null;

		return ctx.substring(s + tag_href_open.length(), e);
	}


	public static String menu_next_item(String ctx, Map<String, String> map) {

		int s = ctx.indexOf(tag_li_open);
		int e = ctx.indexOf(tag_li_close, s);

		if (s == -1 || e == -1)
			return null;

		String item = ctx.substring(s, e);
		String key = menu_item_key(item);
		String val = menu_item_value(item);

		if (key != null && key.length() > 0 && val != null && val.length() > 0) {
			map.put(key, val);
		}

		return ctx.substring(e + tag_li_close.length());
	}


	public static void parse_menuinfo(String ctx) {

		//Map<String, String> map = new HashMap<String, String>();
		Map<String, String> map = new LinkedHashMap<String, String>();

		while (ctx != null && ctx.length() > 0) {
			ctx = menu_next_item(ctx, map);
		}

		for (String key : map.keySet()) {  
			System.out.println(key + " > " + map.get(key));  
		}
	}

	public static String get_image(String ctx, Map <String, String> map) {
		int s = ctx.indexOf("<img");
		int e = ctx.indexOf("</div>", s);

		if (s == -1 || e == -1)
			return null;

		String [] items = ctx.substring(s, e).split("\"");

		map.put("img", items[1]);

		return ctx.substring(e);
	}

	public static String get_title(String ctx, Map <String, String> map) {
		final String tag_span_open = "<span";
		final String tag_span_close = "</span>";

		int s = ctx.indexOf(tag_span_open);
		int e = ctx.indexOf(tag_span_close, s + tag_span_open.length());

		if (s == -1 || e == -1)
			return null;

		map.put("title", ctx.substring(s, e).replaceAll("<[^>]*>", ""));

		return ctx.substring(e + tag_span_close.length());
	}

	public static String get_author(String ctx, Map <String, String> map) {
		final String tag_span_open = "<span";
		final String tag_span_close = "</span>";

		int s = ctx.indexOf(tag_span_open);
		int e = ctx.indexOf(tag_span_close, s + tag_span_open.length());

		if (s == -1 || e == -1)
			return null;

		map.put("author", ctx.substring(s, e).replaceAll("<[^>]*>", ""));

		return ctx.substring(e + tag_span_close.length());
	}

	public static String get_page_link(String ctx, Map <String, String> map) {

		int s_tool = ctx.indexOf("tool_button");
		int s = ctx.indexOf(tag_href_open, s_tool);
		int e = ctx.indexOf(tag_href_close, s + tag_href_open.length());

		if (s_tool == -1 || s == -1 || e == -1)
			return null;

		map.put("url", ctx.substring(s, e));

		return ctx.substring(e + tag_href_close.length());
	}

	public static String get_description(String ctx, Map <String, String> map) {
		final String tag_desc_open = "<div class=\"description\"";
		final String tag_desc_close = "</div>";
		int s = ctx.indexOf(tag_desc_open);
		int e = ctx.indexOf(tag_desc_close, s + tag_desc_open.length());

		if (s == -1 || e == -1)
			return null;

		map.put("desc", ctx.substring(s, e).replaceAll("<[^>]*>", ""));

		return ctx.substring(e + tag_desc_close.length());
	}

	public static void parse_bookinfo(String ctx) {
		Map <String, String> map = new HashMap<String, String>();

		if (ctx != null && ctx.length() > 1) ctx = get_image(ctx, map);
		if (ctx != null && ctx.length() > 1) ctx = get_title(ctx, map);
		if (ctx != null && ctx.length() > 1) ctx = get_author(ctx, map);
		if (ctx != null && ctx.length() > 1) ctx = get_page_link(ctx, map);
		if (ctx != null && ctx.length() > 1) ctx = get_description(ctx, map);

		for (String key : map.keySet()) {  
			System.out.println("key= "+ key + " and value= " + map.get(key));  
		}  
	}

	public static void parse_search_result(String ctx) {

		Map <String, String> map = new HashMap<String, String>();

		while (ctx != null && ctx.length() > 0) {
			ctx = get_next_bookinfo(ctx, map);
		}

		for (String key : map.keySet()) {  
			System.out.println("key= "+ key + " and value= " + map.get(key));  
		}  
	}

	public static String get_next_bookinfo(String ctx, Map<String, String> map) {
		int s = ctx.indexOf(tag_div_pic_txt_list_open);
		int href_s = ctx.indexOf(tag_href_open, s + tag_div_pic_txt_list_open.length());
		int href_e = ctx.indexOf(tag_href_close, href_s + tag_href_open.length());
		int e = ctx.indexOf(tag_div_pic_txt_list_close, href_e + tag_href_close.length());

		if (s == -1 || e == -1 || href_s == -1 || href_e == -1)
			return null;

		String val = ctx.substring(href_s + tag_href_open.length(), href_e);

		int span_s = ctx.indexOf("<span", href_s);
		int span_e = ctx.indexOf("</span>", span_s);
		if (span_s == -1 || span_e == -1)
			return null;

		String key = ctx.substring(span_s, span_e).replaceAll("<[^>]*>", "");

		map.put(key, val);

		return ctx.substring(e + tag_div_pic_txt_list_close.length());
	}

	public static String parse_content(String content) {

		int s = content.indexOf("list2");
		int start = content.indexOf("<li>", s);
		int end   = content.indexOf("</ul>", start);

		System.out.println("s = " + s + " start = " + start + "  end = " + end);

		String new_string = new String(content.substring(start, end));

		Map <String, String> map = new HashMap<String, String>();

		String ctx = content.substring(start, end).trim();

		while (ctx != null && ctx.length() > 0) {
			ctx = get_next_item(ctx, map);
		}

		for (String key : map.keySet()) {  
			System.out.println("key= "+ key + " and value= " + map.get(key));  
		}  

		return new_string;
	}

	public static String get_next_item(String ctx, Map <String, String> map) {

		int s = ctx.indexOf(tag_li_open);
		int e = ctx.indexOf(tag_li_close);

		if (s == -1 || e == -1)
			return "";

		String []item = ctx.substring(s + tag_li_open.length(), e).split("\"");

		if (item[2].indexOf(">") == -1 || item[2].indexOf("<") == -1)
			return "";

		map.put(item[2].substring(item[2].indexOf(">"), item[2].indexOf("<")), item[1]);

		for (int i = 0; i < item.length; i++) {
			System.out.println(i + " : " + item[i]);
		}

		return ctx.substring(e + tag_li_close.length());
	}
	
}
