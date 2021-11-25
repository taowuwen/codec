
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <regex.h>
#include <errno.h>
#include <mcheck.h>

int match_string(const char *string, const char *pattern, int *match)
{
    regex_t re;
    int ret = 0;
    char buf[1024] = {0};

    ret = regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB);
    if (ret != 0) {
        (void)regerror(ret, &re, buf, sizeof(buf));
        fprintf(stderr, "regcomp failed: %s\n", buf);
    //    regfree(&re);
        return -1;
    }

    ret = regexec(&re, string, 0, NULL, 0);

    regfree(&re);

    *match = (ret == 0);
    return 0;
}

int main(int argc, char **argv)
{
    FILE *fp = NULL;
    char buf_line[1024] =  {0};
    int matched = 0;
	//mtrace();

    if (argc < 2) {
        fprintf(stderr, "usage: %s re [file]\n", argv[0]);
        return -1;
    }

    if (argc == 3) {
        fp = fopen(argv[2], "r");
        if (!fp) {
            fprintf(stderr, "fopen failed %s", strerror(errno));
            return -2;
        }
    } else {
        fp = stdin;
    }

    while (fgets(buf_line, 1024, fp) != NULL) {
        if (match_string(buf_line, argv[1], &matched)) {
            break;
        }

        if (matched) {
            fprintf(stdout, "%s", buf_line);
        }
    }

    if (fp != stdin) {
        fclose(fp);
    }
    {
        char *ptr = malloc(1023 * sizeof(char));
        memset(ptr, 0, 8192);
        memcpy(ptr, buf_line, sizeof(buf_line));
        free(ptr);
    }
    printf("do exit\n");

    return 0;
}
