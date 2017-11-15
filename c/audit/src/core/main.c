
#include "ems_core.h"

#include <libgen.h>

typedef ems_int (*ems_cmd_entry)(ems_int cmd, ems_int argc, ems_char **argv);

typedef struct {
	ems_int       id; 
	ems_str       cmd;
	ems_cmd_entry entry;
} ems_cmd;

extern ems_int ems_main(ems_int cmd, ems_int argc, ems_char **argv);

static ems_cmd ems_cmd_table[] = 
{
	{0,        ems_string("mpaudit"),        ems_main},
};

#define SIZE_TABLE	sizeof(ems_cmd_table)/sizeof(ems_cmd)

ems_logger  _glog;

ems_logger  *logger()
{
	return &_glog;
}

#ifdef DEBUG
static ems_int
do_test(ems_int argc, ems_char **argv)
{
	ems_int i;

	for (i = 0; i < argc; i++) 
	{
		printf("argv[%d] = %s\n", i, basename(argv[i]));
	}

	return EMS_OK;
}
#endif

static ems_void ems_print_usage()
{
	ems_int  i;
	ems_cmd *cmd;

	printf("help EMS Cmd lists:\n");

	for (i = 0; i < SIZE_TABLE; i++) {
		cmd = &ems_cmd_table[i];

		printf("\n %s", str_text(&cmd->cmd));
	}

	printf("\n");
}


ems_int main(ems_int argc, ems_char **argv)
{
	ems_int  rtn;
	ems_int  i;
	ems_cmd *cmd;

#ifdef DEBUG
	ems_reset_rlimit();
	do_test(argc, argv);
#endif

	for (i = 0; i < SIZE_TABLE; i++) {
		cmd = &ems_cmd_table[i];

		if (!strcmp(str_text(&cmd->cmd), basename(argv[0]))) 
		{
			ems_start_memory_trace(EMS_YES);
			ems_logger_init(&_glog, stdout, EMS_LOG_TRACE);
			rtn = cmd->entry(cmd->id, argc, argv);
			ems_logger_uninit(&_glog);
			ems_stop_memory_trace();
			exit(rtn);
		}
	}

	ems_print_usage();

	return 1;
}

