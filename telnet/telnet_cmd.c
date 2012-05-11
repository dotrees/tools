
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "telnet_cmd.h"

#define TYPE_INVALID 100

static char cmd_type_name[CMD_TYPE_NUM][MAX_TYPE_SIZE] = {
																"display",
																"set",
																"monitor",
																"help",
																"logout",
																"clear"
															} ;

//register telnet command storage
static int cmd_num;
static telnet_cmd_t cmd[MAXCMDNUM]; 


char helpMenu[] = "********************************************************************************\r\n  " 
					 "                     Order Help \r\n"  
					 "            display :  output informations about                 \r\n"
					 "                set :  set                                       \r\n"
					 "            monitor :                                            \r\n"
					 "             logout :  leave the telnet                          \r\n"
                     "              clear :  clear the screen                          \r\n"					 
					 "\r\n********************************************************************************\r\n"; 
					 
//////////////////////////////////////////////////////////////////////////////////////
// Method  : register_cmd_function
// In      : cmd_name and three telnet_cmd_callback
// Out     : no
// Purpose : register the cmd_name and its callback to the  telnet_cmd_t cmd[MAXCMDNUM]
// Note    : the cmd_name size smaller than MAX_CMD_SIZE(31) 
//			 the number of cmd is smaller than MAXCMDNUM(128)				 
int register_telnet_cmd(const char *cmdname, 
				telnet_cmd_callback telnet_cmd_cb_display,
				telnet_cmd_callback telnet_cmd_cb_set,
				telnet_cmd_callback telnet_cmd_cb_monitor)
{
	if(cmd_num >= MAXCMDNUM)
	{
		perror("the max cmd number!! please reset the MAXCMDNUM");
		return 0;
	}
	else
	{
		if (strlen(cmdname) > MAX_CMD_SIZE - 1)
			{
				fprintf(stderr,"Warn :%s is longer  than 31,the register is not Success\n", cmdname);
				return 0;
			}
		
		if(memcpy(cmd[cmd_num].cmd_name,cmdname,strlen(cmdname)) == NULL)
		{
			perror("memcpy err");
		}
		
		cmd[cmd_num].telnet_cmd_cb_display = telnet_cmd_cb_display ;
		cmd[cmd_num].telnet_cmd_cb_set     = telnet_cmd_cb_set ;
		cmd[cmd_num].telnet_cmd_cb_monitor = telnet_cmd_cb_monitor ;
			
	}
	cmd_num++;
	return 0;
}
//////////////////////////////////////////////////////////////////////////////////
// Method  : match_function
// In      : the command to match
// Out     : adress of matched callback function
// Purpose : finds a callback function based on the command that
//           the user issued
// Note    : 
void * match_function( t_cmdtype type, const char *cmd_name)
{
	int i;
    for (i=0 ; i < cmd_num;  i++)
    {
    	if (strcmp(cmd_name, cmd[i].cmd_name) == 0 )
	    {
			switch (type)
			{
				case 0:
					return cmd[i].telnet_cmd_cb_display;
				case 1:
					return cmd[i].telnet_cmd_cb_set;
				case 2:
					return cmd[i].telnet_cmd_cb_monitor;
				default:
					break;
			}	
		}
    }

#ifdef TELNET_CMD_DEBUG    
	fprintf(stderr,"please input the right order,no func is register to %s\r\n",cmd_name);
#endif
 
	return NULL;
}

////////////////////////////////////////////////////////////////////////
// Method  : handle_blank_function
// In      : the pointer point to the blank
// Out     : the pointer point to the letter
// Purpose : let the pointer overleap excessive blank
// Note    : use for the function string_handle() and parameter_handle()
char *vk_space_handle(char *p)
{
	while((*p) == ' ')
	{
		p ++;
	}
	return p;
}

