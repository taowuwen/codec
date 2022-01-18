
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <regex.h>
#include <errno.h>
#include <mcheck.h>

int print_details(regex_t *re, const char *string)
{
    int errcode = 0;
    regmatch_t pm = {0, 0};
    regoff_t total_length = strlen(string);
    regoff_t last_end = 0;
    int idx = 0;

    errcode = 0;

    pm.rm_so = 0;
    pm.rm_eo = total_length;

    errcode = regexec(re, string, 1, &pm, 0);
    while (errcode == 0) {

        fprintf(stdout, "idx: %2d, [%.*s] --- [%.*s] --- [%s] -- rest: %d, %d\n", idx++,
                pm.rm_so - last_end, string + last_end,
                pm.rm_eo - pm.rm_so, string + pm.rm_so,
                string + pm.rm_eo,
                pm.rm_eo,
                total_length - pm.rm_eo
                );

        last_end = pm.rm_eo;

        pm.rm_so = pm.rm_eo;
        pm.rm_eo = total_length - last_end;

        errcode = regexec(re, string, 1, &pm, REG_STARTEND);
        printf("errcode : %d\n", errcode);
    }

    return 1;
}

int match_string(const char *string, const char *pattern, int *match)
{
    regex_t re;
    int ret = 0;
    char buf[1024] = {0};

    ret = regcomp(&re, pattern, REG_EXTENDED | REG_NEWLINE);
    if (ret != 0) {
        (void)regerror(ret, &re, buf, sizeof(buf));
        fprintf(stderr, "regcomp failed: %s\n", buf);
    //    regfree(&re);
        return -1;
    }

    ret = print_details(&re, string);
    //ret = regexec(&re, string, 0, NULL, 0);

    regfree(&re);

    *match = (ret == 0);
    return 0;
}

int main(int argc, char **argv)
{
    FILE *fp = NULL;
    char buf_line[1024] =  {0};
    int matched = 0;
	mtrace();

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
        if (buf_line[strlen(buf_line) -1] == '\n')
            buf_line[strlen(buf_line) - 1] = '\0';

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
    }
    printf("do exit\n");

    return 0;
}
