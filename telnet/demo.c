#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

/** includes such 2 header files*/
#include "telnet_cmd.h"
#include "telnet_server.h"
#define MAX_BUF 128
int i=259;
void *set_content(struct session* sp)
{	
    char *hint="\n\rpress ctrl+c to stop monitor\r\n";
    cmd_output(sp,hint,strlen(hint));
	while(1){
        time_t cur_time;
        char out_buffer[MAX_BUF];
        time(&cur_time);
		if(!continue_disp(sp))//check loop flag
            break;
		sprintf(out_buffer,"i= %d time: %s\r",i,ctime(&cur_time));
		cmd_output(sp,out_buffer,strlen(out_buffer));
		cmd_out_flush(sp);
		net_flush(sp);
		usleep(2000000);
	}
	end_monitor(sp);
	//pthread_exit(NULL);
	return NULL;
	
}

void monitor_func(int argc,char **argv,char *raw)
{
	/** validate */
	char *error="monitor tunnel i\r\n";
	struct session *sp;
	sp=get_session();
	if(argc==0)
	{
		cmd_output(sp,error,strlen(error)+1);
		return;
	}
	if(strcmp(argv[0],"i")!=0)
	{
		cmd_output(sp,error,strlen(error)+1);
		return;
	}
	/** start fix content thread */
	if(!create_monitor(sp,NULL,(func_t)set_content,(void*)sp))
	{
		wrtlog("From %s cannot create read blacklist thread\n",sp->client_addr);
		return;
	}
	
}
void display_func(int argc,char **argv,char *raw)
{
	char outbuf[256];
	memset(outbuf,0,256);
	char help[]="you can display tunnel_speed\r\n";
	char speed[]="tunnel_speed";
	char vari[]="i";
	struct session *sp;
	sp=get_session();
	if(argc==0)
	{   
		cmd_output(sp,help,strlen(help));
		return;
	}
			
	if(strlen(speed)==strlen(argv[0])&&strncmp(argv[0],speed,strlen(speed))==0)
	{
		if(argc<2)
		{
		    sprintf(outbuf,"display tunnel %s %s\r\n",speed,vari);
			cmd_output(sp,outbuf,strlen(outbuf));
			return;
		}
		else if((strncmp(argv[1],vari,strlen(vari))==0) && (argc>1))
		{
			if(argc == 2)
			{
				sprintf(outbuf,"tunnel_speed=%d\r\n",i);
				cmd_output(sp,outbuf,strlen(outbuf));
				return;
			}
			else
			{
				sprintf(outbuf,"too more parameters after %s %s you can display %s %s\r\n",speed,vari,speed,vari);
				cmd_output(sp,outbuf,strlen(outbuf));
				return;
			}	
		}
		else
		{
		    sprintf(outbuf,"no tunnel speed defined by %s\r\n",argv[1]);
			cmd_output(sp,outbuf,strlen(outbuf));
			return;
		}
	}
	else
	{
		sprintf(outbuf,"display %s %s\r\n",speed,vari);
		cmd_output(sp,outbuf,strlen(outbuf));
		return;
	}
}
void set_func(int argc,char **argv,char *raw)
{
    struct session *sp;
	sp=get_session();
	char outbuf[256];
	memset(outbuf,0,256);
	sprintf(outbuf,"%s\r\n%#.5f\r\n",raw,11.11);
	cmd_output(sp,outbuf,strlen(outbuf));
}

/**
	0---23
	!0---any
*/
void *thread_function()
{
	init_server_cfg();
	telnet_service(NULL, 0);
	pthread_exit(NULL);
}

static void sig_handler(int signo)
{
	switch(signo){
	case SIGINT:
		fprintf(stderr, "end by user\n");
		exit(1);
	default:
		fprintf(stderr, "unknow signal\n");
		break;
	}
}

int main()
{
	/** start telnet thread */
	pthread_t thread_id;
	if(pthread_create(&thread_id,NULL,thread_function,NULL))
	{
		perror("cannot create read blacklist thread");
		exit(1);
	}
	if(pthread_detach(thread_id) != 0)
	{
		perror("cannot detach thread");
		exit(1);
	}
	if(signal(SIGINT,sig_handler) == SIG_ERR){
		fprintf(stderr, "Unable create handle for SIGINT");
		return -1;
	}
	
	/** register process func */
	register_telnet_cmd("tunnel",display_func,set_func,monitor_func);
	
	/** print variable i */
	while(1)
	{
		fprintf(stderr,"main process while loop\n");
		fprintf(stderr,"i=%d\n",i++);
		usleep(500000000);
	}
					
	return 0;
}

