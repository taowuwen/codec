/*
 * uhttpd - Tiny single-threaded httpd - Static file handler
 *
 *   Copyright (C) 2010-2012 Jo-Philipp Wich <xm@subsignal.org>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include "uhttpd.h"
#include "uhttpd-utils.h"
#include "uhttpd-file.h"

#include "uhttpd-mimetypes.h"

#ifdef DT_USE_SELECT
#include "dt_uhttpd_select.h"
#else
#include "dt_uhttpd_evt.h"
#endif

#ifdef __APPLE__
time_t timegm (struct tm *tm);
#endif

static const char * uh_file_mime_lookup(const char *path)
{
	struct mimetype *m = &uh_mime_types[0];
	const char *e;

	while (m->extn)
	{
		e = &path[strlen(path)-1];

		while (e >= path)
		{
			if ((*e == '.' || *e == '/') && !strcasecmp(&e[1], m->extn))
				return m->mime;

			e--;
		}

		m++;
	}

	return "application/octet-stream";
}

static const char * uh_file_mktag(struct stat *s)
{
	static char tag[128];

	snprintf(tag, sizeof(tag), "\"%x-%x-%x\"",
			 (unsigned int) s->st_ino,
			 (unsigned int) s->st_size,
			 (unsigned int) s->st_mtime);

	return tag;
}

static time_t uh_file_date2unix(const char *date)
{
	struct tm t;

	memset(&t, 0, sizeof(t));

	if (strptime(date, "%a, %d %b %Y %H:%M:%S %Z", &t) != NULL)
		return timegm(&t);

	return 0;
}

static char * uh_file_unix2date(time_t ts)
{
	static char str[128];
	struct tm *t = gmtime(&ts);

	strftime(str, sizeof(str), "%a, %d %b %Y %H:%M:%S GMT", t);

	return str;
}

static char * uh_file_header_lookup(struct client *cl, const char *name)
{
	int i;

	foreach_header(i, cl->request.headers)
	{
		if (!strcasecmp(cl->request.headers[i], name))
			return cl->request.headers[i+1];
	}

	return NULL;
}


static int uh_file_response_ok_hdrs(struct client *cl, struct stat *s)
{
	ensure_ret(uh_http_sendf(cl, NULL, "Connection: close\r\n"));

	if (s)
	{
		ensure_ret(uh_http_sendf(cl, NULL, "ETag: %s\r\n", uh_file_mktag(s)));
		ensure_ret(uh_http_sendf(cl, NULL, "Last-Modified: %s\r\n",
								 uh_file_unix2date(s->st_mtime)));
	}

	return uh_http_sendf(cl, NULL, "Date: %s\r\n", uh_file_unix2date(time(NULL)));
}

static int uh_file_response_206(struct client *cl, struct stat *s)
{

	ensure_ret(uh_http_sendf(cl, NULL, "%s 206 Partial content\r\n",
						 http_versions[cl->request.version]));

	return uh_file_response_ok_hdrs(cl, s);
}

static int uh_file_response_200(struct client *cl, struct stat *s)
{
	ensure_ret(uh_http_sendf(cl, NULL, "%s 200 OK\r\n",
							 http_versions[cl->request.version]));

	return uh_file_response_ok_hdrs(cl, s);
}

static int uh_file_response_304(struct client *cl, struct stat *s)
{
	ensure_ret(uh_http_sendf(cl, NULL, "%s 304 Not Modified\r\n",
							 http_versions[cl->request.version]));

	return uh_file_response_ok_hdrs(cl, s);
}

static int uh_file_response_412(struct client *cl)
{
	return uh_http_sendf(cl, NULL,
						 "%s 412 Precondition Failed\r\n"
						 "Connection: close\r\n",
	                     http_versions[cl->request.version]);
}

static int uh_file_if_match(struct client *cl, struct stat *s, int *ok)
{
	const char *tag = uh_file_mktag(s);
	char *hdr = uh_file_header_lookup(cl, "If-Match");
	char *p;
	int i;

	if (hdr)
	{
		p = &hdr[0];

		for (i = 0; i < strlen(hdr); i++)
		{
			if ((hdr[i] == ' ') || (hdr[i] == ','))
			{
				hdr[i++] = 0;
				p = &hdr[i];
			}
			else if (!strcmp(p, "*") || !strcmp(p, tag))
			{
				*ok = 1;
				return *ok;
			}
		}

		*ok = 0;
		ensure_ret(uh_file_response_412(cl));
		return *ok;
	}

	*ok = 1;
	return *ok;
}

static int uh_file_if_modified_since(struct client *cl, struct stat *s, int *ok)
{
	char *hdr = uh_file_header_lookup(cl, "If-Modified-Since");
	*ok = 1;

	if (hdr)
	{
		if (uh_file_date2unix(hdr) >= s->st_mtime)
		{
			*ok = 0;
			ensure_ret(uh_file_response_304(cl, s));
		}
	}

	return *ok;
}

static int uh_file_if_none_match(struct client *cl, struct stat *s, int *ok)
{
	const char *tag = uh_file_mktag(s);
	char *hdr = uh_file_header_lookup(cl, "If-None-Match");
	char *p;
	int i;
	*ok = 1;

	if (hdr)
	{
		p = &hdr[0];

		for (i = 0; i < strlen(hdr); i++)
		{
			if ((hdr[i] == ' ') || (hdr[i] == ','))
			{
				hdr[i++] = 0;
				p = &hdr[i];
			}
			else if (!strcmp(p, "*") || !strcmp(p, tag))
			{
				*ok = 0;

				if ((cl->request.method == UH_HTTP_MSG_GET) ||
				    (cl->request.method == UH_HTTP_MSG_HEAD))
				{
					ensure_ret(uh_file_response_304(cl, s));
				}
				else
				{
					ensure_ret(uh_file_response_412(cl));
				}

				break;
			}
		}
	}

	return *ok;
}

static int uh_file_if_range(struct client *cl, struct stat *s, int *ok)
{
	char *hdr = uh_file_header_lookup(cl, "If-Range");
	*ok = 1;

	if (hdr)
	{
		*ok = 0;
		ensure_ret(uh_file_response_412(cl));
	}

	return *ok;
}

static int uh_file_if_unmodified_since(struct client *cl, struct stat *s,
									   int *ok)
{
	char *hdr = uh_file_header_lookup(cl, "If-Unmodified-Since");
	*ok = 1;

	if (hdr)
	{
		if (uh_file_date2unix(hdr) <= s->st_mtime)
		{
			*ok = 0;
			ensure_ret(uh_file_response_412(cl));
		}
	}

	return *ok;
}


static int uh_file_scandir_filter_dir(const struct dirent *e)
{
	return strcmp(e->d_name, ".") ? 1 : 0;
}

static void uh_file_dirlist(struct client *cl, struct path_info *pi)
{
	int i;
	int count = 0;
	char filename[PATH_MAX];
	char *pathptr;
	struct dirent **files = NULL;
	struct stat s;

	ensure_out(uh_http_sendf(cl, &cl->request,
							 "<html><head><title>Index of %s</title></head>"
							 "<body><h1>Index of %s</h1><hr /><ol>",
							 pi->name, pi->name));

	if ((count = scandir(pi->phys, &files, uh_file_scandir_filter_dir,
						 alphasort)) > 0)
	{
		memset(filename, 0, sizeof(filename));
		memcpy(filename, pi->phys, sizeof(filename));
		pathptr = &filename[strlen(filename)];

		/* list subdirs */
		for (i = 0; i < count; i++)
		{
			strncat(filename, files[i]->d_name,
					sizeof(filename) - strlen(files[i]->d_name));

			if (!stat(filename, &s) &&
				(s.st_mode & S_IFDIR) && (s.st_mode & S_IXOTH))
			{
				ensure_out(uh_http_sendf(cl, &cl->request,
										 "<li><strong><a href='%s%s'>%s</a>/"
										 "</strong><br /><small>modified: %s"
										 "<br />directory - %.02f kbyte<br />"
										 "<br /></small></li>",
										 pi->name, files[i]->d_name,
										 files[i]->d_name,
										 uh_file_unix2date(s.st_mtime),
										 s.st_size / 1024.0));
			}

			*pathptr = 0;
		}

		/* list files */
		for (i = 0; i < count; i++)
		{
			strncat(filename, files[i]->d_name,
					sizeof(filename) - strlen(files[i]->d_name));

			if (!stat(filename, &s) &&
				!(s.st_mode & S_IFDIR) && (s.st_mode & S_IROTH))
			{
				ensure_out(uh_http_sendf(cl, &cl->request,
										 "<li><strong><a href='%s%s'>%s</a>"
										 "</strong><br /><small>modified: %s"
										 "<br />%s - %.02f kbyte<br />"
										 "<br /></small></li>",
										 pi->name, files[i]->d_name,
										 files[i]->d_name,
										 uh_file_unix2date(s.st_mtime),
										 uh_file_mime_lookup(filename),
										 s.st_size / 1024.0));
			}

			*pathptr = 0;
		}
	}

	ensure_out(uh_http_sendf(cl, &cl->request, "</ol><hr /></body></html>"));
	ensure_out(uh_http_sendf(cl, &cl->request, ""));

