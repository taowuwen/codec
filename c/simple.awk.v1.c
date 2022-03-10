
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <regex.h>
#include <errno.h>
#include <mcheck.h>

const char *str_sub(const char *string, int s, int e)
{
    static char buf[1024] = {0};
    memcpy(buf, string + s, e-s);
    buf[e - s] = '\0';
    return buf;
}

int print_details(regex_t *re, const char *string)
{
    int error;

    regmatch_t pm = {0, 0};

    printf("current string: %s\n", string);
    error = regexec(re, string, 1, &pm, 0);
    while (error == 0) {

        printf("matched idx: (%d, %d) %s: ", pm.rm_so, pm.rm_eo, str_sub(string, 0, pm.rm_so));
        printf(" --> SPLITOR: %s <-- \n", str_sub(string, pm.rm_so, pm.rm_eo));
        printf("rest string: %s\n", string + pm.rm_eo);

        string += pm.rm_eo;
        
        /* While matches found. */
        /* Substring found between pm.rm_so and pm.rm_eo. */
        /* This call to regexec() finds the next match. */
//        error = regexec(re, string + pm.rm_eo, 1, &pm, REG_NOTBOL);
        error = regexec(re, string, 1, &pm, REG_NOTBOL);

        if (*string == '\n') {
            break;
        }
    }

    /*
    for (int i = 0; i < nmatch && ((pmatch +i)->rm_so != -1); i++) {
        s = (pmatch + i)->rm_so;
        e = (pmatch + i)->rm_eo;

        printf("%02d, matched idx: %d, %d, %s: ", i, s, e, str_sub(string, old_end, s));
        printf(" --> SPLITOR: %s <-- \n", str_sub(string, s, e));
        old_end = e;
    }
    */

    return 0;
}


int match_string(const char *string, const char *pattern, int *match)
{
    regex_t re;
    int ret = 0;
    char buf[1024] = {0};

    ret = regcomp(&re, pattern, REG_EXTENDED);
    if (ret != 0) {
        (void)regerror(ret, &re, buf, sizeof(buf));
        fprintf(stderr, "regcomp failed: %s\n", buf);
        return -1;
    }

    if (0) {
        ret = print_details(&re, string);
    } else {
        ret = regexec(&re, string, 0, NULL, 0);
    }

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
