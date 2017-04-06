#ifndef DT_GETOPT_H
#define DT_GETOPT_H


#ifdef WIN32
int dt_getopt(int nargc, char * const nargv[], const char *ostr);
char    *optarg;
#endif

#endif
