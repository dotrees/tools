#define TELOPTS
#define	SLC_NAMES
#define TELCMDS
#include "telnet_server.h"
char *telnet_line_mode[]={"NO_LINEMODE","KLUDGE_LINEMODE","REAL_LINEMODE"};
char *telnet_master_state[]={"SDATA","SIAC","SDOOPT","SWILL_OPT","SUB_NEG","SUBIAC","SCR"};
char *telnet_sub_state[]={"SUB_START","SUB_LINEMODE","SUB_TERMTYPE","SUBTERM_REQUEST","GETTERM","LMODE","GET_SLC",
                         "SNAWS","SUBEND","LMDO","LMWILL"};

char doopt[] = { IAC, DO, '%', 'c', 0 };
char dont[] = { IAC, DONT, '%', 'c', 0 };
char will[] = { IAC, WILL, '%', 'c', 0 };
char wont[] = { IAC, WONT, '%', 'c', 0 };
char crlf[] = { '\r','\n',0};
#ifdef DEFAULTPROMPT
char telnet_log[]=DEFAULTPROMPT;
#else
char telnet_log[]="sdfs # ";
#endif
#ifdef LOGOUT
char telnet_quit[]=LOGOUT;
#else
char telnet_quit[]="logout";
#endif
//处理iac datamark
int datamark(struct session *sp,char c)
{
	wrtlog("From %s in datamark\n\r",sp->client_addr);
	if(sp->syn_num>0)
		sp->syn_num--;
	return 1;
}
//处理iac ip
int do_ip(struct session *sp,char c)
{
    wrtlog("From %s in do ip\n",sp->client_addr);
    set_disp(sp,0);
    sp->cmd_inlen=0;
    sp->cmd_inp=sp->cmd_inbuffer;
   //net_put(telnet_log);
    return 1;
}
//将命令cmd放入历史命令列表中
void put_cmd(struct session *sp,char *cmd,int len)
{
    sp->cmds=sp->cmds%MAX_CMD;
    if(len==0)
        return;
    if(len>LINE_SIZE)
        len=LINE_SIZE;
    memcpy(sp->cmd_back[sp->cmds],cmd,len);
    sp->cmd_back[sp->cmds][len]=0;
    sp->cmds++;
    sp->cmd_cur=sp->cmds;
    if(sp->cmd_num<MAX_CMD)
        sp->cmd_num++;
}
//从历史命令列表中获取命令
void get_cmd(struct session *sp,char **buf,int off)
{
    if(sp->cmds==0)
    {
        *buf=0;
	    sp->cmd_cur=0;
	    return;
    }
    sp->cmd_cur=(sp->cmd_cur+off+sp->cmd_num)%(sp->cmd_num);
    *buf=sp->cmd_back[sp->cmd_cur];
#ifdef _TELNET_DEBUG_
    wrtlog("From %s cmd_cur=%d\n",sp->client_addr,sp->cmd_cur);
    int i;
    for(i=0;i<sp->cmd_num;i++)
    {
        printf("cmd[%d]=%s\n",i,sp->cmd_back[i]);
    }
#endif
}
//将字符c放入命令列表中
int coutput(struct session *sp,char c)
{
    //wrtlog("From %s in coutput c=%d\n\r",sp->client_addr,c);
	if(sp->syn_num>0)//如果收到带外信号，忽略输入的字符，直到iac datamark到达
	{
	    //printf("syn is working\n");
	    return 1;
	}
	char bytes[10];
	int len=0,i;
	char *ocmd=NULL;
	char nc;
	int cmd_len;
#ifdef _CASE_INSENSITIVE
    if(c>='A'&&c<='Z')//将大写转换为小写
	    c|=0x20;
#endif
	if((c==(char)27)&&net_get(sp,bytes,2,0)==2&&bytes[0]==(char)91)//收到光标字符
	{
	    int lsize=get_linesize(sp);
	    if((bytes[1]==(char)65)||(bytes[1]==(char)66))//为上下键时，检索历史命令
	    {
	        if(sp->telnet_mode==NO_LINEMODE)
		    {
		        if((bytes[1]==(char)65))
		            get_cmd(sp,&ocmd,-1);
		        else
			        get_cmd(sp,&ocmd,1);
			    net_put_len(sp,"\r",1);
			    if(ocmd==NULL)
			        cmd_len=0;
			    else
			        cmd_len=strlen(ocmd);
			    net_put_len(sp,telnet_log,strlen(telnet_log));
			    net_put_len(sp,ocmd,cmd_len);
			    sp->line_pos=sp->linesize=(strlen(telnet_log)+cmd_len)%lsize;
			    //printf("lszie=%d line_pos=%d linesize=%d cmd_len=%d cmd_inlen=%d\n",lsize,sp->line_pos,sp->linesize,cmd_len,sp->cmd_inlen);
			    if(sp->line_pos!=0&&sp->cmd_inlen<lsize-strlen(telnet_log))
			    { 
			        nc=' ';
			        for(i=cmd_len;i<sp->cmd_inlen;i++)
			        net_put_len(sp,&nc,1);
			        nc=8;
			        for(i=cmd_len;i<sp->cmd_inlen;i++)
			            net_put_len(sp,&nc,1);
			    }
			    else if(sp->line_pos==0&&ocmd)
			    {
			        net_put(sp,crlf);
			    }
			    if(ocmd!=NULL)
			    {
			        memcpy(sp->cmd_inbuffer,ocmd,cmd_len);
			    }
			    sp->cmd_inp=sp->cmd_inbuffer;
		        sp->cmd_inlen=cmd_len;
		    }
		    net_get(sp,NULL,2,1);
		    net_flush(sp);
	        return 1;
	    }
        else if(bytes[1]==(char)68)//为左键，向左移动光标
        {
		    if(sp->telnet_mode==NO_LINEMODE)
		    {
		        int beg;			
		        if(sp->linesize<sp->cmd_inlen)
			        beg=0;
			    else 
			        beg=sp->linesize-sp->cmd_inlen;
			    //printf("line_beg=%d\n",beg);
			    //printf("pos=%d linesize=%d cmd_inlen=%d\n",sp->line_pos,sp->linesize,sp->cmd_inlen);
			    if(sp->line_pos>beg)
			    {
			        sprintf(bytes,"%c%c%c",27,91,68);
			        net_put(sp,bytes,3);
			        sp->line_pos--;
			    }
		    }
		    net_get(sp,NULL,2,1);
		    net_flush(sp);
	        return 1;
        }
        else if(bytes[1]==(char)67)	//为向右键，向右移动光标
		{
		    if(sp->linemode==NO_LINEMODE)
		    {
		        if(sp->linesize>sp->line_pos)
			    {
			        sprintf(bytes,"%c%c%c",27,91,67);
			        net_put(sp,bytes,3);
			        sp->line_pos++;
			    }
			//printf("pos=%d linesize=%d\n",sp->line_pos,sp->linesize);
		    }
		    net_get(sp,NULL,2,1);
		    net_flush(sp);
	        return 1;
		}
	}
	else if(isdel(sp,c))//为删除字符，删除光标前一个字符
	{
		if(my_state_is_will(sp,TELOPT_ECHO))
		{
		    //printf("linesize=%d linepos=%d cmd_len=%d\n",sp->linesize,sp->line_pos,sp->cmd_inlen);
			if(sp->telnet_mode==NO_LINEMODE&&sp->line_pos>sp->linesize-sp->cmd_inlen)
			{
			    sprintf(bytes,"%c%c%c",27,91,68);
			    net_put(sp,bytes,3);
			    if(sp->line_pos<sp->linesize)
				{
				    net_put_len(sp,sp->cmd_inbuffer+sp->cmd_inlen-sp->linesize+sp->line_pos,
					sp->linesize-sp->line_pos);
				}
				net_put(sp," ");
				for(i=0;i<sp->linesize-sp->line_pos+1;i++)
				    net_put_len(sp,bytes,3);
			    net_flush(sp);
			}
			else if(sp->telnet_mode==KLUDGE_LINEMODE&&sp->cmd_inlen>0)
			{
				sprintf(bytes,"%c%c%c%c%c%c%c%c%c",8,8,8,32,32,32,8,8,8);
				net_put_len(sp,bytes,9);
			    net_flush(sp);
			}
			   
		}
		if(sp->cmd_inlen>0)//&&sp->telnet_mode<REAL_LINEMODE))//||(sp->telnet_mode==REAL_LINEMODE&&!tty_isediting()))
		{
			   //sp->cmd_inlen--;
			   //sp->cmd_inbuffer[sp->cmd_inlen]=0;
			if(sp->linesize&&sp->linesize>=sp->line_pos&&my_state_is_will(sp,TELOPT_ECHO)
			    &&sp->linesize-sp->line_pos<sp->cmd_inlen)
			{
			    memmove(sp->cmd_inbuffer+sp->cmd_inlen-sp->linesize+sp->line_pos-1,
				    sp->cmd_inbuffer+sp->cmd_inlen-sp->linesize+sp->line_pos,
					sp->linesize-sp->line_pos);
				//if(sp->linesize>0)//&&sp->telnet_mode<REAL_LINEMODE)//||(sp->telnet_mode==REAL_LINEMODE&&!tty_isediting()))
		        sp->linesize--;
		         //if(sp->line_pos>0)
		        sp->line_pos--;
				sp->cmd_inlen--;
				sp->cmd_inbuffer[sp->cmd_inlen]=0;
		    }
		    else if(my_state_is_wont(sp,TELOPT_ECHO))
		    {
				sp->cmd_inlen--;
				sp->cmd_inbuffer[sp->cmd_inlen]=0;
		    }
				
		}
		//cwrtlog(sp,"del cur inlen=%d c=%d\n",sp->cmd_inlen,c);
	}
    else if(c=='\r')//为换行符，处理命令
	{
	//printf("in couput:mode=%d\n",sp->telnet_mode);
	    if(sp->telnet_mode==NO_LINEMODE)
		{
		    if(net_get(sp,&c,1,0)>0)
		    {
		        if(c==0||c=='\n')
			    {
			        net_get(sp,NULL,1,1);
				    do_cmd(sp);
				    return;
			    }  
		    }
		    else
		    {
		         do_cmd(sp);	
		    }
		}
		else if(sp->telnet_mode==KLUDGE_LINEMODE)
		{
		    if(net_get(sp,&c,1,0)>0)
		    {
		        if(c==0||c=='\n')
			    {
			        net_get(sp,NULL,1,1);
				    do_cmd(sp);
				     return;
			    }  
		    }
		    else
		    {
		        do_cmd(sp);
		    }
		}
		else
		{
		    if(net_get(sp,&c,1,0)>0)
		    {
		        if(c==0||c=='\n')
			    {
			        net_get(sp,NULL,1,1);
				    do_cmd(sp);
				    return;
			    }  
		    }
		}
	}	
	else if(c=='\n'&&sp->telnet_mode>=KLUDGE_LINEMODE)
	{
	    do_cmd(sp);
	}
	else if(c==3&&sp->telnet_mode==NO_LINEMODE)//为ctrl+c键，关闭monitor线程
	{
		set_disp(sp,0);
		net_put_len(sp,crlf,strlen(crlf));
		net_put_len(sp,telnet_log,strlen(telnet_log));
		sp->cmd_inlen=0;
		sp->cmd_inp=sp->cmd_inbuffer;
		net_flush(sp);
	}
	else if(c=='\t'&&sp->telnet_mode==NO_LINEMODE)//为table键，检索命令
	{
	    int out_len;
		char ret_buf[MAX_BUF_IN];
		sp->cmd_inbuffer[sp->cmd_inlen]=0;
		bzero(ret_buf,sizeof(ret_buf));
		//printf("in coutput cmd_inbuffer=%s\n",sp->cmd_inbuffer);
	    out_len=match_cmd_out(sp->cmd_inbuffer,ret_buf);
		//printf("ret_buf=%s out_len=%d\n",ret_buf,out_len);
		int i;
		nc=' ';
		for(i=0;i<sp->linesize-sp->line_pos;i++)
		    net_put_len(sp,&nc,1);
		nc=8;
        for(i=0;i<sp->linesize-sp->line_pos;i++)
		    net_put_len(sp,&nc,1);	
        sp->cmd_inlen=sp->cmd_inlen-sp->linesize+sp->line_pos;		   
		if(out_len>0)
		{	              
		    net_put_len(sp,"\r",1);
		    net_put_len(sp,telnet_log,strlen(telnet_log));
		    net_put_len(sp,ret_buf,strlen(ret_buf));
		    cmd_in_clear(sp);
		    cmd_input_len(sp,ret_buf,strlen(ret_buf));
			nc=' ';
			for(i=0;i<sp->linesize-out_len;i++)
			    net_put_len(sp,&nc,1);
			nc=8;
			for(i=0;i<sp->linesize-out_len;i++)
			    net_put_len(sp,&nc,1);
		    net_flush(sp);
		    sp->line_pos=sp->linesize=strlen(telnet_log)+strlen(ret_buf);
		}
	}
	else if(c>=32)//为一般命令数据，存储该字符
	{
	    int lsize=get_linesize(sp);
		//printf("lsize=%d curlinesize=%d pos=%d\n",lsize,sp->linesize,sp->line_pos);
		if(my_state_is_will(sp,TELOPT_ECHO))
		{
			net_put_len(sp,&c,1);
			if(sp->linesize>sp->line_pos)
			{
			    if(sp->linesize<lsize-1)
				{
			        net_put_len(sp,sp->cmd_inbuffer+sp->cmd_inlen-sp->linesize+sp->line_pos,
				                 sp->linesize-sp->line_pos);
					sprintf(bytes,"%c%c%c",27,91,68);
			        for(i=0;i<sp->linesize-sp->line_pos;i++)
			        {
			           net_put_len(sp,bytes,3);
			        }
				}
				else
				{
				    net_put_len(sp,sp->cmd_inbuffer+sp->cmd_inlen-sp->linesize+sp->line_pos,
					    lsize-sp->line_pos);
					net_put_len(sp,"\n\r",2);
				}
			}
			else if(sp->linesize>=lsize-1)
			{
				net_put(sp,crlf);
			}			  
			net_flush(sp);
		}
		if(my_state_is_will(sp,TELOPT_ECHO)&&sp->linesize>sp->line_pos)
		{
		   //printf("move %d bytes beg=%c\n",sp->linesize-sp->line_pos,
		   //sp->cmd_inbuffer[sp->cmd_inlen-sp->linesize+sp->line_pos]);
		    memmove(sp->cmd_inbuffer+sp->cmd_inlen-sp->linesize+sp->line_pos+1,
			    sp->cmd_inbuffer+sp->cmd_inlen-sp->linesize+sp->line_pos,sp->linesize-sp->line_pos);					 
			sp->cmd_inbuffer[sp->cmd_inlen-sp->linesize+sp->line_pos]=c;
		}
	    else
		    sp->cmd_inbuffer[sp->cmd_inlen]=c;
		sp->linesize++;
		if(sp->linesize>=lsize)
		{
		    sp->linesize=sp->line_pos=0;
		}
		else
		{
		    sp->line_pos++;
		}
		sp->cmd_inlen++;
		if(sp->cmd_inlen>=MAX_BUF_IN)//命令过长，处理命令前半部分
		{
		    sp->cmd_inbuffer[sp->cmd_inlen]=0;
		    wrtlog("From %s client input cmd \"%s\" is too long,the cmd is too bad\n",sp->client_addr,sp->cmd_inbuffer);
			do_cmd(sp);
	    }
	}
	return 1;
}
//主状态机，处理每一个字符
int dispose(struct session *sp)
{
	struct fsm *pfsm;
	int ti,len;
	char c,nc;
	int ii;
	char buf[64];
	while((len=net_get(sp,&c,1,1))>0&&!cmd_out_full(sp))
	{
		ti=trans_table[sp->master_state][c&0xff];
		pfsm=&trans[ti];
		//wrtlog("From %s c=%d current_state=%s choose %d func\n\r",sp->client_addr,c&0xff,STATE(sp->master_state),ti);
		if(sp->master_state==SSUBNEG&&c==(char)IAC&&net_get(sp,&nc,1,0)>0&&nc==(char)IAC)
		{
		    subopt(sp,c);
			net_get(sp,NULL,1,1);
			sp->master_state=SSUBNEG;
			continue;
		}
		if(pfsm->action)
		    (pfsm->action)(sp,c);
		sp->master_state=pfsm->next_state;
		// printf("sp=%d sp->client_addr=%d\n",sp,sp->client_addr);
		//wrtlog("From %s state=%s\n\r",sp->client_addr,STATE(sp->master_state));
	}
	return 1;
}
//记录输入命令
int recopt(struct session *sp,char c)
{
    cwrtlog(sp,"cmd=%s opt=%s\r\n",TELCMD(c&0xff),sp->net_inp<sp->net_inbuffer+sp->net_inlen?TELOPT(*(sp->net_inp)):"NULL");
    sp->cmd=c;
    return 1;
}
//发送do option命令
/*
  当init大于0时，检查当前状态，否则不检查状态直接发送命令
*/
void send_do (struct session *sp,int option, int init)
{
   //cwrtlog(sp,"in send_do cmd=%s\r\n",TELOPT(option));
    wrtlog("From %s: in send_do cmd=%s\r\n",sp->client_addr,TELOPT(option));
    if (init)
    {
        if ((sp->do_dont_req_num[option] == 0 && his_state_is_will (sp,option)) || his_want_state_is_will (sp,option))
	        return;
      /*
       * Special case for TELOPT_TM:  We send a DO, but pretend
       * that we sent a DONT, so that we can send more DOs if
       * we want to.
       */
        if (option == TELOPT_TM)
	        set_his_want_state_wont (sp,option);
        else
	        set_his_want_state_will (sp,option);
        sp->do_dont_req_num[option]++;
    }
    net_put(sp,doopt, option);
}
//发送dont option命令
void send_dont (struct session *sp,int option, int init)
{
    // wrtlog("From %s in send dont cmd=%s\n\r",sp->client_addr,TELOPT(option));
    if (init)
    {
        if ((sp->do_dont_req_num[option] == 0 && his_state_is_wont (sp,option)) ||his_want_state_is_wont (sp,option))
	        return;
        set_his_want_state_wont (sp,option);
        sp->do_dont_req_num[option]++;
    }
    net_put(sp,dont, option);
}
//发送will option命令
void send_will (struct session *sp,int option, int init)
{
  //wrtlog("From %s in send will cmd=%s\n\r",sp->client_addr,TELOPT(option));
    if (init)
    {
        if ((sp->will_wont_req_num[option] == 0 && my_state_is_will (sp,option)) ||my_want_state_is_will (sp,option))
	        return;
        set_my_want_state_will (sp,option);
        sp->will_wont_req_num[option]++;
    }
    net_put (sp,will, option);
}
//发送wont option命令
void send_wont (struct session *sp,int option, int init)
{
  //wrtlog("in send_wont cmd=%s\r\n",sp->client_addr,TELOPT(option));
    if (init)
    {
        if ((sp->will_wont_req_num[option] == 0 && my_state_is_wont (sp,option)) ||my_want_state_is_wont (sp,option))
	        return;
        set_my_want_state_wont (sp,option);
        sp->will_wont_req_num[option]++;
   }
   net_put(sp,wont, option);
}
#define CMD_OVER() cmd_in_clear(sp);\
                   cmd_output(sp,telnet_log,strlen(telnet_log));\
				   sp->line_pos=sp->linesize=strlen(telnet_log);\
				   cmd_out_flush(sp);\
				   net_flush(sp)