////////////////////////////////////////////////////////////////////////
// Method  : handle_string_function
// In      : the command string from the user
// Out     : no
// Purpose : parse the string to three parts : *cmd_type, *cmd_name and *parameter
//           and the cmd_type is display type=0 ,is set type=1 is monitor type=2 
// Note    : the length of three parts : MAX_TYPE_SIZE(16) MAX_CMD_SIZE(32) MAX_ORD_SIZE(256)  
void string_handle(int *type,char *type_cmd, 
		int *is_cmd_valid, char *cmd_name,
		char *parameter, char *command)
{
	
	char *pre = command;
	char *cur = NULL;
	int len = strlen (command);
	
	if (len > MAXBUF - 1)
	{
		len = MAXBUF - 1;
		command[MAXBUF - 1] = '\0';
	}
	
	cur = memchr(pre,' ',len +1);
	
	if (cur == NULL)
	{
		if (len > MAX_TYPE_SIZE - 1)
		{
			len = MAX_TYPE_SIZE - 1;			
//			wrtlog("the type commamd %s is longer than %d\n", command, MAX_TYPE_SIZE-1);
			
		#ifdef TELNET_CMD_DEBUG
			fprintf(stderr,"the type commamd %s is longer than %d\n", command, MAX_TYPE_SIZE-1);
		#endif
		}
		memcpy(type_cmd,pre,len);
		type_cmd[len] = '\0';
	}
	
	else
	{
		if ( (cur - pre) > MAX_TYPE_SIZE - 1)
		{
			memcpy(type_cmd,pre,MAX_TYPE_SIZE - 1);
			type_cmd[MAX_CMD_SIZE -1] = '\0';
//			wrtlog("the type commamd %s is longer than %d\n",type_cmd,MAX_TYPE_SIZE-1);
			
        #ifdef TELNET_CMD_DEBUG
			fprintf(stderr,"the type commamd %s is longer than %d\n",type_cmd,MAX_TYPE_SIZE-1);
		#endif
		}
		else
		{
			memcpy(type_cmd,pre,cur - pre );
			type_cmd[cur - pre] = '\0';
		}
		pre  = ++cur ;
		pre  = vk_space_handle(pre);
		cur = memchr(pre, ' ',command + len - pre + 1);
		
		if (cur == NULL)
		{
			if((command + len - pre) > MAX_CMD_SIZE -1 )
			{
				memcpy(cmd_name,pre,MAX_CMD_SIZE-1);
				cmd_name[MAX_CMD_SIZE - 1] = '\0';
//				wrtlog("the commamd name %s is longer than %d\n", cmd_name, MAX_CMD_SIZE-1);
				
            #ifdef TELNET_CMD_DEBUG
				fprintf(stderr,"the commamd name %s is longer than %d\n", cmd_name, MAX_CMD_SIZE-1);
            #endif
			}
			else
			{
			memcpy(cmd_name,pre,command + len - pre + 1);
			cmd_name[command + len - pre + 1] = '\0';
			*is_cmd_valid = 1;
			}
		}
		else
		{
			if ((cur - pre) > MAX_CMD_SIZE - 1 )
			{
				memcpy(cmd_name,pre,MAX_CMD_SIZE - 1);
				cmd_name[MAX_CMD_SIZE] = '\0';
//				wrtlog("the commamd name %s is longer than %d\n",cmd_name,MAX_CMD_SIZE-1);
				
			#ifdef TELNET_CMD_DEBUG
				fprintf(stderr,"the commamd name %s is longer than %d\n",cmd_name,MAX_CMD_SIZE-1);
			#endif				
			}
			memcpy(cmd_name,pre,cur - pre);
			*is_cmd_valid = 1;
			pre = ++cur ;
			pre = vk_space_handle(pre);
			memcpy(parameter,pre,command + len - pre);
		}
	}
	
	if (type_cmd!=NULL)
	{
		if (strcmp(type_cmd,cmd_type_name[1]) == 0)
		{
			*type = telnet_cmd_type_set ;
		}
		else if (strcmp(type_cmd,cmd_type_name[0]) == 0)
		{
			*type = telnet_cmd_type_display;
		}
		else if (strcmp(type_cmd,cmd_type_name[2]) == 0)
		{
			*type = telnet_cmd_type_monitor ;
		}
		else
		{
			*type = TYPE_INVALID;
		}
	}
	
#ifdef TELNET_CMD_DEBUG
	printf("%d\n",*type);	
	printf("%s\n",type_cmd);
	printf("cmd_name=%s\ncmd_name size = %d",cmd_name,strlen(type_cmd));
	printf("in string handle: parameter=%d\n",strlen(parameter));	
#endif			
}

