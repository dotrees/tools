#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <sys/time.h>

int i=0;
void display_func(int argc,char **argv,char *arg,char *outbuf,int outbuflen)
{
	fprintf(stderr,"argc = %d\n",argc);
	fprintf(stderr,"raw = %s\n",arg);
	int index=0;
	for(index=0;index<argc;index++)
	{
		fprintf(stderr,"argument = %s\n",(char *)(argv[index]));
	}
	
	//sprintf(outbuf,"%s\r\n",arg);
	char *help="you can display tunnel_speed\r\n";
	char *speed="tunnel_speed";
	char *vari="i";
	if(argc==0)
	{
	    sprintf(outbuf,help);
		return;
	}
	if(strcmp(argv[0],speed)==0)
	{
	   if(argc<2)
	   {
	     sprintf(outbuf,"display tunnel %s %s\r\n",speed,vari);
	   }
	   else if(strncmp(argv[1],vari,strlen(vari))==0)
	   {
	      sprintf(outbuf,"tunnel_speed=%d\r\n",i);
	   }
	   else
	   {
	     sprintf(outbuf,"no tunnel speed defined by %s\r\n",argv[1]);
	   }
	}
	else
	{
	   sprintf(outbuf,"display %s %s\r\n",speed,vari);
	}
}
void set_func(int argc,char **argv,char *arg,char *outbuf,int outbuflen)
{
	sprintf(outbuf,"%s\r\n%#.5f\r\n",arg,11.11);
}


int main()
{
	/** register process func */
	register_telnet_cmd("tunnel",display_func,set_func,NULL);
	char *command = "display tunnel";
	char outbuffer[2048];
	int returnlen;
	returnlen = telnet_cmd_process(command,strlen(command),outbuffer,2048);
	
	fprintf(stderr,"content of outbuffer:%s\n",outbuffer);
	fprintf(stderr,"return len = %d\n",returnlen);
	
	return 0;
}

