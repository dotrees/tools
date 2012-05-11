#include "telnet_server.h"
//extern pthread_mutex_t log_lock;//日志文件锁
//extern pthread_mutex_t var_lock;//session锁
//extern int nclients;//客户数目
//extern FILE* logfile;//日志文件指针
//extern unsigned char trans_table[][NCHARS];//主状态转移矩阵
//extern unsigned char subtrans_table[][NCHARS];//子选项状态转移矩阵
//extern struct fsm trans[];//主状态数组
//extern struct fsm subtrans[];//子选项状态数组
//extern struct session *session_head;//session头
//extern char crlf[];//换行符
//extern int master_socket;//主套接字
//初始化缓冲指针
void io_setup (struct session* sp)
{
    sp->cmd_outbeg = sp->cmd_outend=sp->cmd_outbuffer;
    sp->net_outbeg=sp->net_outend=sp->net_outbuffer;
    sp->net_inp = sp->net_inbuffer;
    sp->cmd_inp = sp->cmd_inbuffer;
    sp->net_inlen=0;
    sp->cmd_inlen=0;
}
/*
从网络输入缓冲中取len个字符，放入b指向的缓冲中，如果forward大于0，缓冲指针就向前移动，否则指针不移动
返回实际读取的字符数
*/
int net_get(struct session *sp,char *b,int len,int forward)
{
    int nlen=sp->net_inbuffer+sp->net_inlen-sp->net_inp;
    if(nlen<len)
        len=nlen;
    if(b!=NULL)
        memcpy(b,sp->net_inp,len);
    if(forward)
    {
        sp->net_inp+=len;
    }
    if(sp->net_inp>=sp->net_inbuffer+sp->net_inlen)//如果缓冲已读完，将缓冲清空
    {
        sp->net_inp=sp->net_inbuffer;
	    sp->net_inlen=0;
    }
    return len;
}
/*
将数据放入网络输出缓冲中，参数为可变参数
    sp:需要输出的session
	format:要输出的格式化字符串
	返回实际输出的字符数
*/
int net_put(struct session *sp,const char *format,...)
{
   //wrtlog("in net_put\n");
    va_list args;
    int len;
    len=strlen(format);
    int rem=sp->net_outbuffer+sizeof(sp->net_outbuffer)-sp->net_outend;
    while(rem<len)
    {
        net_flush(sp);
	    rem=sp->net_outbuffer+sizeof(sp->net_outbuffer)-sp->net_outend;
    }
    va_start(args,format);
    len=vsnprintf(sp->net_outend,rem,format,args);
    va_end(args);
   //wrtlog("in net_put:buf=%s\n",sp->net_outend);
    if(len>rem)
        len=rem-1;
    sp->net_outend+=len;
    return len;
}
/*
   将长度为len的字符串src放入sp的网络输出缓冲中
   返回实际输出的字符数
*/
int net_put_len(struct session *sp,char *src,int len)
{
   //wrtlog("in net_put_len\n\r");
   int rem=sp->net_outbuffer+MAX_BUF_OUT-sp->net_outend;//如果网络输出缓冲不够，刷新缓冲区
    while(rem<len)
    {
        net_flush(sp);
	    rem=sp->net_outbuffer+MAX_BUF_OUT-sp->net_outend;
    }
   
   //if(sp->net_outbuffer+MAX_BUF_OUT-sp->net_outend<len)
   //   len=sp->net_outbuffer+MAX_BUF_OUT-sp->net_outend;
    memcpy(sp->net_outend,src,len);
    sp->net_outend+=len;
    return len;
}
/*
int net_put_len_ln(char *src,int len)
{
   return net_put_len(src,len)+net_put("\r\n");
}*/
/*
    刷新网络缓冲
	sp:要刷新的session
*/
void net_flush(struct session* sp)
{
    int len=sp->net_outend-sp->net_outbeg;
  // wrtlog("in net_flush len=%d buff=%s\n",len,sp->net_outend);
    if(len>0)
    {
        if(sp->urgent!=0)
	    {
		    len=send(sp->client_socket,sp->net_outbeg,sp->urgent-sp->net_outbeg,MSG_OOB);
	    }
	    else
	        len=send(sp->client_socket,sp->net_outbeg,len,0);
    }
    sp->net_outbeg+=len;
    if(sp->net_outbeg==sp->net_outend)
    {
        sp->net_outbeg=sp->net_outend=sp->net_outbuffer;
    }
    if(sp->net_outbeg>=sp->urgent)
        sp->urgent=0;
}
/*
  如果服务器需要设置紧急数据，在调用net_put或者net_put_len发送紧急数据后，
  调用set_neturg()设置紧急指针
*/
void set_neturg (struct session* sp)
{
    sp->urgent = sp->net_outend - 1;
}
/*
   从网络中读取数据
   返回读取的字符数
*/
int net_read (struct session* sp)
{
    sp->net_inlen = read (sp->client_socket, sp->net_inbuffer, MAX_BUF_IN);
    if (sp->net_inlen < 0 && errno == EWOULDBLOCK)
        sp->net_inlen = 0;
  //else if (sp->net_inlen == 0)
  //{
  //    wrtlog("From %s telnetd:  peer died\n",sp->client_addr);
  //}
    sp->net_inp= sp->net_inbuffer;
 // wrtlog("From %s get %d bytes\n",sp->client_addr,sp->net_inlen);
     return sp->net_inlen;
}
int net_out_full(struct session* sp)//测试网络输出缓冲是否已满
{
    return sp->net_outend>=sp->net_outbuffer+sizeof(sp->net_outbuffer);
}
int net_out_rem(struct session* sp)//返回网络输出缓冲剩余的空间
{
    return sp->net_outend-sp->net_outbeg;
}
int net_in_full(struct session* sp)//测试网络输入缓冲是否已满
{
    return sp->net_inlen>=sizeof(sp->net_inbuffer);
}
/*
   将输出数据放入待输出的命令输出buffer中
   sp:对应的session结构
   data:要输出的数据
   len:要输出的长度
   最大输出长度不能超过MAX_BUF_OUT
*/
void cmd_output (struct session *sp,const char *data, int len)
{
    //need filter slc
    //printf("From %s in cmd_output input=%s len=%d strlen(data)=%d\n",sp->client_addr,(char*)data,len,strlen((char*)data));
    if ((&sp->cmd_outbuffer[MAX_BUF_OUT] - sp->cmd_outend) <len)
    {
        cmd_out_flush (sp);
    }
    if(len>&sp->cmd_outbuffer[MAX_BUF_OUT]-sp->cmd_outend)
        len=&sp->cmd_outbuffer[MAX_BUF_OUT]-sp->cmd_outend;
    memcpy (sp->cmd_outend, data, len);
    sp->cmd_outend += len;
}
/*
  同cmd_output()
  但在输出后加上换行符
*/
void cmd_output_ln(struct session *sp,char *data,int len)
{
    cmd_output(sp,data,len);
    cmd_output(sp,crlf,strlen(crlf));
}
/*
   向命令输入buffer中放入命令数据
   src:指向要放入的数据
   len:数据长度
   返回实际放入的长度
*/
int cmd_input_len(struct session* sp,char *src,int len)
{
    if(MAX_BUF_IN-sp->cmd_inlen<len)
	    len=MAX_BUF_IN-len;
	memcpy(sp->cmd_inbuffer+sp->cmd_inlen,src,len);
	sp->cmd_inlen+=len;
	return len;
}
/*
   刷新命令输出缓冲
   完成的工作：
   1）在IAC(255)的字符前再添加一个IAC
   2）对于不支持八位字符的客户端，清除字符的第八位
   3）对输入中只出现一个'\r'，自动在其后添加'\n'或0
   4）命令输出缓冲清空
   在调用cmd_output后需调用此函数将数据刷新到网络输出缓冲中
*/
void cmd_out_flush (struct session* sp)
{
    int n;
    // wrtlog("From %s in cmd_out_flush:len=%d \n",sp->client_addr,sp->cmd_outend - sp->cmd_outbeg);
    char *p=sp->cmd_outbeg;
    if ((n = sp->cmd_outend - sp->cmd_outbeg) > 0)
    {  
        for(p=sp->cmd_outbeg;p<sp->cmd_outend;p++)
	    {
	        if(*p==(char)IAC)
		        net_put_len(sp,p,1);
		    net_put_len(sp,p,1);
		    if(my_state_is_wont(sp,TELOPT_BINARY))
		        *p=*p&0x7f;
		    if(*p=='\r'&&my_state_is_wont (sp,TELOPT_BINARY))
		    {
		        if (p<=sp->cmd_outend-2 && *(p+1) == '\n')
			    {
		            net_put_len(sp,++p,1);
			    }
	            else
			    {
			        char c='\0';
		            net_put_len(sp,&c,1);
			    }
            }
	    }
    }
    sp->cmd_outbeg=sp->cmd_outend=sp->cmd_outbuffer;
}
//从命令输入缓冲中获取len个字符，存储在b中，如果forward大于0，则向前移动指针
int cmd_get(struct session *sp,char *b,int len,int forward)
{
    int nlen=sp->cmd_inlen+sp->cmd_inbuffer-sp->cmd_inp;
    if(nlen<len)
        len=nlen;
    memcpy(b,sp->cmd_inp,len);
    if (forward)
        sp->cmd_inp+=len;
    if(sp->cmd_inp>=sp->cmd_inbuffer+sp->cmd_inlen)
    {
        sp->cmd_inp=sp->cmd_inbuffer;
	    sp->cmd_inlen=0;
    }
    return len;
}
/*
int cmd_input_putback (const char *str, size_t len)
{
  if (len > &sp->cmd_inbuffer[MAX_BUF-1] - sp->cmd_inp)
    len = &sp->cmd_inbuffer[MAX_BUF-1] - sp->cmd_inp;
  strncpy (sp->cmd_inp, str, len);
  sp->cmd_inlen+=len;
}
*/
/*
int cmd_read ()
{
  sp->cmd_inlen = read (cmd, sp->cmd_inbuffer, MAX_BUF);
  if (sp->cmd_inlen < 0 && (errno == EWOULDBLOCK
#ifdef	EAGAIN
		  || errno == EAGAIN
#endif
		  || errno == EIO))
    sp->cmd_inlen = 0;
  sp-> cmd_inp= sp->cmd_inbuffer;
  return sp->cmd_inp;
}
*/
int cmd_out_full(struct session* sp)//测试命令输出缓冲是否已满
{
    return (sp->cmd_outend>=sp->cmd_outbuffer+sizeof(sp->cmd_outbuffer)-1);
}
int cmd_in_rem(struct session* sp)//测试命令输入缓冲剩余空间
{
    return (sizeof(sp->cmd_inbuffer)-sp->cmd_inlen-1);
}
/*
  从网络缓冲中读取字符串，然后处理该字符串
*/
void io_drain (struct session* sp)
{
  //wrtlog("From %s in io_drain\r\n",sp->client_addr);
    if (sp->net_outend - sp->net_outbeg > 0)
        net_flush (sp);
again:
    sp->net_inlen = read (sp->client_socket, sp->net_inbuffer, sizeof(sp->net_inbuffer));
    if (sp->net_inlen < 0)
    {
        if (errno == EAGAIN)
	    {
	        goto again;
	    }
        //clean(sp);
	    sp->logoff=1;
	    return;
    }
    else if (sp->net_inlen == 0)
    {
        clean(sp);
    }
    sp->net_inp = sp->net_inbuffer;
    dispose(sp);		/* state machine */
    if (sp->net_inp <sp->net_inbuffer+sp->net_inlen)//可能命令输出缓冲已满，刷新该缓冲，处理剩下的字符
    {
        cmd_out_flush(sp);
        dispose(sp);
    }
}
char *nextitem (char *current)
{
    if ((*current & 0xff) != IAC)
        return current + 1;
    switch (*(current + 1) & 0xff)
    {
        case DO:
        case DONT:
        case WILL:
        case WONT:
            return current + 3;
        case SB:			/* loop forever looking for the SE */
        {
	        char *look = current + 2;
	        for (;;)
	        if ((*look++ & 0xff) == IAC && (*look++ & 0xff) == SE)
	             return look;
        }
        default:
	        return current + 2;
    }
}
/*
  清除一个长度为len的缓冲中的数据，但保留命令
*/
#define wewant(p) \
  ((buf_beg+len > p) && ((*p&0xff) == IAC) && \
   ((*(p+1)&0xff) != EC) && ((*(p+1)&0xff) != EL))