////////////////////////////////////////////////////////////////////////
// Method  : malloc_argv_function
// In      : string and length
// Out     : pointer to malloc memory
// Purpose : memory
// Note    : if not malloc success return NULL
char * copy_to_argv(const char *str, int len)
{
	char *dest = NULL;
 	if((dest = (char *)malloc(len + 1)) == NULL)
	{

	#ifdef TELNET_CMD_DEBUG
		fprintf(stderr,"the malloc argv[] is not success");
	#endif
//		wrtlog("the malloc argv[] is not success");
		return NULL;
	}
	memcpy(dest,str,len);
	return dest;
}

////////////////////////////////////////////////////////////////////////
// Method  : free_argv_function
void free_argv(int argc,char **argv)
{
	int i=0;
	for (i = 0; i< argc; i++)
	{
		free(argv[i]);
		argv[i] = NULL;
	}
}

////////////////////////////////////////////////////////////////////////
// Method  : handle_parameter_function
// In      : *parameter
// Out     : no
// Purpose : parse the string to *argv[MAX_ORD_SIZE] and the argc 
//           argc and *argv[] for the callback function
// Note    : *parameter include orders is not more than MAX_ORD_SIZE(16)
void parameter_handle(int *argc, char **argv, char *parameter)
{
	
    char *pre=parameter;
	char *cur=NULL;
   	int len = strlen(parameter);
	*argc=0;
	if(strlen(parameter)!=0)
	{               
		while(((cur=memchr(pre,' ',parameter+len-pre))!=0) && ((*argc) < MAX_ORD_NUM ) )
		{
			argv[(*argc)] = copy_to_argv(pre, cur-pre);
			if (argv[*argc] == NULL)
			{
				return;
			}
			
			argv[(*argc)][cur-pre] = '\0';
			(*argc) ++;
			while(cur<parameter+len && *cur==' ')
		 	{
				pre = ++cur;
				pre = vk_space_handle(pre);
			}	
		}
		if((pre<parameter+len)&&((*argc) < MAX_ORD_NUM))
		{
	  		argv[(*argc)] = copy_to_argv(pre, parameter + len-pre );
			if(argv[(*argc)] == NULL)
			{
				return;
			}
	 		argv[(*argc)][parameter + len - pre] = '\0';
	  		++(*argc);
		}
		if( (*argc) >= MAX_ORD_NUM )
		{
 //		  wrtlog("\r\nthe ord_num is more than the MAX_ORD_NUM\r\n",MAX_ORD_NUM);

		#ifdef TELNET_CMD_DEBUG
			fprintf(stderr,"\r\nthe ord_num is more than the MAX_ORD_NUM:%d\r\n",MAX_ORD_NUM);
		#endif
		}
	}
	
#ifdef TELNET_CMD_DEBUG
	int i = 0;
	for(i = 0; i< (*argc); i++)
	{
		printf("argv[%d] = %s\n",i,argv[i]);
	}
#endif
}

