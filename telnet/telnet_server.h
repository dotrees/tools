#ifndef _TELNET_SERVER_H_
#define _TELNET_SERVER_H_

//#define _TELNET_DEBUG_  //debug trigle
#define TERM_DESC //defined for using custom termios struct
#define _CASE_INSENSITIVE//defined if want to be case insensitive

#include <arpa/telnet.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
//#include <resolv.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <signal.h>
#include <getopt.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <time.h>
# include <termios.h>
# include <termio.h>
#include <wait.h>
#include <pthread.h>
#include <sys/resource.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
  telnet model:
     cmd_dispose---------->cmd_out_buffer-------------->telnetd--------------->net_out_buffer----->socket
	 cmd_dispose<----------cmd_in_buffer <-------------telnetd<---------------net_in_buffer<------socket
*/
//#define DEFAULTPROMPT "<telnet service>>> "//define the telnet prompt
//#define LOGOUT "logout"//define logout command
#define MAX_TELNET_MODE NO_LINEMODE//define initial line mode;
//#define DEFAULTIP 192.168.158.133//define host ip address
#define DEFAULTPORT 23//set default port
#define DEFAULTDIR "./"//set default work directory
#define DEFAULTLOG "./telnet-server.log"//set default log file name
//default max
#define MAX_CLIENTS_WAIT 5//max number of clients waiting for served
#define MAX_CLIENTS 5//max clients servied concurrently
#define MAX_HOST 100//server address buffer length
#define MAX_TERM 10//number of bytes used to store client's term type
#define OPTIONS 256//options buffer length
#define MAX_BUF_OUT 10240//out buffer size(must more than 500)
#define MAX_BUF_IN 256// in buffer size
#define MAX_DIR 100//path buffer size
#define NCHARS 256//a byte can represent 256 characters
#define INVALID 0xff
#define CANY (NCHARS+1)
#define CR '\r'
#define LINE_SIZE 80//a default line size for client screen
#define MAX_CMD 20 //default max number of commands to store for clients to retrive,the max length of a command is LINE_SIZE
#define ADDR_LEN 22//ip address buffer length.like xxx.xxx.xxx.xxx.xxxxx,last five bytes for client's port
#define MAX_INACTIVE_TIME_IN_SEC 60*100//if a client doesn't send data in this seconds,the server will close the socket in force

//master state machine
#define SDATA (unsigned char)0  //recieved data
#define SIAC (unsigned char)1   //received one iac
#define SDOPT (unsigned char)2  //recieved a do or dont command
#define SWOPT (unsigned char)3  //recieved a will or wont command
#define SSUBNEG (unsigned char)4//suboption
#define SSUBIAC (unsigned char)5//in suboption but recieved an iac
#define NSTATES (unsigned char)6 //number of states

//sub negotiate machine
#define SSTART (unsigned char)0//sub negotiation begin
#define SSLINEMODE (unsigned char)1//recieved LINEMODE option
#define SSTERMTYPE (unsigned char)2//recieved terminal type option
#define SSTERMREQUEST (unsigned char)3//get a terminal request...
#define SSGETTERM (unsigned char)4//not use
#define SSLMODE (unsigned char)5//got a LM_MODE option
#define SSGETSLC (unsigned char)6//got a LM_SLC option
#define SSNAWS (unsigned char)7//got a NAWS option
#define SSEND (unsigned char)8//sub negotiation end
#define SSLMDO (unsigned char)9//got a LM_DO
#define SSLMWILL (unsigned char)10//got a LM_WILL
#define NSSTATES (unsigned char)11//number of sub states

//invalid state
#define SINVALID (unsigned char)0xff//invalid state

//option state claim
#define MY_STATE_WILL		0x01
#define MY_WANT_STATE_WILL	0x02
#define MY_STATE_DO		    0x04
#define MY_WANT_STATE_DO	0x08