out:
	if (files)
	{
		for (i = 0; i < count; i++)
			free(files[i]);

		free(files);
	}
}

/*
 * handle http range request for dlna
 * updated by tww Feb 16th, 2014
 * */


static int uh_file_range(struct client *cl, struct stat *s, int *ok, int fd)
{
	off_t start, end;
	char *pos = NULL;
	char *hdr = uh_file_header_lookup(cl, "Range");

	char *from, *to;

	*ok = 1;

	if (hdr) {
		D("Range header: %s\n", hdr);

		pos = strstr(hdr, "bytes=");

		if (!pos) {
			uh_http_sendhf(cl, 403, "Forbidden",
					  "Access to this resource is forbidden");

			return -1;
		}

		from = pos + strlen("bytes=");

		pos = strchr(from, '-');
		if (!pos) {
			uh_http_sendhf(cl, 403, "Forbidden",
					  "Access to this resource is forbidden");
			return -1;
		}


		*pos = '\0';
		to = pos + 1;

		start = strtoll(from, NULL, 10);
		end   = strtoll(to, NULL, 10);

		if ((start >= s->st_size) || (end >= s->st_size)) {
			uh_http_sendhf(cl, 403, "Forbidden",
					  "Access to this resource is forbidden");
			return -1;
		}

		if (strlen(from) <= 0) {
			if (strlen(to)<= 0) {
				uh_http_sendhf(cl, 403, "Forbidden",
						  "Access to this resource is forbidden");
				return -1;
			}


			start = s->st_size - end;
		}
		else {
			if (strlen(to)<= 0)
				end = s->st_size -1;
		}

		cl->content_left = end - start + 1;

		D("contents from %lld to %lld total: %lld bytes, filesize: %lld\n",
				start, end, cl->content_left, s->st_size);

		if (cl->content_left <= 0) {
			uh_http_sendhf(cl, 403, "Forbidden",
				  "Access to this resource is forbidden");
			return -1;
		}

		lseek(cl->file, start, SEEK_SET);

		uh_file_response_206(cl, s);
		uh_http_sendf(cl, NULL, 
			"Content-Range: bytes %lld-%lld/%lld\r\n", start, end, s->st_size);
		uh_http_sendf(cl, NULL, "Content-Length: %lld\r\n", cl->content_left);
		return 1;
	}
	return 0;

}


