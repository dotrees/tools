#include "telnet_server.h"

struct fsm trans[]={
	/*state       iput       next_state     action*/
    {SDATA,       IAC,      SIAC,           0},//1
	//{SDATA,       CR,       SDATA,          do_cr},//2
	{SDATA,       CANY,     SDATA,          coutput},//3
	{SIAC,        IAC,      SDATA,          coutput},//4
	{SIAC,        SB,       SSUBNEG,        0},//5

	{SIAC,        NOP,      SDATA,          0},//6
	{SIAC,        DM,       SDATA,          datamark},//7
	{SIAC,        EC,       SDATA,          0/*del_char*/},//8
	{SIAC,        EL,       SDATA,          0/*del_line*/},//9
	{SIAC,        IP,       SDATA,          do_ip},//10
	{SIAC,        WILL,     SWOPT,          recopt},//11
	{SIAC,        WONT,     SWOPT,          recopt},//12
	{SIAC,        DO,       SDOPT,          recopt},//13
	{SIAC,        DONT,     SDOPT,          recopt},//14
	{SIAC,        CANY,     SDATA,          0},//15

	{SSUBNEG,     IAC,      SSUBIAC,        0},//16
	{SSUBNEG,     CANY,     SSUBNEG,        subopt},//17
	{SSUBIAC,     SE,       SDATA,          subend},//18
	{SSUBIAC,     CANY,     SSUBNEG,        subopt},//19

	{SWOPT,       TELOPT_ECHO,     SDATA,   w_echo},//20
	{SWOPT,       TELOPT_LINEMODE, SDATA,   w_linemode},//21
	{SWOPT,       TELOPT_BINARY,   SDATA,   w_binary},//22
	{SWOPT,       TELOPT_TTYPE,    SDATA,   w_termtype},//23
	{SWOPT,       TELOPT_LOGOUT,   SDATA,   w_exit},//
	{SWOPT,       CANY,            SDATA,   w_notsupport},//24
	{SWOPT,       TELOPT_TM,       SDATA,   w_timemark},//25
    {SWOPT,       TELOPT_NAWS,     SDATA,   w_naws},//26
	
	{SDOPT,       TELOPT_ECHO,     SDATA,   d_echo},//27
	{SDOPT,       TELOPT_LINEMODE, SDATA,   d_linemode},//28
	{SDOPT,       TELOPT_BINARY,   SDATA,   d_binary},//29
	{SDOPT,       CANY,            SDATA,   d_notsupport},//30
	{SDOPT,       TELOPT_SGA,      SDATA,   d_sga},//31
	//{SDOPT,       TELOPT_TTYPE,    SDATA,   d_termtype},
	{SDOPT,       TELOPT_LOGOUT,   SDATA,   d_exit},//32
	{SDOPT,       TELOPT_TM,       SDATA,   d_timemark},//33
   
	{SINVALID,    CANY,            SINVALID,myabort},//34
};
struct fsm subtrans[]={
	/*state            input            next state            action*/
	{SSTART,           TELOPT_TTYPE,   SSTERMTYPE,            sub_check},//1
	{SSTART,           TELOPT_LINEMODE,SSLINEMODE,            sub_check},
	{SSTART,           TELOPT_NAWS,    SSNAWS,                sub_check},
	{SSTART,           CANY,           SSEND,                 0},//4
	{SSLINEMODE,       LM_MODE,        SSLMODE,               sub_recopt},
	{SSLMODE,          CANY,           SSEND,                 do_mode},
	{SSLINEMODE,       LM_SLC,         SSGETSLC,              sub_recopt},
	{SSGETSLC,         CANY,           SSGETSLC,              get_char},//8
	{SSLINEMODE,       DO,             SSLMDO,                sub_recopt},
	{SSLINEMODE,       WILL,           SSLMWILL,              sub_recopt},//10
	{SSLINEMODE,       DONT,           SSLMDO,                sub_recopt},
	{SSLINEMODE,       WONT,           SSLMWILL,              sub_recopt},//12
	{SSLMDO,           CANY,           SSEND,                 sub_d_notsupport},//
	{SSLMWILL,         CANY,           SSEND,                 sub_w_notsupport},
	{SSLINEMODE,       CANY,           SSEND,                 0},//15
	{SSTERMTYPE,       TELQUAL_IS,     SSGETTERM,             sub_recopt},//16
	{SSGETTERM,        CANY,           SSGETTERM,             get_char},
	{SSTERMTYPE,       CANY,           SSEND,                 0},//17
	{SSNAWS,           CANY,           SSNAWS,                get_char},//19
	{SSEND,            CANY,           SSEND,                 0},
	{SINVALID,         CANY,           SINVALID,              myabort},//21
};
#define NTRANS sizeof(trans)/sizeof(struct fsm)
#define NSUBTRANS sizeof(subtrans)/sizeof(struct fsm)
unsigned char trans_table[NSTATES][NCHARS];
unsigned char subtrans_table[NSSTATES][NCHARS];