#define my_state_is_do(sp,opt)		((sp)->options[opt]&MY_STATE_DO)
#define my_state_is_will(sp,opt)		((sp)->options[opt]&MY_STATE_WILL)
#define my_want_state_is_do(sp,opt)	((sp)->options[opt]&MY_WANT_STATE_DO)
#define my_want_state_is_will(sp,opt)	((sp)->options[opt]&MY_WANT_STATE_WILL)

#define my_state_is_dont(sp,opt)		(!my_state_is_do(sp,opt))
#define my_state_is_wont(sp,opt)		(!my_state_is_will(sp,opt))
#define my_want_state_is_dont(sp,opt)	(!my_want_state_is_do(sp,opt))
#define my_want_state_is_wont(sp,opt)	(!my_want_state_is_will(sp,opt))

#define set_my_state_do(sp,opt)		((sp)->options[opt] |= MY_STATE_DO)
#define set_my_state_will(sp,opt)		((sp)->options[opt] |= MY_STATE_WILL)
#define set_my_want_state_do(sp,opt)	((sp)->options[opt] |= MY_WANT_STATE_DO)
#define set_my_want_state_will(sp,opt)	((sp)->options[opt] |= MY_WANT_STATE_WILL)

#define set_my_state_dont(sp,opt)		((sp)->options[opt] &= ~MY_STATE_DO)
#define set_my_state_wont(sp,opt)		((sp)->options[opt] &= ~MY_STATE_WILL)
#define set_my_want_state_dont(sp,opt)	((sp)->options[opt] &= ~MY_WANT_STATE_DO)
#define set_my_want_state_wont(sp,opt)	((sp)->options[opt] &= ~MY_WANT_STATE_WILL)
//symetrical

#define HIS_STATE_WILL			MY_STATE_DO
#define HIS_WANT_STATE_WILL		MY_WANT_STATE_DO
#define HIS_STATE_DO			MY_STATE_WILL
#define HIS_WANT_STATE_DO		MY_WANT_STATE_WILL

#define my_will_wont_is_changing(sp,opt) (((sp)->options[opt]+MY_STATE_WILL) & MY_WANT_STATE_WILL)
#define my_do_dont_is_changing(sp,opt) (((sp)->options[opt]+MY_STATE_DO) & MY_WANT_STATE_DO)
#define his_state_is_do			my_state_is_will
#define his_state_is_will		my_state_is_do
#define his_want_state_is_do		my_want_state_is_will
#define his_want_state_is_will		my_want_state_is_do

#define his_state_is_dont		my_state_is_wont
#define his_state_is_wont		my_state_is_dont
#define his_want_state_is_dont		my_want_state_is_wont
#define his_want_state_is_wont		my_want_state_is_dont

#define set_his_state_do		set_my_state_will
#define set_his_state_will		set_my_state_do
#define set_his_want_state_do		set_my_want_state_will
#define set_his_want_state_will		set_my_want_state_do

#define set_his_state_dont		set_my_state_wont
#define set_his_state_wont		set_my_state_dont
#define set_his_want_state_dont		set_my_want_state_wont
#define set_his_want_state_wont		set_my_want_state_dont

#define his_will_wont_is_changing	my_do_dont_is_changing
#define his_do_dont_is_changing		my_will_wont_is_changing

typedef enum telnet_mode_t{
        NO_LINEMODE=0,             //charactor mode
		KLUDGE_LINEMODE=1,         //kludge line mode
		REAL_LINEMODE=2            //real line mode
		}telnet_mode_t;
struct slc_entry      //SLC struct
{
	cc_t sysval;//default value
	cc_t curval;//current value;
	unsigned char sysflag;//default flag;
	unsigned char curflag;//current flag;
	cc_t *src;// point to c_cc array item in termios struct
};
//define costom termios struct
#ifdef TERM_DESC
struct term_desc
{
#define TERM_ECHO 1
#define TERM_BINARYIN 2
#define TERM_BINARYOUT 4
#define TERM_EDIT 8
#define TERM_TRAPSIG 16
#define TERM_SOFTTAB 32
#define TERM_LITECHO 64
#define TERM_LINEMODE 128
#define TERM_RAW 256
  unsigned int term_param;
  cc_t cc_c[NCCS];
};
#endif
//session, used to remember every thing about every client
struct session{
   int client_socket;                    //the socket link to the  client
   char client_addr[ADDR_LEN];           //client ip address
   pthread_t thread_id;                  //thread id
   pthread_t monitor_thread;             //if a client has start a monitor thread,record it
   int disp_ok;                          //if disp_ok is 0,the monitor thread will stop monitor
   char logoff;                          //value is 1 if the client is logoff
   
