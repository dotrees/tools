#include "telnet_server.h"
char host[MAX_HOST];//server ip address
int port;//server port
FILE *logfile;//logfile pointer
pthread_mutex_t log_lock;//log file lock
pthread_mutex_t var_lock;//session lock
char log_file_name[MAX_DIR];//log file path
char work_dir[MAX_DIR];//server work directory
struct session *session_head;//session  link head
int keepalive;//1 if keep the socket alive
int master_socket;//service socket
int nclients;//clients number
telnet_mode_t max_telnet_mode; //available NO_LINEMODE,KLUDGE_LINEMODE,REAL_LINEMODE
//initiate  server
void init_server_cfg()
{
    init_fsm();//初始化状态机
    pthread_mutex_init(&log_lock,0);//初始化锁
    pthread_mutex_init(&var_lock,0);
#ifdef DEFAULTIP//设置服务器ip地址
    memcpy(host,DEFAULTIP,strlen(DEFAULTIP)+1);
#else
    memcpy(host,"localhost",strlen("localhost")+1);
#endif
#ifdef DEFAULTPORT//设置服务器默认端口
    port=DEFAULTPORT;
#else
    port=23;
#endif
#ifdef DEFAULTIDR//设置服务器工作目录
	memcpy(work_dir,DEFAULTDIR,strlen(DEFAULTDIR)+1);
#else
    memcpy(work_dir,"./",strlen("./"));
#endif
#ifdef DEFAULTLOG//设置日志文件
	memcpy(log_file_name,DEFAULTLOG,strlen(DEFAULTLOG)+1);
#else
    memcpy(log_file_name,"./telnet-server.log",strlen("./telnet-server.log")+1);
#endif	
	keepalive=0;
#ifdef MAX_TELNET_MODE
	max_telnet_mode=MAX_TELNET_MODE;//设置服务器最大支持模式
#else
    max_telnet_mode=NO_LINEMODE;
#endif
	session_head=0;//session链初始化为空
	logfile= fopen(log_file_name, "a+");//打开日志文件
    if (!logfile)
        perror("fopen()");
	wrtlog("host=%s  port=%d work_directory=%s log_file=%s(pid: %d)\n",
	      host,port,work_dir,log_file_name,getpid());
}
//begin service
int telnet_service(char *ip, int port_number)
{
	if(ip != NULL) {
		strcpy(host, ip);
	}

/*	int port_n = -1;
	if(port_number == 0)
		port_n = port;
	else 
		port_n = port_number;*/

	int port_n = port_number;
   struct sockaddr_in addr;
   int addrlen,on=1;
   chdir(work_dir);//切换到工作目录
   signal(SIGCHLD, SIG_IGN);
   if ((master_socket= socket(PF_INET, SOCK_STREAM, 0)) < 0) {//创建主套接字
        perror("socket()");
	    return 1;
    }
    setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, &on,sizeof(on));//开启地址重用
	setsockopt(master_socket,SOL_SOCKET,SO_OOBINLINE,(char*)&on,sizeof(on));
	if (keepalive
      && setsockopt (master_socket, SOL_SOCKET, SO_KEEPALIVE,(char *) &on, sizeof (on)) < 0)
	{
	    perror("keepalive");
	}
	memset(&addr,0,sizeof(addr));
	addr.sin_family = AF_INET;
    addr.sin_port = htons(port_n);
    if((addr.sin_addr.s_addr = inet_addr(host))==INADDR_NONE)//设置服务器地址
	    addr.sin_addr.s_addr=INADDR_ANY;
    addrlen = sizeof(struct sockaddr_in);
    if (bind(master_socket, (struct sockaddr *) &addr, addrlen) < 0) {
		close(master_socket);
		perror("bind()");
		return 1;
    }