int buffer_clear (char *buf_beg,char *cmd_start,int len)//clear all data but cmd 
{
    char *thisitem, *next;
    char *good;
    thisitem = buf_beg;
    while ((next = nextitem (thisitem)) <= cmd_start)
        thisitem = next;

    /* Now, thisitem is first before/at boundary. */
    good = buf_beg;		/* where the good bytes go */
    while (buf_beg+len> thisitem)
    {
        if (wewant (thisitem))
	    {
	        int length;
	        for (next = thisitem; wewant (next) && buf_beg+len > next;next = nextitem (next));
	        length = next - thisitem;
	        memmove (good, thisitem, length);
	        good += length;
	        thisitem = next;
	    }
        else
	    {
	        thisitem = nextitem (thisitem);
	    }
    }
    return good-buf_beg;
  //sp->net_outbeg = sp->net_outbuffer;
  //sp->net_outbeg = good;		/* next byte to be sent */
  //sp->urgent = 0;
}				/* end of netclear */
/*
  向日志文件写入日志；
  参数为可变参数
*/					
void wrtlog(char *format,...)
{
	va_list arg;
#ifdef _TELNET_DEBUG_
	char buff[MAX_BUF_OUT];
#endif
    if(logfile)
    {
	    va_start(arg,format);
	    pthread_mutex_lock(&log_lock);
	    vfprintf(logfile,format,arg);
	    pthread_mutex_unlock(&log_lock);
	    va_end(arg);
    }
#ifdef _TELNET_DEBUG_
	va_start(arg,format);
	vsnprintf(buff,512,format,arg);
	va_end(arg);
	printf(buff);
#endif
}
void wrt_log(char *src)
{
    pthread_mutex_lock(&log_lock);
	if(logfile)
	    fprintf(logfile,src);
	pthread_mutex_unlock(&log_lock);
#ifdef _TELNET_DEBUG_
    printf(src);
#endif
}
/*
  类似于wrtlog,但是在写入日志前，写入"From <client ip address>"以区别处理不同客户时写入的日志
*/
void cwrtlog(struct session *sp,char *format,...)
{
    char buff[MAX_BUF_OUT];
    va_list args;
    sprintf(buff,"From %s:",sp->client_addr);
    va_start(args,format);
    vsnprintf(buff+strlen(buff),MAX_BUF_OUT-strlen(buff),format,args);
    va_end(args);
	wrt_log(buff);
}
char* peerip(struct session *sp,char *buf,int len)
{
    struct sockaddr_in addr;
    int addr_len;
    if(getpeername(sp->client_socket,(struct sockaddr*)&addr,&addr_len)!=0)
    {
        wrtlog("int client_wrtlog: bad socket:%d\n",sp->client_socket);
    }
    return (char*)inet_ntop(AF_INET,(void*)&addr.sin_addr,buf,len);
}
void telnet_ntoa(u_short src,char *buf,int len)//将整数转换为字符串
{
    int i=0;
   //printf("src=%u\n",src);
    while(src!=0&&i<len)
    {
        buf[i]=(char)(0x30|(src%10));
	    src/=10;
	    i++;
    }
    int j=0;
    while(j<i/2)
    {
        buf[j]=buf[i-j-1]^buf[j];
	    buf[i-j-1]=buf[j]^buf[i-j-1];
	    buf[j]=buf[j]^buf[i-j-1];
	    j++;
    }
    buf[i]=0;
   //printf("buf=%s\n",buf);
}
void set_client_addr(struct session* sp,struct sockaddr_in *addr)//设置session中的客户端地址
{
    int len;
    inet_ntop(AF_INET,(void*)&addr->sin_addr,sp->client_addr,sizeof(sp->client_addr));
    len=strlen(sp->client_addr);
    memcpy(sp->client_addr+len,".",2);
    len++;
    telnet_ntoa(ntohs(addr->sin_port),sp->client_addr+len,sizeof(sp->client_addr)-len);
}
//初始化状态机
void fsminit(unsigned char trans_tab[][NCHARS],struct fsm fsm_trans[],int nstates)
{
	struct fsm *pf;
	int i,j,k;
	for(i=0;i<nstates;i++)
		for(j=0;j<NCHARS;j++)
		    trans_tab[i][j]=(unsigned char)INVALID;
	for(i=0;fsm_trans[i].current_state!=SINVALID;i++)
	{
		if(fsm_trans[i].input==CANY)
		{
			for(j=0;j<NCHARS;j++)
			{
				if(trans_tab[fsm_trans[i].current_state][j]==(unsigned char)INVALID)
				    trans_tab[fsm_trans[i].current_state][j]=(unsigned char)i;
			}
		}
		else
			trans_tab[fsm_trans[i].current_state][fsm_trans[i].input]=(unsigned char)i;
	}
	k=i;
	for(i=0;i<NCHARS;i++)
	{
		for(j=0;j<nstates;j++)
			if(trans_tab[j][i]==(unsigned char)INVALID)
		    {
			     trans_tab[j][i]=(unsigned char)(k);
		    }
	}
}
//初始化主状态机和子选项状态机
void init_fsm()
{
	fsminit(trans_table,trans,NSTATES);
	fsminit(subtrans_table,subtrans,NSSTATES);
}
//去掉一个字符串前的' ' '\t' '\n'等字符
void fre_strip(char *buf,int len,char **out)
{
	char del[]="\t\n\r ";
	int i=0;
	while(i<len&&strchr(del,buf[i])!=NULL)
	{
		i++;
	}
	*out=buf+i;
}
//去掉一个字符串后的空格
void  back_strip(char *buf,int len,char **out)
{
	char del[]="\t\n\r ";
	int i=len-1;
	while(i>=0&&strchr(del,buf[i])!=NULL)
		i--;
	*out=buf+i+1;
}
//删除一个session sp
void clean(struct session *sp)
{
    if(!sp)
	{
        return;
	}
	shutdown(sp->client_socket,2);
    close(sp->client_socket);
    set_disp(sp,0);
    struct session *ss,*pre;
    ss=session_head;
    pre=session_head;
	pthread_mutex_lock(&var_lock);
    while(ss&&ss->thread_id!=sp->thread_id)
    {
        pre=ss;
        ss=ss->next;
    }
    if(!ss)
	    wrtlog("Can't find the client session.Should never happen here\n");
    else if(ss==session_head)
    {
        session_head=ss->next;
	    free(ss);
    }
    else
    {
	    pre->next=ss->next;
	    free(ss);
    }
    if(nclients>0)
        nclients--;
  // wrtlog("current clients number=%d\n",nclients);
    pthread_mutex_unlock(&var_lock);
    return;
}
//清除所有session,关闭服务器
void clean_all()
{
    struct session *sp,*np;
    sp=session_head;
    while(sp)
    {
        np=sp;
	    close(sp->client_socket);
	    sp->disp_ok=0;
	    sp=np->next;
	    free(np);
    }
    session_head=0;
    close(master_socket);
    close(fileno(logfile));
    pthread_exit(NULL);
}
//获取当前session
struct session* get_session()
{
    pthread_t tid;
    tid=pthread_self();
    struct session* sp;
    sp=session_head;
    while(sp&&sp->thread_id!=tid)sp=sp->next;
    return sp;
}
