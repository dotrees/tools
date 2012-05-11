#ifndef _TELNET_CMD_H_
#define _TELNET_CMD_H_

#include <stdio.h>
#include "telnet_server.h"
#define MAXBUF 256

#define CMD_OUTBUF 256
#define MAXCMDNUM 128
#define CMD_TYPE_NUM  6
#define MAX_TYPE_SIZE 16
#define MAX_CMD_SIZE 32

#define MAX_ORD_NUM  16
//#define TELNET_CMD_DEBUG  


#ifdef __cplusplus
extern "C" {
#endif

/** interface related with telnet protocol */
int telnet_cmd_process(char *command, int cmd_len, char *tem_outbuf, int outbuflen);


/** data structure and interface related with internal command process */

typedef enum __telnet_cmd_type_ {
	telnet_cmd_type_display = 0,
	telnet_cmd_type_set = 1,
	telnet_cmd_type_monitor = 2,
} t_cmdtype;

typedef void (*telnet_cmd_callback)(int argc, 
									char **argv,
									char *raw);
								//	char *outbuf,        // store return value
								//	int outbuflen);  // length of return content

typedef struct _telnet_cmd_ {
	//int cmd_type;
	char cmd_name[32];
	telnet_cmd_callback  telnet_cmd_cb_display; //for display cb
	telnet_cmd_callback  telnet_cmd_cb_set;     //for set cb
	telnet_cmd_callback  telnet_cmd_cb_monitor; //for monitor cb
} telnet_cmd_t;

int register_telnet_cmd(char const *typename1, 
				telnet_cmd_callback telnet_cmd_cb_display,
				telnet_cmd_callback telnet_cmd_cb_set,
				telnet_cmd_callback telnet_cmd_cb_monitor);

void *match_function( t_cmdtype type, const char *cmd_name);

char *vk_space_handle(char * p);
void string_handle(int *type,char *type_cmd, 
		int *is_cmd_valid, char *cmd_name,
		char *parameter, char *command);
char * copy_to_argv(const char *str, int len);
void free_argv(int argc,char **argv);

telnet_cmd_callback ensure_type(int i,t_cmdtype type);
int match_cmd_out(char *match_buf, char *outbuf);

void parameter_handle(int *argc, char **argv, char *parameter);
int  out_cmdname(t_cmdtype type, char *outbuf);

#ifdef __cplusplus
}
#endif

#endif