////////////////////////////////////////////////////////////////////////
// Method  : out_command_function
// In      : the type and outbuf of store command 
// Out     : no
// Purpose : input the type name return the register's cmd_names of the type
//			 for example : input (set)   return  (tunnel , net,.... )
int out_cmdname(t_cmdtype type,char *outbuf)
{
	char *err = "please input the right command!!\r\n";
	char *no_register_command = "this type name have no register command,please register first\r\n";
	char *p = outbuf;
	struct session *sp;
	sp=get_session();
	switch(type)
	{
		case telnet_cmd_type_display:
			{
				int i;
				int j=0;  //一行输出2个cmd_name
				for (i=0; i< cmd_num; i++)
				{
					if(cmd[i].telnet_cmd_cb_display != NULL)
					{	
						int len=strlen(cmd[i].cmd_name);
							if (len > MAX_CMD_SIZE)
								len=MAX_CMD_SIZE;
						memcpy(p, cmd[i].cmd_name,len);
						memset(p+len,' ',MAX_CMD_SIZE-len);
						p += MAX_CMD_SIZE;
		
						j++;
						if ( j%2 == 0)
						{
							memcpy(p,"\r\n",2) ;
							p += 2;
							cmd_output(sp, "   ",3);
							cmd_output(sp,outbuf,strlen(outbuf));
							memset(outbuf,0,CMD_OUTBUF);
							p = outbuf;
						}
					}
				}
				if (j%2 != 0)
					memcpy(p,"\r\n",2);
					
				if (j != 0)
				{
					cmd_output(sp, "   ",3);
					cmd_output(sp,outbuf,strlen(outbuf));
					return 0;
				}
				else
				{
					cmd_output(sp,no_register_command, strlen(no_register_command));
					return 0;
				}
			}			
		case telnet_cmd_type_set:
			{
				int i;
				int j=0;  //一行输出2个cmd_name
				for (i=0; i< cmd_num; i++)
				{
					if(cmd[i].telnet_cmd_cb_set != NULL)
					{	
						int len=strlen(cmd[i].cmd_name);
							if (len > MAX_CMD_SIZE)
								len=MAX_CMD_SIZE;
						memcpy(p, cmd[i].cmd_name,len);
						memset(p+len,' ',MAX_CMD_SIZE-len);
						p += MAX_CMD_SIZE;
		
						j++;
						if ( j%2 == 0)
						{
							memcpy(p,"\r\n",2) ;
							p += 2;
							cmd_output(sp, "   ",3);
							cmd_output(sp,outbuf,strlen(outbuf));
							memset(outbuf,0,CMD_OUTBUF);
							p = outbuf;
						}
					}
				}
				if (j%2 != 0)
					memcpy(p,"\r\n",2);
				if (j != 0)
				{
					cmd_output(sp, "   ",3);
					cmd_output(sp,outbuf,strlen(outbuf));
					return 0;
				}
				else
				{
					cmd_output(sp,no_register_command, strlen(no_register_command));
					return 0;
				}
			}			
		case telnet_cmd_type_monitor:
			{
				int i;
				int j=0;  					//一行输出2个cmd_name
				for (i=0; i< cmd_num; i++)
				{
					if(cmd[i].telnet_cmd_cb_monitor != NULL)
					{	
						int len=strlen(cmd[i].cmd_name);
							if (len >  MAX_CMD_SIZE)
								len= MAX_CMD_SIZE;
						memcpy(p, cmd[i].cmd_name,len);
						memset(p+len,' ', MAX_CMD_SIZE-len);
						p += MAX_CMD_SIZE;
		
						j++;
						if ( j%2 == 0)
						{
							memcpy(p,"\r\n",2) ;
							p += 2;
							cmd_output(sp, "   ",3);
							cmd_output(sp,outbuf,strlen(outbuf));
							memset(outbuf,0,CMD_OUTBUF);
							p = outbuf;
						}
					}
				}
				if (j%2 != 0)
					memcpy(p,"\r\n",2);
				if (j != 0)
				{
					cmd_output(sp, "   ",3);
					cmd_output(sp,outbuf,strlen(outbuf));
					return 0;
				}
				else
				{
					cmd_output(sp,no_register_command, strlen(no_register_command));
					return 0;
				}
			}	
		default:
			{
				cmd_output(sp,err,strlen(err));
				return 0;
			}
	}
}

////////////////////////////////////////////////////////////////////////
// Method  : ensure_type_function
// In      : type
// Out     : telnet_cmd_callback 
// Purpose : get the callback of the type
// Note    : 
telnet_cmd_callback ensure_type(int i,t_cmdtype type)
{
	switch(type)
	{
		case 0:
			return cmd[i].telnet_cmd_cb_display;
		case 1:
			return cmd[i].telnet_cmd_cb_set;
		case 2:
			return cmd[i].telnet_cmd_cb_monitor;
	}
} 

