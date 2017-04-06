#include "ems.h"
#include <sys/types.h>
#include <sys/sysinfo.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

ems_char *ems_trim(ems_char *src)
{
	ems_char *s = NULL, *e = NULL;
	ems_char *buf = NULL;
	ems_int l;

	buf = ems_strdup(src);
	if (!buf)
		return src;

	l = strlen(src);
	s = buf;
	e = s + l - 1;
	while (s < e) {
		if ((*s == ' ') || (*s=='\t') || (*s =='\n') || (*s =='\r'))
			s++;
		else if ((*e==' ') || (*e =='\t') || (*e=='\n') || (*e=='\r'))
			e--;
		else
			break;
	}

	if ((s >= e) && (*s == ' ')) {
		*src = '\0';
		ems_free(buf);
		return NULL;
	}

	e++;
	*e = '\0';

	snprintf(src, l+1, "%s", s);

	ems_free(buf);

	return src;
}

ems_cchar *ems_geterrmsg(ems_int err)
{
	static ems_char buf_err[128] = {0};

	snprintf(buf_err, 128, "(%d): %s", err, strerror(err));

	return buf_err;
}

ems_int ems_reset_rlimit()
{
	struct rlimit rl, old; 

	if (getrlimit(RLIMIT_CORE, &rl) == 0) {
		rl.rlim_cur = rl.rlim_max;
		if (setrlimit(RLIMIT_CORE, &rl))
			fprintf(stderr, "setrlimit RLIMIT_CORE failed: %s\n", strerror(errno));
	}

	getrlimit(RLIMIT_NOFILE, &old);

	rl.rlim_cur = rl.rlim_max = 65000;
	if (setrlimit(RLIMIT_NOFILE, &rl)) {
		if (errno == EPERM) {
			rl.rlim_cur = rl.rlim_max = old.rlim_max;
			if (setrlimit(RLIMIT_NOFILE, &rl))
				fprintf(stderr, "setrlimit RLIMIT_CORE failed: %s\n", strerror(errno));

		} else
			fprintf(stderr, "setrlimit RLIMIT_CORE failed: %s\n", strerror(errno));
	}

	return 0;
}

ems_cchar *ems_itoa(ems_int i)
{
	static ems_char buf[64];
	snprintf(buf, 64, "%d", i);
	return buf;
}

ems_int  ems_atoi(ems_cchar *str)
{
	if (str)
		return (ems_int)strtol(str, NULL, 10);

	return 0;
}

ems_long   ems_atol(ems_cchar *str)
{
	if (str)
		return (ems_int)strtol(str, NULL, 10);

	return 0;
}

long long  ems_atoll(ems_cchar *str)
{
	if (str)
		return strtoll(str, NULL, 10);

	return 0;
}

#define DIR_MODE	0775
#define FILE_MODE	0640

static ems_int mkdir_r(ems_cchar *path)
{
	ems_char  buf[1024], *p;
	ems_int   ret;

	snprintf(buf, sizeof(buf), "%s", path);

	p = buf + 1;

	while (*p != '\0') {

		if (*p == '/') {
			*p = '\0';

			ret = mkdir(buf, DIR_MODE);

			if (ret != 0 && errno != EEXIST) {
				return 1;
			}

			*p = '/';
		}
		p++;
	}

	mkdir(path, DIR_MODE);
	if (ret != 0 && errno != EEXIST) {
		return 1;
	}

	return 0;
}

ems_int mkdir_p(ems_cchar *path)
{
	ems_int   ret;

	ret = mkdir(path, DIR_MODE);
	if (ret == 0)
		return 0;

	ret = errno;

	switch(ret) {
	case ENOENT:
		return mkdir_r(path);

	case EEXIST:
	{
		struct stat st;

		if (stat(path, &st))
			return -1;

		if (S_ISDIR(st.st_mode))
			return 0;

		return -1;
	}

	default:
		return -1;
	}

	return 0;
}

static ems_int rm_dir(ems_cchar *path)
{
	ems_char buf[1024] = {0};
	struct dirent *d = NULL;
	DIR           *dirp = NULL;
	struct stat st;
	ems_int ret = 0;

	dirp = opendir(path);

	while (NULL != (d = readdir(dirp))) {
		if (!strncmp(d->d_name, ".", 1))
			continue;

		if (!strncmp(d->d_name, "..", 2))
			continue;

		snprintf(buf, sizeof(buf), "%s/%s", path, d->d_name);

		ret = -1;
		if (stat(buf, &st))
			goto err_out;

		if (S_ISDIR(st.st_mode)) {
			ret = rm_dir(buf);
		} else {
			ret = unlink(buf);
		}

		if (ret != 0) 
			goto err_out;
	}

	closedir(dirp);

	return rmdir(path);

err_out:
	closedir(dirp);
	return ret;
}

ems_int rm_rf(ems_cchar *path)
{
	struct stat st;

	if (stat(path, &st))
		return -1;

	if (S_ISDIR(st.st_mode)) {
		return rm_dir(path);
	} else {
		return unlink(path);
	}

	return 0;
}
