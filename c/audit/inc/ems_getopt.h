#ifndef EMS_GETOPT_H
#define EMS_GETOPT_H


#ifdef WIN32
int ems_getopt(int nargc, char * const nargv[], const char *ostr);
char    *optarg;
#endif

#endif