////////////////////////////////////////////////////////////////////////
// Method  : match_cmd_out_function
// In      : the string for match and  the outbuf for return match result
// Out     : if match seccess return length of outbuf if not return 0
// Purpose : for the key of TAB
// Note    : 
int match_cmd_out(char *match_buf, char *outbuf)
{
	char cmd_type[MAX_TYPE_SIZE];
	char cmd_name[MAX_CMD_SIZE];
	char parameter[MAXBUF]; 
	memset(cmd_type, 0, MAX_TYPE_SIZE);
	memset(cmd_name, 0, MAX_CMD_SIZE);
	t_cmdtype type = 100;
	int is_cmd_valid = 0;

	string_handle((int *)(&type), cmd_type, &is_cmd_valid, cmd_name, parameter, match_buf);

	if (is_cmd_valid == 0)
	{
		int len = strlen(cmd_type);
		int i = 0;
		int j = 0;
		for(i = 0; i < CMD_TYPE_NUM; i++)
		{
			if((strncmp(cmd_type,cmd_type_name[i],len)==0) && (len < strlen(cmd_type_name[i])+1))
			{
					if (j > 0)
						return 0;
					memcpy(outbuf,cmd_type_name[i],strlen(cmd_type_name[i]));
					j++;
			}
		}
		if (j == 1)
			return strlen(outbuf);
	}
	else if (type != TYPE_INVALID)
	{
	    int len=strlen(cmd_type);
		int i = 0;
		int j = 0;	
		for(i = 0; i < cmd_num; i++)
		{
			if(   (strncmp(cmd_name,cmd[i].cmd_name,strlen(cmd_name))==0) 
				&& (ensure_type(i,type)!= NULL)
				&&	(strlen(cmd_name )< strlen(cmd[i].cmd_name)+1)
			   )
			{
				if (j > 0)
					return 0;
				memcpy(outbuf,cmd_type,len);
				outbuf[len] = ' ';
				memcpy(outbuf+len+1,cmd[i].cmd_name,strlen(cmd[i].cmd_name));
				j++;
				
			}
		}
		if (j == 1)
			return	strlen(outbuf)	;	
	}
	else
	   return 0;	
	
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////
// Method  : interface_related_telnet_function
// In      : the string from user
// Out     : if the command include '?' return the cammand length . others output direct
// Purpose : interface related with telnet protocol ,let the command from user
//           parse and run the command ,return the result what the command want        
// Note    : if the command is not right, the function will tell .
//         : tranfer the callback function: (*func)(argc,(char **)argv,parameter)
//         : three parameters: argc, **crgc ,*parameter get from string_handle and 
//		   : parameter_handle()           

int telnet_cmd_process(char *command, int cmd_len, char *temp_outbuf, int outbuflen)
{

	struct session *sp=get_session();
	char outbuf[CMD_OUTBUF];	
	memset(outbuf,0,CMD_OUTBUF);
#ifdef TELNET_CMD_DEBUG
	fprintf(stderr, "command = %s\n",command);
#endif
	
	char msg_ord_err[] = "Illegal command! Please input again or input \'help\' for help!\r\n"; 
	char mesgcmderr[] = "Illegal command! Please input again or input \'set\',\'display\',\'monitor\' !\r\n";
	char type_err[]="no match to input ,Please input again or input \'help\' for help!\r\n"; 
	char cmd_err[] ="no match to input ,Please input again or input \'set\',\'display\',\'monitor\' !\r\n";
	
	t_cmdtype type = 100;
	
	char cmd_type[MAX_TYPE_SIZE];
	char cmd_name[MAX_CMD_SIZE];
	char parameter[MAXBUF]; 
	
	int  argc = 0;
	char *argv[MAX_ORD_NUM];
	int is_cmd_valid = 0;
	
	/** init variable */
	memset(cmd_type, 0, MAX_TYPE_SIZE);
	memset(cmd_name, 0, MAX_CMD_SIZE);
	memset(parameter, 0, MAXBUF);

	string_handle((int *)(&type), cmd_type, &is_cmd_valid, cmd_name, parameter, command);
	
	if (is_cmd_valid == 0 )
	{
	
	#ifdef TELNET_CMD_DEBUG
		fprintf(stderr,"cmd_name is NULL\n");
	#endif
	
		if (   ((strlen(cmd_type_name[1]) == cmd_len )&&(strncmp(cmd_type,cmd_type_name[1],strlen(cmd_type)) == 0) ) 
					|| ((strlen(cmd_type_name[0]) == cmd_len )&&(strncmp(cmd_type,cmd_type_name[0],strlen(cmd_type)) == 0) )
					|| ((strlen(cmd_type_name[2]) == cmd_len )&&(strncmp(cmd_type,cmd_type_name[2],strlen(cmd_type)) == 0) ) 
				)
		{
			out_cmdname(type,outbuf);
			return 0;
		}

	   else if (cmd_type[cmd_len-1] == '?')
		{
			char *p = outbuf;
			if((cmd_type[0] == '?') && strlen(cmd_type) == 1 )
			{
				int i;
				int j=0;
				for (i = 0; i < CMD_TYPE_NUM; i++)
				{
					memcpy(p,cmd_type_name[i],strlen(cmd_type_name[i]));
					memset(p+strlen(cmd_type_name[i]),' ',MAX_TYPE_SIZE - strlen(cmd_type_name[i]));
					p += MAX_TYPE_SIZE;
					j++;
					if (j%4 == 0)
					{
						memcpy(p,"\r\n",2) ;
						p += 2;
						cmd_output(sp, "   ",3);
						cmd_output(sp,outbuf,strlen(outbuf));
						memset(outbuf,0,CMD_OUTBUF);
						p = outbuf;
					}
				}
				if(j%4 != 0)
					memcpy(p,"\r\n",2);				
			    if (j != 0)
				{
					cmd_output(sp, "   ",3);
					cmd_output(sp,outbuf,strlen(outbuf));
					return 0;
				}
			    else
			    {
					char no_type[] = "have no type name, please set the type name like  display set monitor!!\r\n";
					cmd_output(sp,no_type,strlen(no_type));
					return 0;
			    }	   
			}
			else
			{
				int len = strlen(cmd_type);
				int i = 0;
				int j = 0;
				for(i = 0; i < CMD_TYPE_NUM; i++)
				{
					if((strncmp(cmd_type,cmd_type_name[i],strlen(cmd_type)-1)==0) && (strlen(cmd_type) < strlen(cmd_type_name[i])+2))
					{
						memcpy(p,cmd_type_name[i],strlen(cmd_type_name[i]));
						memset(p+strlen(cmd_type_name[i]),' ',MAX_TYPE_SIZE - strlen(cmd_type_name[i]));
						//printf("*p = %s \n",p);
						p += MAX_TYPE_SIZE;
						j++;
						if (j%4 == 0)
						{
							memcpy(p,"\r\n",2) ;
							p += 2;
							cmd_output(sp, "   ",3);
							cmd_output(sp,outbuf,strlen(outbuf));
							memset(outbuf,0,CMD_OUTBUF);
							p = outbuf;
						}
					}
				}
				if(j%4 != 0)
					memcpy(p,"\r\n",2);
				if (j != 0)
				{
					cmd_output(sp, "   ",3);
					cmd_output(sp,outbuf,strlen(outbuf));
				}
				else
				{
					cmd_output(sp,type_err,strlen(type_err));
					return 0;
				}
				
				memset(temp_outbuf,0,strlen(temp_outbuf));
				memcpy(temp_outbuf,command,strlen(command)-1);
				return strlen(temp_outbuf);
			}		
		}
		
		
		else if ((strlen(cmd_type_name[3]) == cmd_len )&&(strncmp(cmd_type,cmd_type_name[3],cmd_len) == 0))
		{
			cmd_output(sp,helpMenu,strlen(helpMenu));
			return 0;
		} 		
		else
		{
			char *p = outbuf;
			int i = 0;
			int j = 0;
			for(i = 0; i < CMD_TYPE_NUM; i++)
			{
				if((strncmp(cmd_type,cmd_type_name[i],strlen(cmd_type))==0) && (strlen(cmd_type) < strlen(cmd_type_name[i])+1))
				{
					memcpy(p,cmd_type_name[i],strlen(cmd_type_name[i]));
					memset(p+strlen(cmd_type_name[i]),' ',MAX_TYPE_SIZE - strlen(cmd_type_name[i]));
					//printf("*p = %s \n",p);
					p += MAX_TYPE_SIZE;
					j++;
					if (j%4 == 0)
					{
						memcpy(p,"\r\n",2) ;
						p += 2;
						cmd_output(sp, "   ",3);
						cmd_output(sp,outbuf,strlen(outbuf));
						memset(outbuf,0,CMD_OUTBUF);
						p = outbuf;
					}
				}
			}
			if(j%4 != 0)
				memcpy(p,"\r\n",2);
				
			if (j != 0)
			{
				cmd_output(sp, "   ",3);
				cmd_output(sp,outbuf,strlen(outbuf));
			}
			else
			{
				cmd_output(sp,msg_ord_err,strlen(msg_ord_err));
				return 0;
			}
			
			memset(temp_outbuf,0,strlen(temp_outbuf));
			memcpy(temp_outbuf,command,strlen(command));
			return strlen(temp_outbuf);
		}
	}
	
	else if(    ((strlen(cmd_type_name[1]) == strlen(cmd_type) )&&(strncmp(cmd_type,cmd_type_name[1],strlen(cmd_type)) == 0) ) 
				|| ((strlen(cmd_type_name[0]) == strlen(cmd_type) )&&(strncmp(cmd_type,cmd_type_name[0],strlen(cmd_type)) == 0) )
				|| ((strlen(cmd_type_name[2]) == strlen(cmd_type) )&&(strncmp(cmd_type,cmd_type_name[2],strlen(cmd_type)) == 0) ) 
			)
	{
	#ifdef  TELNET_CMD_DEBUG
		fprintf(stderr,"cmd_name is not NULL\n");
	#endif
		if (cmd_name[strlen(cmd_name)-1] == '?')
		{
			char *p = outbuf;
			if ( (strlen(cmd_name)==1) && cmd_name[0] == '?' )
			{
				out_cmdname(type,p);
				return 0;
			}
			
			else
			{
				int i = 0;
				int j = 0;	
				for(i = 0; i < cmd_num; i++)
				{
					if(		(strncmp(cmd_name,cmd[i].cmd_name,strlen(cmd_name)-1)==0) 
							&& (ensure_type(i,type)!= NULL)
							&&	(strlen(cmd_name )< strlen(cmd[i].cmd_name)+2)
					   )
					{
						memcpy(p,cmd[i].cmd_name,strlen(cmd[i].cmd_name));
						memset(p+strlen(cmd[i].cmd_name),' ',MAX_CMD_SIZE - strlen(cmd[i].cmd_name));
						p += MAX_CMD_SIZE;
						j++;
						if (j%2 == 0)
						{
							memcpy(p,"\r\n",2) ;
							p += 2;
							cmd_output(sp, "   ",3);
							cmd_output(sp,outbuf,strlen(outbuf));
							memset(outbuf,0,CMD_OUTBUF);
							p = outbuf;	
						}
					}
				}
				if (j%2 != 0)
					memcpy(p,"\r\n",2);
					
				if (j != 0)
				{
					cmd_output(sp, "   ",3);
					cmd_output(sp,outbuf,strlen(outbuf));
				}
				else
				{
					cmd_output(sp,cmd_err,strlen(cmd_err));
					return 0;
				}

				memset(temp_outbuf,0,strlen(temp_outbuf));
				memcpy(temp_outbuf,command,strlen(command)-1);
				return strlen(temp_outbuf);
			}
			
		}
		else
		{
			parameter_handle((&argc), argv, parameter);
		
			telnet_cmd_callback func = NULL;
			func = match_function( type, cmd_name);
		
			if (func == NULL)
			{
				char *p = outbuf;
				int i = 0;
				int j = 0;	
				for(i = 0; i < cmd_num; i++)
				{
					if(		(strncmp(cmd_name,cmd[i].cmd_name,strlen(cmd_name))==0) 
							&& (ensure_type(i,type)!= NULL)
							&&	(strlen(cmd_name )< strlen(cmd[i].cmd_name)+1)
					   )
					{
						memcpy(p,cmd[i].cmd_name,strlen(cmd[i].cmd_name));
						memset(p+strlen(cmd[i].cmd_name),' ',MAX_CMD_SIZE - strlen(cmd[i].cmd_name));
						p += MAX_CMD_SIZE;
						j++;
						if (j%2 == 0)
						{
							memcpy(p,"\r\n",2) ;
							p += 2;
							cmd_output(sp, "   ",3);
							cmd_output(sp,outbuf,strlen(outbuf));
							memset(outbuf,0,CMD_OUTBUF);
							p = outbuf;	
						}
					}
				}
				if (j%2 != 0)
					memcpy(p,"\r\n",2);
					
				if (j != 0)
				{
					cmd_output(sp, "   ",3);
					cmd_output(sp,outbuf,strlen(outbuf));
				}
				else
				{
					cmd_output(sp,mesgcmderr,strlen(mesgcmderr));
					return 0;
				}
				
				memset(temp_outbuf,0,strlen(temp_outbuf));
				memcpy(temp_outbuf,command,strlen(command));
				return strlen(temp_outbuf);
			}
		
			else
			{
			#ifdef TELNET_CMD_DEBUG
				fprintf(stderr,"argc = %d\n",argc);
				int index;
				for(index = 0;index<argc;index++)
				{
					fprintf(stderr,"argcment: %s\n",argv[index]);
				}
			#endif
			
				(*func)(argc,(char **)argv,parameter);
				
				/** free memory alloced */
				free_argv(argc, argv);
			}
			
		}
	}
	
	else
	{
		cmd_output(sp,msg_ord_err,sizeof(msg_ord_err));
		return 0;
	}	
	return 0;
}