#ifdef MAX_CLINETS_WAIT
	if (listen(master_socket, MAX_CLINETS_WAIT) < 0) {
#else
    if(listen(master_socket,5)<0){
#endif
		perror("listen()");
		return 1;
    }
	while (1) {
	    int sock;
		struct session *sp=0;
        addrlen = sizeof(struct sockaddr_in);
		//wrtlog("in service loop:\n\r");
        sock= accept(master_socket, (struct sockaddr *) &addr, &addrlen);
        if (sock < 0) {
            perror("accept()");
			clean_all();//fatal error
			return;
        }
		if(!(sp=create_session()))//创建一个session
		{
		    wrtlog("can't create session\n");
			close(sock);
			continue;
		}
		sp->client_socket=sock;//设置套接字
		time(&sp->log_time);		//记录登录时间
		set_client_addr(sp,&addr);//记录客户端地址
		//struct linger lgr;
	    //lgr.l_onoff=1;
	    //lgr.l_linger=0;
	    //setsockopt(sp->client_socket,SOL_SOCKET,SO_LINGER,&lgr,sizeof(struct linger));
        wrtlog("Request from: %s at %s\n",sp->client_addr,ctime(&sp->log_time));
		if(pthread_create(&(sp->thread_id),NULL,(void* (*)(void*))do_service,(void*)sp))//创建线程开始服务
		{
		    wrtlog("error create thread\n");
		    wrtlog(strerror(errno));
		    clean_all();
		    return;
		}		
        if(pthread_detach(sp->thread_id)!=0)
		{
		    wrtlog(strerror(errno));
		    clean_all();
		    return;
		}
	}
}
//serve a client
int do_service(struct session* sp)
{
	fd_set in,out,exp;
#ifdef MAX_INACTIVE_TIME_IN_SEC//设置服务器最大等待客户端的时间
	struct timeval time_span={MAX_INACTIVE_TIME_IN_SEC,0};
#else
    struct timeval time_span={60*5,0};
#endif
	sp->logoff=0;//设置客户端是否退出标志
	time(&sp->log_time);//记录客户登录时间
	dup2(sp->client_socket, 0);//复制标准输入
    //dup2(sp->client_socket, 1);
    //dup2(sp->client_socket, 2);
	init_term(sp);//初始化终端信息
	get_slc_defaults (sp);//设置slc
	io_setup(sp);//初始化缓冲区指针
	sp->telnet_mode=NO_LINEMODE;//设置默认模式：字符模式
	sp->max_telnet_mode=max_telnet_mode;//设置客户端的最大支持模式
	if (my_state_is_wont (sp,TELOPT_SGA))//服务器抑制sga
        send_will (sp,TELOPT_SGA, 1);
    send_do (sp,TELOPT_ECHO, 1);//测试客户端是否是较老的系统
    if (his_state_is_wont (sp,TELOPT_LINEMODE)&&sp->max_telnet_mode>=REAL_LINEMODE)//如果支持行模式，尝试启用行模式
    {
      /* Query the peer for linemode support by trying to negotiate
         the linemode option. */
	  //sp->telnet_mode=NO_LINEMODE;
        sp->linemode = 0;
        sp->editmode = 0;
        send_do (sp,TELOPT_LINEMODE, 1);	/* send do linemode */
    }
	send_do (sp,TELOPT_TTYPE,1);
    send_do (sp,TELOPT_NAWS, 1);//试图获取客户端的窗口大小
	while(his_will_wont_is_changing (sp,TELOPT_NAWS)&&!sp->logoff)
	{
	    io_drain(sp);
	}
	if (his_want_state_is_will (sp,TELOPT_ECHO) && his_state_is_will (sp,TELOPT_NAWS))//设置echo选项
	{
        while(his_will_wont_is_changing (sp,TELOPT_ECHO)&&!sp->logoff)
	    {
	        io_drain(sp);
	    }
	}
	if (sp->telnet_mode < REAL_LINEMODE&&sp->max_telnet_mode>=KLUDGE_LINEMODE)//如果客户端拒绝实行模式，尝试协商准行模式
	{
        send_do (sp,TELOPT_TM, 1);
		//io_drain(sp);
	}
	/*
	if (his_want_state_is_will (TELOPT_ECHO))
    {
        w_echo(TELOPT_ECHO);
    }*/
	if (my_state_is_wont (sp,TELOPT_ECHO)&&sp->telnet_mode==NO_LINEMODE)//客户端不支持行模式，进入默认模式
	{
        send_will (sp,TELOPT_ECHO, 1);
		//io_drain(sp);
	}
	if(!sp->logoff)
	{
	    io_drain(sp);
	    localstat (sp);//根据服务状态发出选项协商
	    if(my_state_is_wont(sp,TELOPT_ECHO)&&sp->telnet_mode==NO_LINEMODE)//对于windows客户端，有时在字符模式下却要求服务端不提供回显，这时强制设置服务端回显，但不再向客户端发送该选项
	    {
	        set_my_state_will(sp,TELOPT_ECHO);
	        set_my_want_state_will(sp,TELOPT_ECHO);
	    }
	}
	//client_wrtlog(sp,"From %s begin loop\n",sp->client_addr);
	while(!sp->logoff)
	{
	   int s;
	   FD_ZERO(&in);//清空描述符集
	   FD_ZERO(&out);
	   FD_ZERO(&exp);
	   if(!net_in_full(sp))//网络输入缓冲未满，可读套接字
	   {
	        FD_SET(sp->client_socket,&in);
	   }
	   if(net_out_rem(sp)>0)//网络输出缓冲未空，可写套接字
	   {
	        FD_SET(sp->client_socket,&out);
	   }
	   if(!sp->syn_num)//未接收到带外信号，可接收带外数据
	   {
	        FD_SET(sp->client_socket,&exp);
	    }
		if ((s = select (sp->client_socket+1, &in, &out, &exp, &time_span)) <= 0)
	    {
		    //perror("select()");
	        if (s == -1 && errno == EINTR)//被信号中断
	            continue;
			else//超时，关闭套接字
			{
			   //return 0;
			   //sleep(5);
			    cwrtlog(sp,"time out.going to logout\n\r");
				net_put(sp,"Inactive too long...Please log again\n\r");
			    logout(sp);
			    break;
			}
	    }
		if (FD_ISSET (sp->client_socket, &exp))//如果接收到带外信号，置同步信号
		{
	        sp->syn_num = 1;
			wrtlog("From %s get a syn\n\r",sp->client_addr);
		}
        if (FD_ISSET (sp->client_socket, &in))//套接字可读，读取套接字
	    {
	    /* Something to read from the network... */
	    /*FIXME: handle  !defined(SO_OOBINLINE) */
	        if(net_read (sp)<=0)//套接字关闭，清理session，退出
		    {
				cmd_out_flush(sp);
				net_flush(sp);
				time_t logofftime;
				time(&logofftime);
				cwrtlog(sp,"logoff at %s\n",ctime(&logofftime));
	            clean(sp);
		        pthread_exit(NULL);
				return;
		    }
	    }
		if(sp->cmd_outend>sp->cmd_outbeg&&!net_out_full(sp))//待输出的命令buffer不空，输出
		{
		    cmd_out_flush(sp);
		}
		if (FD_ISSET (sp->client_socket, &out) &&sp->net_outend-sp->net_outbeg > 0)//套接字可输出，输出网络缓存
	        net_flush (sp);
        if (sp->net_inlen> sp->net_inp-sp->net_inbuffer)//处理从网络中获得的数据
		{
	        dispose(sp);
		}
		//set_line_mode(REAL_LINEMODE);
		
	}
	clean(sp);//清理session，退出
	pthread_exit(NULL);
}
struct session* create_session()
{
    pthread_mutex_lock(&var_lock);
    if(nclients>=MAX_CLIENTS)//控制同时服务的客户数目
    {
	    wrtlog("too much clients\n");
		pthread_mutex_unlock(&var_lock);
		return NULL;
    }
    if(!session_head)//创建第一个session
    {
	    session_head=malloc(sizeof(struct session));
	    if(!session_head)
	    {
		    pthread_mutex_unlock(&var_lock);
		    return NULL;
	    }
	    memset(session_head,0,sizeof(struct session));
    }
    else//创建session
    {
	    struct session *sp;
	    sp=malloc(sizeof(struct session));
	    if(!sp)
	    {
		    pthread_mutex_unlock(&var_lock);
		    return NULL;
	    }
        memset(sp,0,sizeof(struct session));
	    sp->next=session_head;
        session_head=sp;
    }
    nclients++;
    pthread_mutex_unlock(&var_lock);
    return session_head;
}