   struct slc_entry slcs[NSLC+1];         //slc buffer
   enum telnet_mode_t telnet_mode;        //current mode
   enum telnet_mode_t max_telnet_mode;    //the max mode can use
   unsigned char uselinemode;             //value is 1 if going to uselinemode
   unsigned char linemode;                //is 1 if current is using linemode
   unsigned char alwayslinemode;          //is 1 if must use line mode in force
   unsigned char editmode;                //current edit mode,used in real linemode
   unsigned char useeditmode;             //edit mode want by client,used in real linemode
   char term[MAX_TERM+1];                 //client terminal-type
   struct winsize ws;                      //client screen size
   unsigned char options[OPTIONS];         //options buffer
   int will_wont_req_num[OPTIONS];         //record will and wont request times
   int do_dont_req_num[OPTIONS];           //record do and dont request times
   unsigned char master_state;             //master state machine state
   unsigned char sub_state;                //sub state machine state
   
   char net_inbuffer[MAX_BUF_IN+1];        //net income buffer
   char net_outbuffer[MAX_BUF_OUT+1];      //net outgoing buffer
   char sub_neg_buffer[MAX_BUF_IN+1];      //sub negotiation buffer
   char cmd_inbuffer[MAX_BUF_IN+1];        //command buffer ready to execute
   char cmd_outbuffer[MAX_BUF_OUT+1];      //outcome produced by command executing
   char *net_inp;                         
   char *net_outbeg,*net_outend,*urgent;
   char *cmd_inp;
   char *cmd_outbeg,*cmd_outend;
   
   int net_inlen;//buffers length
   int net_outlen;
   int sub_neg_len;
   int cmd_inlen;
   int cmd_outlen;
   
   unsigned char *def_slcbuf;//used for slc disposing
   int def_slclen ;
   int slcchange;		/* change to slc is requested */
   unsigned char *slcptr;	/* pointer into slc buffer */
   unsigned char slcbuf[NSLC * 6];	/* buffer for slc negotiation */
   char _terminit;

   int syn_num;//record syn times
   unsigned char cmd;
   time_t log_time;//record  client logon time
   
   struct session* next;//point next session
#ifdef TERM_DESC
   struct term_desc termbuf;
#else
   struct termios termbuf;
#endif
   char cmd_back[MAX_CMD][LINE_SIZE+1];//record recent commands
   int cmds;
   int cmd_num;
   int cmd_cur;
   int linesize;//current length of comamnd line
   int line_pos;//current position of cursor
   
   char forwardmask;//used for REALLINE MODE
   char fw_do_dont_req_num,fw_will_wont_req_num;
};
//finite state machine struct
struct fsm
{
	unsigned char current_state;
	int input;
	unsigned char next_state;
	int (*action)(struct session*,char);
};

extern char host[MAX_HOST];  //server host
extern int port;             //server port
extern FILE *logfile;        //log file pointor
extern char log_file_name[MAX_DIR];              //log file name
extern char work_dir[MAX_DIR];                   //server work directory
extern unsigned char trans_table[][NCHARS];      //master finite state machine state matrix
extern unsigned char subtrans_table[][NCHARS];   //sub finite state machine state matrix
extern struct fsm  trans[];                      //master fsm array
extern struct fsm  subtrans[];                   //sub fsm array
extern char* telnet_line_mode[];                 //literal telnet mode
extern char* telnet_master_state[];              //literal master state
extern char* telnet_sub_state[];                  //literal sub state
extern telnet_mode_t max_telnet_mode;            //initial telnet mdoe
extern int nclients;                             //current number of clients
extern pthread_mutex_t log_lock;                 //mutex lock for logfile
extern pthread_mutex_t var_lock;                 //mutex lock for session link
extern int master_socket;                        //server passive socket
extern char crlf[];                              //'\r\n'
extern struct session *session_head;             //session link
#define STATE(x) telnet_master_state[x]
#define SUBSTATE(x) telnet_sub_state[x]
#define TELNET_MODE(x) telnet_line_mode[x]