/*
   处理命令
*/
int do_cmd(struct session *sp)
{
    char *beg,*end;
    int out_len=0;
    char ret_buf[MAX_BUF_IN];
    char buf[64];
    //char *cmd_b,*cmd_e,*cur,*pre;
	sp->cmd_inbuffer[sp->cmd_inlen]=0;
    wrtlog("From %s in do_cmd cmd=%s\r\n",sp->client_addr,sp->cmd_inbuffer);
    if(sp->telnet_mode>=KLUDGE_LINEMODE)//先输出换行符
    {
	    cmd_output(sp,"\r",1);
    }
    else
    {
        cmd_output(sp,"\r\n",2);
    }
    if(sp->cmd_inlen==0)//输入长度为0，直接退出
    {
        CMD_OVER();
        return 0;
    }
    if(sp->monitor_thread)//if a monitor thread is running,don't execute any command
    {
        sp->linesize=sp->line_pos=sp->cmd_inlen=0;
	    sp->cmd_inp=sp->cmd_inbuffer;
	     return 0;
    }
    fre_strip(sp->cmd_inbuffer,sp->cmd_inlen,&beg);//删除前后空格
    back_strip(sp->cmd_inbuffer,sp->cmd_inlen,&end);
    if(end-beg<=0)//if command is just apace charactors, return
    {
        CMD_OVER();
        return 1;
    }
    put_cmd(sp,beg,end-beg);//put this command in history list
  /*cur=beg;
  cmd_out_flush(sp);
  while((pre<end)&&((cur=memchr(pre,';',end-pre))!=NULL))
  {
    //printf("cur-pre=%d\n",cur-pre);
         if(pre>=cur)
		{
		   pre=++cur;
		   continue;
		}
         fre_strip(pre,cur-pre,&cmd_b);
		 back_strip(pre,cur-pre,&cmd_e);
		 if(cmd_b>=cmd_e)
		 {
		    pre=++cur;
		   continue;
		 }
		if(strlen(telnet_quit)==cmd_e-cmd_b&&strncmp(cmd_b,telnet_quit,cmd_e-cmd_b)==0)
		{
		    wrtlog("From %s going to logout\n",sp->client_addr);
			logout(sp);			
		}
		bzero(ret_buf,sizeof(ret_buf));
		*cmd_e=0;
		wrtlog("input=%s len=%d\n\r",sp->client_addr,cmd_b,cmd_e-cmd_b);
		out_len=telnet_cmd_process(cmd_b,cmd_e-cmd_b,ret_buf,MAX_BUF_IN);
		cmd_out_flush(sp);
		net_put(sp,telnet_log);
		sp->linesize=sp->line_pos=strlen(telnet_log);
		net_flush(sp);
		if(out_len>MAX_BUF_IN)
        {
            wrtlog("From %s: %d bytes output from telnet_cmd_process is more than %d,\
			the recieve buffer is not big enough,the remainming will\
	                not being send to client\r\n",sp->client_addr,out_len,MAX_BUF_IN);
	        out_len=MAX_BUF_IN;
        }
		if(out_len>0)
		{
		   wrtlog("From %s output=%s len=%d\r\n",sp->client_addr,sp->cmd_outend,out_len);
		   cmd_output(sp,ret_buf,out_len);
		   cmd_in_clear(sp);
		   cmd_input_len(sp,ret_buf,out_len);
		   sp->linesize=sp->line_pos=sp->linesize+out_len;
		   //sp->cmd_outend+=out_len;
		   cmd_out_flush(sp);
		}
		pre=++cur;
  }
  if(pre>=end)
  {
       //sp->cmd_inlen=0;
	   CMD_OVER();
	   return 1;
  }
  fre_strip(pre,end-pre,&cmd_b);
  back_strip(pre,end-pre,&cmd_e);
  if(cmd_b>=cmd_e)
  {
     //sp->cmd_inlen=0;
	 CMD_OVER();
    return 1;
  }*/
    bzero(ret_buf,sizeof(ret_buf));
    if(strlen(telnet_quit)==end-beg&&strncmp(beg,telnet_quit,end-beg)==0)//logout
    {
        wrtlog("From %s going to logout\n",sp->client_addr);
	    return logout(sp);
    }
	if(end-beg==5&&strncmp(beg,"clear",5)==0)
	{
	    if(strncmp("VT100",sp->term,5)==0||strlen(sp->term)==0||strncmp(sp->term,"ANSI",4)==0)
        {
		    char buf[]={0x1b,0x5b,0x32,'J',0x1b,0x5b,'1',';','1','H',0};
		    cmd_output(sp,buf,strlen(buf));
	    }
	}
    else
	{
    //*cmd_e=0;
         *end=0;
    //wrtlog("From %s input=%s len=%d\n\r",sp->client_addr,beg,end-beg);
    //cwrtlog(sp,"*cmd_b=%c  *(cmd_e-1)=%c\n",*cmd_b,*(cmd_e-1));
        out_len=telnet_cmd_process(beg,end-beg,ret_buf,MAX_BUF_IN);//process the command
	}
    cmd_output(sp,telnet_log,strlen(telnet_log));//put the prompt
    sp->linesize=sp->line_pos=strlen(telnet_log);
    if(out_len>MAX_BUF_IN)//if output is too long
    {
        wrtlog("From %s %d bytes output from telnet_cmd_process is more than %d,\
	      the recieve buffer is not big enough,the remainming will\
	      not being send to client\r\n",sp->client_addr,out_len,MAX_BUF_IN);
	    out_len=MAX_BUF_IN;
    }
    //sp->cmd_outend[out_len]=0;
    //wrtlog("From %s output=%s len=%d origlen=%d\r\n",sp->client_addr,sp->cmd_outend,out_len,sp->cmd_outend-sp->cmd_outbuffer);
    //sp->cmd_outend+=out_len;
    //if(monitor_thread)
    //   usleep(1000);
    if(out_len>0)
    {
        int lsize=get_linesize(sp);
        if(out_len+sp->linesize<lsize)
	    {
            cmd_output(sp,ret_buf,out_len);
	        sp->linesize=sp->line_pos=sp->linesize+out_len;
	    }
	    else
	    {
	        cmd_output(sp,ret_buf,lsize-sp->linesize);
		    cmd_output(sp,crlf,strlen(crlf));
		    cmd_output(sp,ret_buf+lsize,out_len-lsize+sp->linesize);
		    sp->linesize=sp->line_pos=out_len-lsize+sp->linesize;
	    }	 
    }
    //printf("From %s in do_cmd   out_len=%d out_buf=%s\n\n",sp->client_addr,out_len,ret_buf);
    cmd_out_flush(sp);//刷新命令输出缓冲
    //sp->cmd_inlen=out_len;
    cmd_in_clear(sp);//清除命令输入缓冲
    cmd_input_len(sp,ret_buf,out_len);//将输出命令置入输入命令buffer
    //sp->cmd_inp=sp->cmd_inbuffer;
    //cmd_out_flush();
    net_flush(sp);//刷新网络缓冲
    //sp->cmd_inlen=0;
    //sp->cmd_inp=sp->cmd_inbuffer;
    return 1;
}
//处理will wont echo命令
int w_echo(struct session *sp,char c)
{
   //wrtlog("From %s in w_echo\n",sp->client_addr);
    if (sp->do_dont_req_num[c&0xff])
    {
        sp->do_dont_req_num[c&0xff]--;
        if (sp->do_dont_req_num[c&0xff]&&((sp->cmd==WILL&&his_state_is_will(sp,c))||(sp->cmd==WONT&&his_state_is_wont(sp,c))))
	        sp->do_dont_req_num[c&0xff]--;
    }
    if(sp->do_dont_req_num[c&0xff]==0)
    {
        if(sp->cmd==WILL)
	    {
	        if(his_want_state_is_wont(sp,c))
	        {
		        sp->do_dont_req_num[c&0xff]++;
			    send_dont(sp,c,0);
		    }
		    else
		    {
		        send_dont(sp,c,1);
		    }
		    set_his_state_will(sp,c);
	    }
	    else
	    {
	        if(his_want_state_is_will(sp,c))
		    {
		        set_his_want_state_wont (sp,c);
	            if (his_state_is_will (sp,c))
	                send_dont (sp,c, 0);
		    }
		    set_his_state_wont(sp,c);
	   }
    }
    return his_state_is_will(sp,c);
}
//处理will wont timemark命令
int w_timemark(struct session *sp,char c)
{
   //wrtlog("From %s in timemark\r\n",sp->client_addr);
   if (sp->do_dont_req_num[c&0xff])
   {
      sp->do_dont_req_num[c&0xff]--;
      if (sp->do_dont_req_num[c&0xff]&&((sp->cmd==WILL&&his_state_is_will(sp,c))||(sp->cmd==WONT&&his_state_is_wont(sp,c))))
	       sp->do_dont_req_num[c&0xff]--;
   }
   if(sp->do_dont_req_num[TELOPT_TM]==0)
   {
        if(sp->cmd==WILL)
	    {
	        if(his_want_state_is_wont(sp,c))
		    {
                if(sp->telnet_mode==NO_LINEMODE)
	            {
		            if(sp->max_telnet_mode>=KLUDGE_LINEMODE)
			        {
		                sp->telnet_mode=KLUDGE_LINEMODE;
		                client_state(sp,TELOPT_LINEMODE,WILL);
			            send_wont(sp,TELOPT_SGA,1);
				        set_his_want_state_will(sp,TELOPT_ECHO);
				        send_wont(sp,TELOPT_ECHO,0);
				        sp->will_wont_req_num[TELOPT_ECHO]++;
			        }
	            }
		    }
	    }
	    else
	    {
	        if(his_state_is_will(sp,c))
	            set_his_state_wont(sp,c);
		    else
		    {
		        sp->telnet_mode=NO_LINEMODE;
			    client_state(sp,TELOPT_LINEMODE,WONT);
			    send_will (sp,TELOPT_SGA, 1);
		        send_will (sp,TELOPT_ECHO, 1);
			    set_his_state_wont (sp,c);
		    }
	    }
    }
}
//处理will wont linemode命令
int w_linemode(struct session *sp,char c)
{
   //wrtlog("From %s in w_linemode\r\n",sp->client_addr);
    if (sp->do_dont_req_num[c&0xff])
    {
        sp->do_dont_req_num[c&0xff]--;
        if (sp->do_dont_req_num[c&0xff]&&((sp->cmd==WILL&&his_state_is_will(sp,c))||(sp->cmd==WONT&&his_state_is_wont(sp,c))))
	        sp->do_dont_req_num[c&0xff]--;
    }
    if(sp->do_dont_req_num[c&0xff]==0)
    {
        if(sp->cmd==WILL)
	    {
	        if(sp->max_telnet_mode>=REAL_LINEMODE)
		    {
	            if(his_want_state_is_wont(sp,c))
		        {
		            sp->telnet_mode=REAL_LINEMODE;
		            client_state(sp,TELOPT_LINEMODE,WILL);
		            set_his_want_state_will(sp,c);
		            send_do(sp,c,0);
		        }
		        else
		        {
		            sp->telnet_mode=REAL_LINEMODE;
		            client_state(sp,TELOPT_LINEMODE,WILL);
		        }	 
		    }
		    else
		    {
		        send_dont(sp,TELOPT_LINEMODE,0);
		        sp->do_dont_req_num[c&0xff]++;
		       //set_his_state_wont(TELOPT_LINEMODE);
		    }
		    set_his_state_will(sp,c);
	    }
	    else
	    {
	        if(his_want_state_is_will(sp,c))
		    {
		        if(sp->telnet_mode==REAL_LINEMODE)
			    {
			        sp->telnet_mode=NO_LINEMODE;
			        client_state(sp,TELOPT_LINEMODE,WONT);
			    }
			    set_his_want_state_wont (sp,c);
	            if (his_state_is_will (sp,c))
	                send_dont (sp,c, 0);
		    }
		    set_his_state_wont (sp,c);
	    }
    }
    return 1;
}
//处理will wont binary命令
int w_binary(struct session *sp,char c)
{
   //wrtlog("From %s in w_binary\r\n",sp->client_addr);
    if (sp->do_dont_req_num[c&0xff])
    {
        sp->do_dont_req_num[c&0xff]--;
        if (sp->do_dont_req_num[c&0xff]&&((sp->cmd==WILL&&his_state_is_will(sp,c))||(sp->cmd==WONT&&his_state_is_wont(sp,c))))
	        sp->do_dont_req_num[c&0xff]--;
    }
    if(sp->do_dont_req_num[c&0xff]==0)
    {
        if(sp->cmd==WILL)
	    {
	        if(his_want_state_is_wont(sp,c))
		    {
			    tty_binaryin(sp,1);
			    set_his_want_state_will(sp,c);
			    send_do(sp,c,0);
		    }
		    set_his_state_will(sp,c);
	    }
	    else
	    {
	        if(his_want_state_is_will(sp,c))
		    {
			    tty_binaryin(sp,0);
			    set_his_want_state_wont (sp,c);
	            if (his_state_is_will (sp,c))
	                send_dont (sp,c, 0);
		    }
		    set_his_state_wont(sp,c);
	    }
    }
}
//处理will wont naws协商
int w_naws(struct session *sp,char c)
{
   //wrtlog("From %s in w_naws\r\n",sp->client_addr);
    if (sp->do_dont_req_num[c&0xff])
    {
        sp->do_dont_req_num[c&0xff]--;
        if (sp->do_dont_req_num[c&0xff]&&((sp->cmd==WILL&&his_state_is_will(sp,c))||(sp->cmd==WONT&&his_state_is_wont(sp,c))))
	        sp->do_dont_req_num[c&0xff]--;
   }
   if(sp->do_dont_req_num[c&0xff]==0)
   {
       if(sp->cmd==WILL)
	   {
	       if(his_want_state_is_wont(sp,c))
	        {
			    send_dont(sp,c,0);
			    set_his_state_will(sp,c);
		    }
		    set_his_state_will(sp,c);
	    }
	    else
	    {
	        if(his_want_state_is_will(sp,c))
		    {
		        set_his_want_state_wont (sp,c);
	            if (his_state_is_will (sp,c))
	                send_dont (sp,c, 0);
		    }
		     set_his_state_wont(sp,c);
	    }
	}
}
//处理will wont logout选项
int w_exit(struct session *sp,char c)
{
  // wrtlog("From %s in w_exit\r\n",sp->client_addr);
   if (sp->do_dont_req_num[c&0xff])
   {
        sp->do_dont_req_num[c&0xff]--;
        if (sp->do_dont_req_num[c&0xff]&&((sp->cmd==WILL&&his_state_is_will(sp,c))||(sp->cmd==WONT&&his_state_is_wont(sp,c))))
	        sp->do_dont_req_num[c&0xff]--;
   }
   if(sp->do_dont_req_num[c&0xff]==0)
   {
        if(sp->cmd==WILL)
        {
            if(his_want_state_is_wont(sp,c))
	        {
	            set_his_want_state_will(sp,c);
		        send_do(sp,c,0);
			    sp->logoff=1;
			    //clean(sp);
			    return 1;
	        }
		    else
		    {
		   //clean(sp);
		        sp->logoff=1;
		        return 1;
		    }
        }
	    else
	    {
	        if(his_want_state_is_will(sp,c))
		    {
		        set_his_want_state_wont(sp,c);
		        if(his_state_is_will(sp,c))
		            send_dont(sp,c,0);
		    }
		    set_his_state_wont(sp,c);
	    }
	}
	return 1;
}
//处理will wont termtype选项
int w_termtype(struct session *sp,char c)
{
  //wrtlog("From %s in w_termtype\r\n",sp->client_addr);
    if (sp->do_dont_req_num[c&0xff])
    {
        sp->do_dont_req_num[c&0xff]--;
        if (sp->do_dont_req_num[c&0xff]&&((sp->cmd==WILL&&his_state_is_will(sp,c))||(sp->cmd==WONT&&his_state_is_wont(sp,c))))
	        sp->do_dont_req_num[c&0xff]--;
    }
    if(sp->do_dont_req_num[c&0xff]==0)
    {
        if(sp->cmd==WILL)
        {
            if(his_want_state_is_wont(sp,c))
	        {
	            set_his_want_state_will(sp,c);
		        send_do(sp,c,0);			    
	        }
			net_put(sp,"%c%c%c%c%c%c",IAC,SB,TELOPT_TTYPE,TELQUAL_SEND,IAC,SE);
		    set_his_state_will(sp,c);
        }
	    else
	    {
	        if(his_want_state_is_will(sp,c))
		    {
		        set_his_want_state_wont(sp,c);
		        if(his_state_is_will(sp,c))
		            send_dont(sp,c,0);
		    }
		    set_his_state_wont(sp,c);
	    }
	}
	return 1;
}
//不支持该选项
int w_notsupport(struct session *sp,char c)
{
    //wrtlog("From %s in w_not_support\r\n",sp->client_addr);
    send_dont(sp,c,0);
    return 1;
}
//对linemode选项进行处理
int d_linemode(struct session *sp,char c)
{
    //wrtlog("From %s in d_linemode\r\n",sp->client_addr);
    if (sp->will_wont_req_num[c&0xff])
    {
        sp->will_wont_req_num[c&0xff]--;
        if (sp->will_wont_req_num[c&0xff] && (my_state_is_will (sp,c)&&sp->cmd==DO||my_state_is_wont(sp,c)&&sp->cmd==DONT))
	        sp->will_wont_req_num[c&0xff]--;
    }
	if(sp->cmd==DO)
	{
	    if ((sp->will_wont_req_num[c&0xff] == 0) && (my_want_state_is_wont (sp,c)))
	    {
	        sp->will_wont_req_num[c&0xff]++;
		    send_wont(sp,c,0);
	    }
	    set_my_state_will(sp,c);
	}
	else
	{
	    if((sp->will_wont_req_num[c&0xff] == 0) && (my_want_state_is_will (sp,c)))
	    {
	        set_my_want_state_wont (sp,c);
            if (my_state_is_will (sp,c))
	        send_wont (sp,c, 0);
	    }
	    set_my_state_wont (sp,c);
	}
	return 1;
}
//对do dont sga选项处理
int d_sga(struct session *sp,char c)
{
   //wrtlog("From %s in d_sga\r\n",sp->client_addr);
    if (sp->will_wont_req_num[c&0xff])
    {
        sp->will_wont_req_num[c&0xff]--;
        if (sp->will_wont_req_num[c&0xff] && (my_state_is_will (sp,c)&&sp->cmd==DO||my_state_is_wont(sp,c)&&sp->cmd==DONT))
	        sp->will_wont_req_num[c&0xff]--;
    }
	if(sp->will_wont_req_num[c&0xff]==0)
	{
	    if(sp->cmd==DO)
	    {
	        if ((sp->will_wont_req_num[c&0xff] == 0) && (my_want_state_is_wont (sp,c)))
		    {
	            if(sp->telnet_mode>=KLUDGE_LINEMODE)
		        {
		            sp->telnet_mode=NO_LINEMODE;
		            client_state(sp,TELOPT_LINEMODE,WONT);
			 //if(sp->telnet_mode==NO_LINEMODE)
			 //{
			        set_my_state_will(sp,c);
				    send_will(sp,c,0);
				    set_my_want_state_will(sp,c);
				    return 1;
			// }
		        }
		        send_wont(sp,c,0);
		        sp->will_wont_req_num[c&0xff]++;
		    }
            set_my_state_will(sp,c);		
	    }
	    else
	    {
	        if((sp->will_wont_req_num[c&0xff] == 0) && (my_want_state_is_will(sp,c)))
		    {
		        if(sp->telnet_mode==KLUDGE_LINEMODE)
			    {
			        sp->telnet_mode=REAL_LINEMODE;
			        client_state(sp,TELOPT_LINEMODE,WILL);
			    }
			    set_my_want_state_wont (sp,c);
                if (my_state_is_will (sp,c))
	                send_wont (sp,c, 0);
		    }
		    set_my_state_wont(sp,c);
	    }
	}
	return 1;
}
//对do dont echo选项进行处理
int d_echo(struct session *sp,char c)
{
   //wrtlog("From %s in d_echo\r\n",sp->client_addr);
    if (sp->will_wont_req_num[c&0xff])
    {
        sp->will_wont_req_num[c&0xff]--;
        if (sp->will_wont_req_num[c&0xff] && (my_state_is_will (sp,c)&&sp->cmd==DO||my_state_is_wont(sp,c)&&sp->cmd==DONT))
	         sp->will_wont_req_num[c&0xff]--;
    }
	if(sp->cmd==DO)
	{
	    if ((sp->will_wont_req_num[c&0xff] == 0) && (my_want_state_is_wont (sp,c)))
	    {
	      //if(sp->telnet_mode==NO_LINEMODE)
	      //{
		    tty_setecho(sp,1);
	      //}
	        set_my_want_state_will(sp,c);
	        send_will(sp,c,0);
	    }
	    set_my_state_will(sp,c);
	}
	else
	{
	    if(sp->will_wont_req_num[c&0xff]==0&&my_want_state_is_will(sp,c))
	    {
	      //if(sp->telnet_mode==NO_LINEMODE)
		  //{
			tty_setecho(sp,0);
		  //}
		    set_my_want_state_wont (sp,c);
            if (my_state_is_will (sp,c))
	            send_wont (sp,c, 0);
        }
        set_my_state_wont (sp,c);
		if(sp->telnet_mode==NO_LINEMODE)
		{
		   //send_will(TELOPT_ECHO,1);
		    cwrtlog(sp,"shoud not come here\n");
		    set_my_state_will(sp,TELOPT_ECHO);
		}
	}
}
//对do dont binary选项进行处理
int d_binary(struct session *sp,char c)
{
  //wrtlog("From %s in d_binary\n\r",sp->client_addr);
   if (sp->will_wont_req_num[c&0xff])
    {
        sp->will_wont_req_num[c&0xff]--;
        if (sp->will_wont_req_num[c&0xff] && (my_state_is_will (sp,c)&&sp->cmd==DO||my_state_is_wont(sp,c)&&sp->cmd==DONT))
	        sp->will_wont_req_num[c&0xff]--;
    }
	if(sp->cmd==DO)
	{
	    if((sp->will_wont_req_num[c&0xff] == 0) && (my_want_state_is_wont (sp,c)))
	    {
		    tty_binaryout(sp,1);
		    set_my_want_state_will (sp,c);
	        send_will (sp,c, 0);
	   }
	   set_my_state_will(sp,c);
	}
	else
	{
	    if((sp->will_wont_req_num[c&0xff] == 0) && (my_want_state_is_will (sp,c)))
	    {
		    tty_binaryout(sp,0);
		    set_my_want_state_wont(sp,c);
		    if (my_state_is_will (sp,c))
	            send_wont (sp,c, 0);
	    }
	    set_my_state_wont(sp,c);
	}
	return 1;
}
//对do dont logout进行处理
int d_exit(struct session *sp,char c)
{
    //wrtlog("From %s in d_exit\r\n",sp->client_addr);
    if (sp->will_wont_req_num[c&0xff])
    {
        sp->will_wont_req_num[c&0xff]--;
        if (sp->will_wont_req_num[c&0xff] && (my_state_is_will (sp,c)&&sp->cmd==DO||my_state_is_wont(sp,c)&&sp->cmd==DONT))
	        sp->will_wont_req_num[c&0xff]--;
    }
	if(sp->cmd==DO)
	{
	    if((sp->will_wont_req_num[c&0xff] == 0) && (my_want_state_is_wont (sp,c)))
	    {
	        set_my_want_state_will (sp,TELOPT_LOGOUT);
	        send_will (sp,TELOPT_LOGOUT, 0);
	        set_my_state_will (sp,TELOPT_LOGOUT);
	    }
	    net_flush (sp);
	    sp->logoff=1;
	   //clean(sp);
	}
	else
	{
	    if((sp->will_wont_req_num[c&0xff] == 0) && (my_want_state_is_will (sp,c)))
	    {
	        set_my_want_state_wont (sp,c);
            if (my_state_is_will (sp,c))
	            send_wont (sp,c, 0);
        }
        set_my_state_wont (sp,c);
	}
}
//对do dont timemark进行处理
int d_timemark(struct session *sp,char c)
{
   // wrtlog("From %s in d_timemark\r\n",sp->client_addr);
    if (sp->will_wont_req_num[c&0xff])
    {
        sp->will_wont_req_num[c&0xff]--;
        if (sp->will_wont_req_num[c&0xff] && (my_state_is_will (sp,c)&&sp->cmd==DO||my_state_is_wont(sp,c)&&sp->cmd==DONT))
	        sp->will_wont_req_num[c&0xff]--;
    }
	if(sp->cmd==DO)
	{
	    if((sp->will_wont_req_num[c&0xff] == 0) && (my_want_state_is_wont (sp,c)))
	    {
            send_will (sp,c, 0);
	        set_my_want_state_wont (sp,c);
	        set_my_state_wont (sp,c);
		    return 1;
	    }
	    set_my_state_wont (sp,c);
	}
	else
	{
	    if((sp->will_wont_req_num[c&0xff] == 0) && (my_want_state_is_will (sp,c)))
	    {
	        set_my_want_state_wont (sp,c);
            if (my_state_is_will (sp,c))
	            send_wont (sp,c, 0);
        }
        set_my_state_wont (sp,c);
	}
	return 1;
}
//不支持的do dont选项
int d_notsupport(struct session *sp,char c)
{
   //wrtlog("From %s in d_not_not_support\r\n",sp->client_addr);
    send_wont(sp,c,0);
    return 1;
}
//处理行模式
int client_state(struct session *sp,unsigned char code,char parm)
{
  // wrtlog("From %s in client_state\n\r",sp->client_addr);
    net_flush (sp);
  /*
   * Process request from client. code tells what it is.
   */
    switch (code)
   {
        case TELOPT_LINEMODE:
      /*
       * Don't do anything unless client is asking us to change
       * modes.
       */
            sp->uselinemode = (parm == (char)WILL);
	  /*
	   * Quit now if we can't do it.
	   */
	   //printf("in client_state:linemode=%d uselinemode=%d\n",sp->linemode,sp->uselinemode);
	        if (sp->uselinemode ==sp->linemode)
	            return;

	  /*
	   * If using real linemode and linemode is being
	   * turned on, send along the edit mode mask.
	   */
	        if (sp->telnet_mode == REAL_LINEMODE && sp->uselinemode)
	        {
		        sp->useeditmode = 0;
		        if (tty_isediting (sp))
		            sp->useeditmode |= MODE_EDIT;
		        if (tty_istrapsig (sp))
		            sp->useeditmode |= MODE_TRAPSIG;
		        if (tty_issofttab (sp))
		            sp->useeditmode |= MODE_SOFT_TAB;
		        if (tty_islitecho (sp))
		            sp->useeditmode |= MODE_LIT_ECHO;
		        net_put(sp,"%c%c%c%c%c%c%c", IAC,SB, TELOPT_LINEMODE, LM_MODE,sp->useeditmode, IAC, SE);
		        sp->editmode = sp->useeditmode;
	        }
	        tty_setlinemode (sp,sp->uselinemode);
	        sp->linemode =sp->uselinemode;
	        if (!sp->linemode)
	        {
	            send_will (sp,TELOPT_ECHO, 1);
		        sp->telnet_mode=NO_LINEMODE;
	        }
	        else
	        {
	            send_wont(sp,TELOPT_ECHO,1);
		//printf("in client_state: send wont echo\n");
		        tty_setecho(sp,0);
	        }
            break;
        case LM_MODE:
        {
	        int ack, changed;
	/*
	 * Client has sent along a mode mask.  If it agrees with
	 * what we are currently doing, ignore it; if not, it could
	 * be viewed as a request to change.  Note that the server
	 * will change to the modes in an ack if it is different from
	 * what we currently have, but we will not ack the ack.
	 */
	        sp->useeditmode &= MODE_MASK;
	        ack = (sp->useeditmode & MODE_ACK);
	        sp->useeditmode &= ~MODE_ACK;
	        if (changed = (sp->useeditmode ^sp->editmode))
	        {
	    /*
	     * This check is for a timing problem.  If the
	     * state of the tty has changed (due to the user
	     * application) we need to process that info
	     * before we write in the state contained in the
	     * ack!!!  This gets out the new MODE request,
	     * and when the ack to that command comes back
	     * we'll set it and be in the right mode.
	     */
		 //printf("the mode is different\n");
	            if (ack)
	                localstat (sp);
	            if (changed & MODE_EDIT)
	                tty_setedit (sp,sp->useeditmode & MODE_EDIT);
	            if (changed & MODE_TRAPSIG)
	                tty_setsig (sp,sp->useeditmode & MODE_TRAPSIG);
	            if (changed & MODE_SOFT_TAB)
	                tty_setsofttab (sp,sp->useeditmode & MODE_SOFT_TAB);
	            if (changed & MODE_LIT_ECHO)
	                tty_setlitecho (sp,sp->useeditmode & MODE_LIT_ECHO);
	            if (!ack)
	            {
		            net_put(sp,"%c%c%c%c%c%c%c", IAC,SB, TELOPT_LINEMODE, LM_MODE,sp->useeditmode | MODE_ACK, IAC, SE);
	            }
	            sp->editmode =sp->useeditmode;
	        }
	        break;
        }				/* end of case LM_MODE */
        default:
     /* What? */
            break;
    }				/* end of switch */
    net_flush (sp);
}
//根据服务器当前状态进行协商
void localstat (struct session *sp)
{
    net_flush (sp);
  //int need_will_echo = 0;
  /*
   * Check for state of BINARY options.
   */
    if (tty_isbinaryin (sp))
    {
        if (his_want_state_is_wont (sp,TELOPT_BINARY))
	        send_do (sp,TELOPT_BINARY, 1);
    }
    else
    {
        if (his_want_state_is_will (sp,TELOPT_BINARY))
	        send_dont (sp,TELOPT_BINARY, 1);
    }
    if (tty_isbinaryout (sp))
    {
        if (my_want_state_is_wont (sp,TELOPT_BINARY))
	        send_will (sp,TELOPT_BINARY, 1);
    }
    else
    {
        if (my_want_state_is_will (sp,TELOPT_BINARY))
	        send_wont (sp,TELOPT_BINARY, 1);
    }
  /*
   * Check linemode on/off state
   */
    sp->uselinemode = tty_linemode (sp);
  /*
   * If alwayslinemode is on, and pty is changing to turn it off, then
   * force linemode back on.
   */
    if (sp->alwayslinemode && sp->linemode && !sp->uselinemode)
    {
        sp->uselinemode = 1;
        tty_setlinemode (sp,sp->uselinemode);
    }

  /*
   * Do echo mode handling as soon as we know what the
   * linemode is going to be.
   * If the pty has echo turned off, then tell the client that
   * the server will echo.  If echo is on, then the server
   * will echo if in character mode, but in linemode the
   * client should do local echoing.  The state machine will
   * not send anything if it is unnecessary, so don't worry
   * about that here.
   *
   * If we need to send the WILL ECHO (because echo is off),
   * then delay that until after we have changed the MODE.
   * This way, when the user is turning off both editing
   * and echo, the client will get editing turned off first.
   * This keeps the client from going into encryption mode
   * and then right back out if it is doing auto-encryption
   * when passwords are being typed.
   */
   /*
  if (sp->uselinemode)
  {
  printf("tty_isecho=%d\n",tty_isecho());
      if (tty_isecho ())
	  {
	      send_wont (TELOPT_ECHO, 1);
		  tty_setecho(0);
	  }
      else
	      need_will_echo = 1;
  }*/
  /*
   * If linemode is being turned off, send appropriate
   * command and then we're all done.
   */
    if (!sp->uselinemode && sp->linemode)
    {
        if (sp->telnet_mode == REAL_LINEMODE)
	    {
	        send_dont (sp,TELOPT_LINEMODE, 1);
	    }
        else if (sp->telnet_mode == KLUDGE_LINEMODE)
	        send_will (sp,TELOPT_SGA, 1);
        send_will (sp,TELOPT_ECHO, 1);
        sp->linemode =sp->uselinemode;
	    sp->telnet_mode=NO_LINEMODE;
        goto done;
    }

  /*
   * If using real linemode check edit modes for possible later use.
   * If we are in kludge linemode, do the SGA negotiation.
   */
    if (sp->telnet_mode == REAL_LINEMODE)
    {
        sp->useeditmode = 0;
        if (tty_isediting (sp))
	        sp->useeditmode |= MODE_EDIT;
        if (tty_istrapsig (sp))
	        sp->useeditmode |= MODE_TRAPSIG;
        if (tty_issofttab (sp))
	        sp->useeditmode |= MODE_SOFT_TAB;
        if (tty_islitecho (sp))
	        sp->useeditmode |= MODE_LIT_ECHO;
    }
    else if (sp->telnet_mode == KLUDGE_LINEMODE)
    {
        if (tty_isediting (sp) &&sp->uselinemode)
	        send_wont (sp,TELOPT_SGA, 1);
        else
	        send_will (sp,TELOPT_SGA, 1);
    }

  /*
   * Negotiate linemode on if pty state has changed to turn it on.
   * Send appropriate command and send along edit mode, then all done.
   */
    if (sp->uselinemode && !sp->linemode)
    {
        if (sp->telnet_mode == KLUDGE_LINEMODE)
	    {
	        send_wont(sp,TELOPT_SGA, 1);
	    }
        else if (sp->telnet_mode == REAL_LINEMODE)
	    {
	        send_do (sp,TELOPT_LINEMODE, 1);
	  /* send along edit modes */
	        net_put (sp,"%c%c%c%c%c%c%c", IAC, SB,TELOPT_LINEMODE, LM_MODE, sp->useeditmode, IAC, SE);
	        sp->editmode = sp->useeditmode;
	    }
        sp->linemode =sp->uselinemode;
        goto done;
    }

  /*
   * None of what follows is of any value if not using
   * real linemode.
   */
    if (sp->telnet_mode < REAL_LINEMODE)
        goto done;
    if (sp->linemode && his_state_is_will (sp,TELOPT_LINEMODE))
    {
      /*
       * If edit mode changed, send edit mode.
       */
        if (sp->useeditmode !=sp->editmode)
	    {
	  /*
	   * Send along appropriate edit mode mask.
	   */
	        net_put(sp,"%c%c%c%c%c%c%c", IAC, SB,TELOPT_LINEMODE, LM_MODE, sp->useeditmode, IAC, SE);
	        sp->editmode =sp->useeditmode;
	    }
      /*
       * Check for changes to special characters in use.
       */
	    if(sp->_terminit)
	    {
            start_slc (sp);
            check_slc (sp);
            end_slc (sp,0);
		}
  }
done:
/*
  if (need_will_echo)
  {
       send_will (TELOPT_ECHO, 1);
   }*/
   /*
  else
  {
      send_wont(TELOPT_ECHO,1);
	  //sp->will_wont_req_num[TELOPT_ECHO]++;
   }*/
  /*
   * Some things should be deferred until after the pty state has
   * been set by the local procesp->  Do those things that have been
   * deferred now.  This only happens once.
   */

    if (sp->_terminit == 0)
    {
        sp->_terminit = 1;
        defer_terminit (sp);
    }
    net_flush (sp);
    return;
}				/* end of localstat */
void defer_terminit (struct session *sp)
{
  /*
   * The only other module that currently defers anything.
   */
    deferslc (sp);
}				
int terminit (struct session *sp)
{
    return (sp->_terminit);
}
//对子选项进行处理
int subopt(struct session *sp,char c)
{
    struct fsm *pf;
	int ti;
	char nc;
	ti=subtrans_table[sp->sub_state][c&0xff];
	pf=&subtrans[ti];
	//wrtlog("From %s cur sub_state=%s next sub state=%s choose %d sub func\n",sp->client_addr,SUBSTATE(sp->sub_state),SUBSTATE(pf->next_state),ti);
	if(pf->action)
	    (pf->action)(sp,c);
	sp->sub_state=pf->next_state;
	return 1;
}
//子选项结束，对子选项的结果进行处理
int subend(struct session *sp,char c)
{  
   if(sp->sub_state==SSGETSLC)
   {
        start_slc (sp);
	    do_opt_slc (sp,sp->sub_neg_buffer, sp->sub_neg_len);
	    end_slc (sp,0);
    }
   else if(sp->sub_state==SSGETTERM)
   {
        if(sp->sub_neg_len>=MAX_TERM)
	        sp->sub_neg_len=MAX_TERM;
        memcpy(sp->term,sp->sub_neg_buffer,sp->sub_neg_len);
	    sp->term[sp->sub_neg_len]=0;
	    wrtlog("From %s the term is %s\n",sp->client_addr,sp->sub_neg_buffer);
   }
   else if(sp->sub_state==SSNAWS)
   {
        if(sp->sub_neg_len==4)
	    {
	        sp->ws.ws_col = sp->sub_neg_buffer[0]<<8|sp->sub_neg_buffer[1];
	        sp->ws.ws_row = sp->sub_neg_buffer[2]<<8|sp->sub_neg_buffer[3];
		    wrtlog("From %s col=%d row=%d\n",sp->client_addr,sp->ws.ws_col,sp->ws.ws_row);
		    if(strncmp("VT100",sp->term,5)==0||strlen(sp->term)==0||strncmp(sp->term,"ANSI",4)==0)
            {
		        char buf[100]={0x1b,0x5b,0x32,'J',0x1b,0x5b,'1',';','1','H',0};
		        net_put_len(sp,buf,strlen(buf));
		        CMD_OVER();
	        }
	    }
   }
   sp->sub_neg_len=0;
   sp->sub_state=SSTART;
   return 1;
}
//发送同步信号
int myabort(struct session *sp,char c)//synchronizes betwen client and server
{
	char buf[3];
	buf[0]=IAC;
	buf[1]=DM;
	buf[2]=0;
	net_put(sp,buf);
	set_neturg(sp);
	net_flush(sp);
}
//设置显示标志
int set_disp(struct session *sp,int on)
{
    sp->disp_ok=on;
    if(!on&&sp->monitor_thread)
	{
		pthread_cancel(sp->monitor_thread);
        sp->monitor_thread=0;
	}
    return 1;
}
//检查monitor线程是否继续运行
int continue_disp(struct session *sp)
{
    return sp->disp_ok&&!sp->logoff;
}
//退出前进行一些处理
int logout(struct session *sp)
{
    set_disp(sp,0);
    cmd_out_flush(sp);
    net_flush(sp);
	send_will(sp,TELOPT_LOGOUT,1);
	send_do(sp,TELOPT_LOGOUT,1);
	sp->logoff=(char)1;
	return 1;
}
/*
  打开或关闭行模式
  on不为0打开行模式，否则关闭行模式
*/
void set_line_mode(struct session* sp,int on)
{
    if(on)
    {
        if(sp->telnet_mode==REAL_LINEMODE)
            return;
        sp->max_telnet_mode=REAL_LINEMODE;
        send_do(sp,TELOPT_LINEMODE,1);
    }
    else
    {
        if(sp->max_telnet_mode>=KLUDGE_LINEMODE)
            sp->max_telnet_mode=NO_LINEMODE;
	    else
	        return;
	    if(sp->telnet_mode==REAL_LINEMODE)
	    {
	        send_dont(sp,TELOPT_LINEMODE,0);
	        sp->do_dont_req_num[TELOPT_LINEMODE]++;
	        send_will(sp,TELOPT_SGA,1);
	        send_will(sp,TELOPT_ECHO,1);
	    }
	    else if(sp->telnet_mode==KLUDGE_LINEMODE)
	    {
	        send_will(sp,TELOPT_SGA,1);
	    }
    }
}
