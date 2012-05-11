#include "telnet_server.h"
int get_char(struct session *sp,char c)
{
    sp->sub_neg_buffer[sp->sub_neg_len++]=c;
    return 1;
}
int sub_check(struct session *sp,char c)
{
    if(his_state_is_wont(sp,c))
    {
        sp->sub_state=SSEND;
    }
    return 1;
}
int sub_recopt(struct session *sp,char c)
{
    sp->cmd=c;
    return 1;
}
int do_mode(struct session *sp,char c)
{
    sp->useeditmode=c;
    client_state(sp,LM_MODE,0);
    return 1;
}
int sub_d_notsupport(struct session *sp,char c)
{
    if (sp->fw_will_wont_req_num)
    {
        sp->fw_will_wont_req_num--;
        if (sp->fw_will_wont_req_num && ((MY_STATE_WILL&sp->forwardmask)&&sp->cmd==DO||
	        (!(MY_STATE_WILL&sp->forwardmask)&&sp->cmd==DONT)))
	        sp->fw_will_wont_req_num--;
    }
    if(sp->fw_will_wont_req_num==0&&(sp->cmd==DO&&!(MY_WANT_STATE_WILL&sp->forwardmask)))
    {
        char bytes[]={IAC,SB,WONT,LM_FORWARDMASK,IAC,SE,0};
	    net_put(sp,bytes);
	    sp->fw_will_wont_req_num++;
	    sp->forwardmask|=MY_WANT_STATE_WILL;
    }
    else if(sp->fw_will_wont_req_num==0&&(sp->cmd==DONT&&(MY_WANT_STATE_WILL&sp->forwardmask)))
    {
        if(sp->forwardmask&MY_STATE_WILL)
	    {
            char bytes[]={IAC,SB,WONT,LM_FORWARDMASK,IAC,SE,0};
	        net_put(sp,bytes);
	    }
	    sp->forwardmask&=~(MY_WANT_STATE_WILL|MY_STATE_WILL);
    }
    return 1;
}
int sub_w_notsupport(struct session *sp,char c)
{
    if (sp->fw_do_dont_req_num)
    {
        sp->fw_do_dont_req_num--;
        if (sp->fw_do_dont_req_num && ((MY_STATE_DO&sp->forwardmask)&&sp->cmd==WILL
	      ||(!(MY_STATE_DO&sp->forwardmask)&&sp->cmd==WONT)))
	        sp->fw_do_dont_req_num--;
   }
   if(sp->fw_do_dont_req_num==0)
   {
        char bytes[]={IAC,SB,DONT,LM_FORWARDMASK,IAC,SE,0};
        if(sp->cmd==WILL)
	    {
	        if(!(MY_WANT_STATE_DO&sp->forwardmask))
		    {		   
		        sp->forwardmask|=MY_WANT_STATE_DO;
		        net_put(sp,bytes);
		        sp->fw_do_dont_req_num++;  
		    }
		    else
		    {
		        sp->forwardmask|=MY_STATE_DO;
		    }
	    }
	    else
	    {
	        if(MY_WANT_STATE_DO&sp->forwardmask)
		    {
			    sp->forwardmask&=~MY_WANT_STATE_DO;
	            if (MY_STATE_DO&sp->forwardmask)
	                net_put(sp,bytes);
		    }
		    sp->forwardmask&=~MY_STATE_DO;
	   }
    }
    return 1;
}
//create a monitor thread
/*
   创建一个monitor线程
   func 函数指针
   args:参数指针
*/
int create_monitor(struct session *sp,pthread_attr_t *ta,func_t func,void *args)
{
    if(sp->monitor_thread!=0)//monitor线程已存在，直接返回
        return 1;
	sp->disp_ok=1;
    if(pthread_create(&sp->monitor_thread,ta,func,args)<0)//创建失败，退出
        return 0;
    if(pthread_detach(sp->monitor_thread) != 0)
	{
		wrtlog("cannot detach thread do %s\n",sp->client_addr);
		//return 1;
	}
	return 1;
}//end a monitor thread
void end_monitor(struct session* sp)//关闭线程，同时将线程id清0
{
    sp->monitor_thread=0;
    pthread_exit(NULL);
}
//get column of client screen
int get_linesize(struct session *sp)//获取客户屏幕宽度
{
    if(his_state_is_will(sp,TELOPT_NAWS)&&sp->ws.ws_col!=0)
        return sp->ws.ws_col;
    else
        return LINE_SIZE;
}
