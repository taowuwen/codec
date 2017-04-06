
#include "ems_core.h"
#include "ems_cmd.h"

#include <libgen.h>

typedef ems_int (*ems_cmd_entry)(ems_int cmd, ems_int argc, ems_char **argv);

typedef struct {
	ems_int       id; 
	ems_str       cmd;
	ems_cmd_entry entry;
} ems_cmd;


extern ems_int ems_main(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int cmd_main(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_c(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_ctrl(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_status(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_bwlist(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_radius(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_portal(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_qos(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_fw(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_app(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_user(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_wireless(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_network(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_config(ems_int cmd, ems_int argc, ems_char **argv);

extern ems_int main_test_radius(ems_int cmd, ems_int argc, ems_char **argv);
#ifdef FOR_TEST_INM
extern ems_int main_test_nm(ems_int cmd, ems_int argc, ems_char **argv);
#endif

#ifdef EMS_LOGGER_FILE
extern ems_int main_log(ems_int cmd, ems_int argc, ems_char **argv);
#endif

extern ems_int main_updatestatus(ems_int cmd, ems_int argc, ems_char **argv);
extern ems_int main_statistic(ems_int cmd, ems_int argc, ems_char **argv);

static ems_cmd ems_cmd_table[] = 
{
	{CMD_EMS,        ems_string("ems"),        ems_main},
	{CMD_EMS_C,      ems_string("ems_c"),      main_c},
	{CMD_EMS_CTRL,   ems_string("ems_ctrl"),   main_ctrl},
	{CMD_EMS_STATUS, ems_string("ems_status"), main_status},
	{CMD_EMS_BWLIST, ems_string("ems_bwlist"), main_bwlist},
	{CMD_EMS_RADIUS, ems_string("ems_radius"), main_radius},
	{CMD_EMS_PORTAL, ems_string("ems_portal"), main_portal},
	{CMD_EMS_QOS,    ems_string("ems_qos"),    main_qos},
	{CMD_EMS_FW,     ems_string("ems_fw"),     main_fw},
	{CMD_EMS_APP,    ems_string("ems_app"),    main_app},
	{CMD_EMS_USER,   ems_string("ems_user"),   main_user},
	{CMD_EMS_WIRELESS,ems_string("ems_wireless"), main_wireless},
	{CMD_EMS_NETWORK,ems_string("ems_network"),main_network},
	{CMD_EMS_CONFIG, ems_string("ems_config"), main_config},
	{CMD_EMS_TEST_RADIUS, ems_string("ems_test_radius"), main_test_radius},

	{CMD_GET_DC,           ems_string("ems_getdc"),      main_updatestatus},
	{CMD_GET_CONF,         ems_string("ems_getconf"),    main_updatestatus},
	{CMD_GET_UPDATEFILE,   ems_string("ems_getupdatefile"), main_updatestatus},
	{CMD_UPDATESTATUS,     ems_string("ems_updatestatus"),  main_updatestatus},
	{CMD_DOWNLOAD,         ems_string("ems_download"),      main_updatestatus},
	{CMD_STATICSTIC,       ems_string("ems_statistic"),     main_updatestatus},
	{CMD_TOTAL_INFO,       ems_string("ems_totalinfo"),     main_updatestatus}

#ifdef EMS_LOGGER_FILE
	,{CMD_EMS_LOG,    ems_string("ems_log"),    main_log}
#endif

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
		fprintf(stderr, "argv[%d] = %s\n", i, basename(argv[i]));
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
			ems_logger_init(&_glog, stdout, EMS_LOG_WARN);
			rtn = cmd->entry(cmd->id, argc, argv);
			ems_logger_uninit(&_glog);
			ems_stop_memory_trace();
			exit(rtn);
		}
	}

	ems_print_usage();

	return 1;
}