bool uh_file_request(struct client *cl, struct path_info *pi, int *detached)
{
	int rlen;
	int ok = 1;
	int fd = -1;
	char buf[UH_LIMIT_MSGHEAD];

	/* we have a file */
	if ((pi->stat.st_mode & S_IFREG) && ((fd = open(pi->phys, O_RDONLY)) > 0))
	{
		/* test preconditions */
		if (ok) ensure_out(uh_file_if_modified_since(cl, &pi->stat, &ok));
		if (ok) ensure_out(uh_file_if_match(cl, &pi->stat, &ok));
		if (ok) ensure_out(uh_file_if_range(cl, &pi->stat, &ok));
		if (ok) ensure_out(uh_file_if_unmodified_since(cl, &pi->stat, &ok));
		if (ok) ensure_out(uh_file_if_none_match(cl, &pi->stat, &ok));

		if (ok > 0)
		{

			switch(uh_file_range(cl, &pi->stat, &ok, fd)) {
			case 0:
				cl->content_left = pi->stat.st_size;
				ensure_out(uh_file_response_200(cl, &pi->stat));
				ensure_out(uh_http_sendf(cl, NULL, "Content-Length: %lld\r\n", pi->stat.st_size));
				break;
			case 1:
				break;
			default:
				goto out;
				break;

			}

			ensure_out(uh_http_sendf(cl, NULL, "Content-Type: %s\r\n", uh_file_mime_lookup(pi->name)));

			if ((cl->request.version > UH_HTTP_VER_1_0) &&
				(cl->request.method != UH_HTTP_MSG_HEAD))
			{
				ensure_out(uh_http_send(cl, NULL, "Transfer-Encoding: chunked\r\n", -1));
			}

			ensure_out(uh_http_send(cl, NULL, "\r\n", -1));

			if (cl->request.method != UH_HTTP_MSG_HEAD)
			{
				if (detached && pi->stat.st_size >= (1 << 2)) {
					*detached = 1;
				#ifdef DT_USE_SELECT
					D("Client(%d) had already detached\n", cl->fd.fd);
					uh_client_detach(cl);
					dt_uhttpd_send_content(cl, fd);
					#ifdef DT_USE_EPOLL
					#error "please do not use -DDT_USE_EPOLL"
					#endif
				#elif DT_USE_EPOLL
					dt_uhttpd_evt_send(cl, fd);
					#ifdef DT_USE_SELECT
					#error "please do not use -DDT_USE_SELECT"
					#endif
				#else
					#error "must compile with DT_USE_SELECT or DT_USE_EPOLL"
				#endif
				} else {
					/* pump file data */
					while ((rlen = read(fd, buf, sizeof(buf))) > 0)
						ensure_out(uh_http_send(cl, &cl->request, buf, rlen));

					/* send trailer in chunked mode */
					ensure_out(uh_http_send(cl, &cl->request, "", 0));
				}
			}
		}

		/* one of the preconditions failed, terminate opened header and exit */
		else
		{
			ensure_out(uh_http_send(cl, NULL, "\r\n", -1));
		}
	}

	/* directory */
	else if ((pi->stat.st_mode & S_IFDIR) && !cl->server->conf->no_dirlists)
	{
		/* write status */
		ensure_out(uh_file_response_200(cl, NULL));

		if (cl->request.version > UH_HTTP_VER_1_0)
			ensure_out(uh_http_send(cl, NULL,
				"Transfer-Encoding: chunked\r\n", -1));

		ensure_out(uh_http_send(cl, NULL,
				"Content-Type: text/html;charset=UTF-8\r\n\r\n", -1));

		/* content */
		uh_file_dirlist(cl, pi);
	}

	/* 403 */
	else
	{
		ensure_out(uh_http_sendhf(cl, 403, "Forbidden",
				  "Access to this resource is forbidden"));
	}

out:
	if (detached && (*detached == 1)) {
		/*
		 * do nothing here
		 * */
	} else {
		if (fd > -1)
			close(fd);
	}

	return false;
}