/*initiate server*/
void init_server_cfg();
/*serve a client represented by sp*/
int do_service(struct session* sp);
/*put a charactor into client command in buffer*/
int coutput(struct session*,char);
/*set syn flag when datamark recieved*/
int datamark(struct session*,char);
/*record command,which maybe do,dont,will or wont*/
int recopt(struct session*,char);
/*manage sub negotiation*/
int subopt(struct session*,char);
/*end sub negotiation*/
int subend(struct session*,char);
/*manage do or dont echo option*/
int d_echo(struct session*,char);
/*manage do or dont binary option*/
int d_binary(struct session*,char);
/*manage do and dont terminal type option*/
int d_termtype(struct session*,char);
/*manage do and dont linemode option*/
int d_linemode(struct session*,char);
/*manage do and dont option those not supported by this server*/
int d_notsupport(struct session*,char);
/*manage do and dont sppress go ahead option*/
int d_sga(struct session*,char);
/*manage do and dont logout option*/
int d_exit(struct session*,char);
/*manage do and dont timemark option*/
int d_timemark(struct session*,char);
/*manage will and wont echo recieved from client*/
int w_echo(struct session*,char);
/*manage will and wont linemode recieved from client*/
int w_linemode(struct session*,char);
/*manage will and wont binary recieved from client*/
int w_binary(struct session*,char);
/*manage will and wont options recieved from client that aren't supported by server*/
int w_notsupport(struct session*,char);
/*manage will and wont terminal type recieved from client*/
int w_termtype(struct session*,char);
/*manage will and wont recieved from client*/
int w_timemark(struct session*,char);
/*manage will and wont naws recieved from client*/
int w_naws(struct session*,char);
/*manage will and wont logout recieved from client*/
int w_exit(struct session*,char c);
/*try to synchronize with the client*/
int myabort(struct session*,char);
/*record sub negotiation command*/
int sub_recopt(struct session*,char);
/*execute the command sent by client represented by session*/
int do_cmd(struct session*);
//int do_cr(struct session*,char);
/*when recieved "iac ip"*/
int do_ip(struct session*,char c);
/*check if this option is supported*/
int sub_check(struct session*,char c);
/*record sub negotiation charactor*/
int get_char(struct session*,char c);
/*execute when recieved LM_MODE*/
int do_mode(struct session*,char c);
/*for LM_FORWARDMASK*/
int sub_d_notsupport(struct session*,char c);
/*not support LM_FORWARDMASK*/
int sub_w_notsupport(struct session*,char c);
/*
  open or close linemode
  on>0 open real linemode
  on=0 close real linemode
*/
void set_line_mode(struct session*,int on);
//void rcvurg(struct session*,int sig);
/*
  strip space charactors at head of buf
  buf:the string need to strip
  len:length of the string
  out:store the pointor pointing first charactor that is not a space charactor
*/
void fre_strip(char *buf,int len,char **out);
/*
  strip space charactors at end of buf
  buf:the string need to strip
  len:length of the string
  out:store the pointor pointing last charactor that is not a space charactor
*/
void  back_strip(char *buf,int len,char **out);
/*manage the pty*/
/*get pty linemode,return 1 if in linemode,0 if not*/
int tty_linemode (struct session*);
/*set pty linemode.on=1 open linemode,on=0 close linemode*/
void tty_setlinemode (struct session*,int on);
/*check whether pty is echoing,return 1 if it is,0 if not*/
int tty_isecho (struct session*);
/*set pty echo status.on=1 open pty echo,on=0 set pty not echo*/
void tty_setecho (struct session*,int on);
/*set pty binaryin.on=1 if pty permits client transferring 8 bit ascii char,otherwise on=0*/
void tty_binaryin (struct session*,int on);
/*set pty binaryout.on=1 if pty will transfer 8 bit ascii char,otherwise on=0*/
void tty_binaryout (struct session*,int on);
/*get pty binaryin status,return 1 if pty permits,0 otherwise*/
int tty_isbinaryin (struct session*);
/*get pty binaryout status,return 1 if pty will transfer 8 bit ascii,0 otherwise*/
int tty_isbinaryout (struct session*);
/*get pty edit status.return 1 if pty is editing,0 otherwise*/
int tty_isediting (struct session*);
/*get pty trapsig status.return 1 if pty is editing,0 otherwise*/
int tty_istrapsig (struct session*);
/*set pty edit status.on=1:permit client editing,on=0 pty will edit*/
void tty_setedit (struct session*,int on);
/*set pty trapsig status.on=1:permit client trapping signal,on=0 pty will trap signal*/
void tty_setsig (struct session*,int on);
/*get pty softtab status.return 1 if pty open,otherwise return 0*/
int tty_issofttab (struct session*);
/*set pty softtab status.on=1:permit client sending table char as some space charactors,on=0 otherwise*/
void tty_setsofttab (struct session*,int on);
/*set pty litecho status. on=1:permit client echoing charactors as what charactors really are,
on=0 transforms some special charactors like up, down,left...as ctrl+x*/
void tty_setlitecho (struct session*,int on);
/*get pty litecho status.return 1 if pty permits literature echo,otherwise return 0*/
int tty_islitecho (struct session*);
/*init pty*/
void init_term(struct session*);
/*get pty configuration*/
void get_term(struct session*);
/*set slcs*/
int spcset (cc_t[],int func, cc_t * valp,cc_t ** valpp);
/*
    dispose linemode option
	code:LINEMODE
	     parm=WILL: open linemode
		 parm=WONT: close linemode
	code:LM_MODE
	     parm: not used
*/
int client_state(struct session*,unsigned char code,char parm);
/*
  check server state,if the state server wishes is not the same as current state,negotiate with
  client
*/
void localstat (struct session*);
/*
  initiate the terminal after recieving some requirements from client
*/
void defer_terminit (struct session*);
/*add server current slc into slc buffer*/
void send_slc (struct session*);
/*send server default slc into slc buffer*/
void default_slc (struct session*);
/*set default slc*/
void get_slc_defaults (struct session*);
/*add a slc(function_name support_level value) to buffer pointed by *slcptr*/
void add_slc (unsigned char **slcptr,char func, char flag,cc_t val);
/*begin dispose slc*/
void start_slc (struct session*);
/*end dispose slc.if bufp is not NULL,*bufp will get slc buffer*/
int end_slc (struct session*,register unsigned char **bufp);
/*process a slc.process_slc will check current server slc and request slc,
change server slc if necessary,otherwis return not support this request slc or
 go on negotiating this slc*/
void process_slc (struct session*,unsigned char func, unsigned char flag,cc_t val);
/*
  change server's slc to <funcion support_level value>
  if server can't change,tell client not supporting this slc
*/
void change_slc (struct session*,char func, char flag, cc_t val);
/*check server's slc,if the slc server wishes is not same as slc in use,negotiate slc with client*/
void check_slc (struct session*);
/*manage the slcs buffered in ptr sent by client,length of buffer is len*/
void do_opt_slc (struct session*,register unsigned char *ptr, register int len);
/*dispose slc defered*/
void deferslc (struct session*);
/*check if charactor c is a delete charactor*/
int isdel(struct session*,char c);

#define net_in_rem(sp) ((sp)->net_inlen+(sp)->net_inbuffer-(sp)->net_inp)//check how much free buffer is left in net in buffer
#define cmd_in_clear(sp) (sp)->cmd_inlen=0;(sp)->cmd_inp=(sp)->cmd_inbuffer//clear the command in buffer
#define cmd_out_rem(sp) ((sp)->cmd_outend-(sp)->cmd_outbeg)//find how much free buffer is left in command out buffer
/*get client ip address in literature store in buffer buf length of len*/
char* peerip(struct session *sp,char *buf,int len);
/*setup io buffer*/
void io_setup (struct session*);
/*
  get len charators from net in buffer,return actual number of charactors got
  b:       will store charactors got from net in buffer
  len:     length of buffer b,or number of charactors wishing to get
  forward: 1 if caller wants to move read point,0 the pointer will not move
  return actual number of charactors got from net in buffer
*/
int net_get(struct session*,char *b,int len,int forward);
/*
 put format string in net out buffer
 format: format string
 return actual number of charactors sent 
*/
int net_put(struct session*,const char *format,...);
/*
   put len charactors pointed by src into net out buffer
   src: points string waiting to be sent
   len: length of the string
   return actual number of charactors sent
*/
int net_put_len(struct session*,char *src,int len);
/*flush out net buffer*/
void net_flush(struct session*);
/*set urgent charactor*/
void set_neturg (struct session*);
/*read from net,and put recieved charactors in net in buffer*/
int net_read (struct session*);
/*
 check whether net out buffer is full.
 return nonzero if net out buffer is full
*/
int net_out_full(struct session*);
/*
  find how much free space remaining in net out buffer
  return number of bytes in free in net out buffer
*/
int net_out_rem(struct session*);
/*
   check whether net in buffer is full
   return nozero if net in buffer is full
*/
int net_in_full(struct session*);
/*find how much free space remaining in command in buffer,
return number of bytes in free in command in buffer*/
int cmd_in_rem(struct session*);
/*
 put len charactors pointed by data in command out buffer
 data  :points to data that need to be sent to the client
 len   :the length of the data
 note: it is sugguested anyone who want to send some charactors  to client during executing command should
       call cmd_output or cmd_output_ln.after sending over,cmd_out_flush and net_flush should be called
*/
void cmd_output (struct session*,const char *data, int len);
/*
  like cmd_output,but add '\r\n' at the end
*/
void cmd_output_ln(struct session*,char *data,int len);
/*
  flush the command out buffer into net out buffer
  whenever sending charactors to client by calling cmd_output has been finished,call this function
*/
void cmd_out_flush (struct session*);
/*
  get len charactors store in b,recturn actual charactors got 
  b     :will store the charactors getting from command in buffer
  len   :number of char wish to get
  forward=1 if want to move read pointer
return actual number of charactors got from command in buffer  
*/
int cmd_get(struct session*,char *b,int len,int forward);
/*check whether command out buffer is full.return 1 if command out buffer is full*/
int cmd_out_full(struct session*);
/*read charactors from net,dispose these charactors*/
void io_drain (struct session*);
/*clear data in buf_beg from cmd_start,left only command*/
int buffer_clear (char *buf_beg,char *cmd_start,int len);
/*
  write log in log file
  format : the format string waiting to be wrote into log file
*/
void wrtlog(char *format,...);
/*like wrtlog,but add client's ip address and port before write log*/
void cwrtlog(struct session*,char *format,...);
/*initiate finite state machine*/
void init_fsm();
/*delete session*/
void clean(struct session*);
typedef void*(*func_t)(void*);
/*
  create a monitor thread,func_t is void* func_name(void *)
  func_t represent a function the new thread will run
  void* represents arguments transferred to func_t
  return 1 if success,0 if fail
*/
int create_monitor(struct session*,pthread_attr_t*,func_t,void*);
/*
  end monitor thread
*/
void end_monitor(struct session*);
/*
  get current session representing current client.
  note: in monitor thread, cann't call this function because the monitor thread is a new thread
*/
struct session* get_session();
/*
  create a session
*/
struct session* create_session();
/*
 get column of client's screen
 return colomn of client's screen
 if the client does not support naws option,
 return default column 80
*/
int get_linesize(struct session *sp);

/** telnet service*/
int telnet_service(char *ip, int port_number);

int continue_disp(struct session *sp);
#ifdef __cplusplus
}
#endif

#endif

