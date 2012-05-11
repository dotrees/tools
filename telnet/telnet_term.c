#include "telnet_server.h"
int tty_isecho (struct session* ssp)
{
#ifdef TERM_DESC
    return ssp->termbuf.term_param & TERM_ECHO;
#else
    return ssp->termbuf.c_lflag & ECHO;
#endif
}
void tty_setecho (struct session *ssp,int on)
{
#ifdef TERM_DESC
    ssp->termbuf.term_param= on>0 ? ssp->termbuf.term_param|TERM_ECHO : ssp->termbuf.term_param&(~TERM_ECHO);
#else
    if (on)
        ssp->termbuf.c_lflag |= ECHO;
    else
        ssp->termbuf.c_lflag &= ~ECHO;
#endif
}
void tty_binaryin (struct session* ssp,int on)
{
#ifdef TERM_DESC
    ssp->termbuf.term_param= (on>0 ? 
    ssp->termbuf.term_param|TERM_BINARYIN : ssp->termbuf.term_param&(~TERM_BINARYIN));
#else
    if (on)
        ssp->termbuf.c_iflag &= ~ISTRIP;
    else
        ssp->termbuf.c_iflag |= ISTRIP;
#endif
}
void tty_binaryout (struct session* ssp,int on)
{
#ifdef TERM_DESC
    ssp->termbuf.term_param= (on>0 ? 
    ssp->termbuf.term_param|TERM_BINARYOUT : ssp->termbuf.term_param&(~TERM_BINARYOUT));
#else
    if (on)
    {
        ssp->termbuf.c_cflag &= ~(CSIZE | PARENB);
        ssp->termbuf.c_cflag |= CS8;
        ssp->termbuf.c_oflag &= ~OPOST;
    }
    else
    {
        ssp->termbuf.c_cflag &= ~CSIZE;
        ssp->termbuf.c_cflag |= CS7 | PARENB;
        ssp->termbuf.c_oflag |= OPOST;
    }
#endif
}
int tty_isbinaryin (struct session *ssp)
{
#ifdef TERM_DESC
    return ssp->termbuf.term_param & TERM_BINARYIN;
#else
    return !(ssp->termbuf.c_iflag & ISTRIP);
#endif
}
int tty_isbinaryout (struct session *ssp)
{
#ifdef TERM_DESC
    return ssp->termbuf.term_param& TERM_BINARYOUT;
#else
    return !(ssp->termbuf.c_oflag & OPOST);
#endif
}
int tty_isediting (struct session *ssp)
{
#ifdef TERM_DESC
    return ssp->termbuf.term_param&TERM_EDIT;
#else
    return ssp->termbuf.c_lflag & ICANON;
#endif
}
int tty_istrapsig (struct session* ssp)
{
#ifdef TERM_DESC
    return ssp->termbuf.term_param & TERM_TRAPSIG;
#else
    return ssp->termbuf.c_lflag & ISIG;
#endif
}
void tty_setedit (struct session *ssp,int on)
{
#ifdef TERM_DESC
    ssp->termbuf.term_param=(on>0 ? 
    ssp->termbuf.term_param|TERM_EDIT : ssp->termbuf.term_param &(~TERM_EDIT));
#else
    if (on)
        ssp->termbuf.c_lflag |= ICANON;
    else
        ssp->termbuf.c_lflag &= ~ICANON;
#endif
}
void tty_setsig (struct session* ssp,int on)
{
#ifdef TERM_DESC
    ssp->termbuf.term_param=(on>0 ? 
    ssp->termbuf.term_param|TERM_TRAPSIG : ssp->termbuf.term_param&(~TERM_TRAPSIG));
#else
    if (on)
        ssp->termbuf.c_lflag |= ISIG;
    else
        ssp->termbuf.c_lflag &= ~ISIG;
#endif
}
int tty_issofttab (struct session* ssp)
{
#ifdef TERM_DESC
    return ssp->termbuf.term_param&TERM_SOFTTAB;
# else
#  ifdef	OXTABS
    return ssp->termbuf.c_oflag & OXTABS;
#  endif
#  ifdef	TABDLY
    return (ssp->termbuf.c_oflag & TABDLY) == TAB3;
# endif
#endif
}
void tty_setsofttab (struct session* ssp,int on)
{
#ifdef TERM_DESC
    ssp->termbuf.term_param=(on>0 ? 
    ssp->termbuf.term_param|TERM_SOFTTAB : ssp->termbuf.term_param&~TERM_SOFTTAB);
#else
    if (on)
    {
# ifdef	OXTABS
        ssp->termbuf.c_oflag |= OXTABS;
# endif
# ifdef	TABDLY
        ssp->termbuf.c_oflag &= ~TABDLY;
        ssp->termbuf.c_oflag |= TAB3;
# endif
    }
    else
    {
# ifdef	OXTABS
        ssp->termbuf.c_oflag &= ~OXTABS;
# endif
# ifdef	TABDLY
        ssp->termbuf.c_oflag &= ~TABDLY;
        ssp->termbuf.c_oflag |= TAB0;
# endif
    }
#endif
}
void tty_setlitecho (struct session *sp,int on)
{
#ifdef TERM_DESC
    sp->termbuf.term_param=(on>0 ? 
    sp->termbuf.term_param|TERM_LITECHO :sp->termbuf.term_param&(~TERM_LITECHO));
#else
# ifdef	ECHOCTL
    if (on)
        sp->termbuf.c_lflag &= ~ECHOCTL;
    else
        sp->termbuf.c_lflag |= ECHOCTL;
# endif
# ifdef	TCTLECH
    if (on)
        sp->termbuf.c_lflag &= ~TCTLECH;
    else
        sp->termbuf.c_lflag |= TCTLECH;
# endif
#endif
}
int tty_islitecho (struct session* sp)
{
#ifdef TERM_DESC
    return sp->termbuf.term_param&TERM_LITECHO;
#else
# ifdef	ECHOCTL
    return !(sp->termbuf.c_lflag & ECHOCTL);
# endif
# ifdef	TCTLECH
    return !(sp->termbuf.c_lflag & TCTLECH);
# endif
# if !defined(ECHOCTL) && !defined(TCTLECH)
    return 0;			/* assumes ctl chars are echoed '~x' */
# endif
#endif
}
int tty_linemode (struct session* sp)
{
#ifdef TERM_DESC
    return sp->termbuf.term_param&TERM_LINEMODE;
#else
# ifdef EXTPROC
   return (sp->termbuf.c_lflag & EXTPROC);
# else
    return 0;			/* Can't ever set it either. */
# endif	/* EXTPROC */
#endif
}
void tty_setlinemode (struct session *sp,int on)
{
#ifdef TERM_DESC
    sp->termbuf.term_param=(on>0 ? sp->termbuf.term_param|TERM_LINEMODE : sp->termbuf.term_param&(~TERM_LINEMODE));
#else
#  ifdef	EXTPROC
    if (on)
        sp->termbuf.c_lflag |= EXTPROC;
    else
        sp->termbuf.c_lflag &= ~EXTPROC;
#  endif
#endif
}
void get_term(struct session *sp)
{
#ifdef TERM_DESC
    struct termios termbuf;
    tcgetattr(1,&termbuf);
    memcpy(sp->termbuf.cc_c,termbuf.c_cc,sizeof(termbuf.c_cc));
    sp->termbuf.term_param=0;
#else
    tcgetattr(1, &sp->termbuf);
#endif
}
#define setval(a, b)	*valp = a ; \
			*valpp = &a ; \
			return b;
#define defval(a)       *valp = ((cc_t)a); \
                        *valpp = (cc_t *)0; \
			return SLC_DEFAULT;
int spcset (cc_t c_cc[NCCS],int func, cc_t * valp,cc_t ** valpp)
{
    switch (func)
    {
        case SLC_EOF:
            setval (c_cc[VEOF], SLC_VARIABLE);

        case SLC_EC:
            setval (c_cc[VERASE], SLC_VARIABLE);

        case SLC_EL:
            setval (c_cc[VKILL], SLC_VARIABLE);

        case SLC_IP:
            setval (c_cc[VINTR], SLC_VARIABLE | SLC_FLUSHIN | SLC_FLUSHOUT);

        case SLC_ABORT:
            setval (c_cc[VQUIT], SLC_VARIABLE | SLC_FLUSHIN | SLC_FLUSHOUT);

        case SLC_XON:
#ifdef VSTART
            setval (c_cc[VSTART], SLC_VARIABLE);
#else
            defval (0x13);
#endif

        case SLC_XOFF:
#ifdef VSTOP
             setval (c_cc[VSTOP], SLC_VARIABLE);
#else
      defval (0x11);
#endif

        case SLC_EW:
#ifdef VWERASE
            setval (c_cc[VWERASE], SLC_VARIABLE);
#else
            defval (0);
#endif

        case SLC_RP:
#ifdef VREPRINT
            setval (c_cc[VREPRINT], SLC_VARIABLE);
#else
            defval (0);
#endif

        case SLC_LNEXT:
#ifdef VLNEXT
            setval (c_cc[VLNEXT], SLC_VARIABLE);
#else
            defval (0);
#endif

        case SLC_AO:
#ifdef VDISCARD
            setval (c_cc[VDISCARD], SLC_VARIABLE | SLC_FLUSHOUT);
#else
            defval (0);
#endif

        case SLC_SUSP:
#ifdef VSUSP
            setval (c_cc[VSUSP], SLC_VARIABLE | SLC_FLUSHIN);
#else
            defval (0);
#endif

#ifdef VEOL
        case SLC_FORW1:
            setval (c_cc[VEOL], SLC_VARIABLE);
#endif

#ifdef VEOL2
        case SLC_FORW2:
            setval (c_cc[VEOL2], SLC_VARIABLE);
#endif

        case SLC_AYT:
#ifdef VSTATUS
            setval (c_cc[VSTATUS], SLC_VARIABLE);
#else
            defval (0);
#endif

        case SLC_BRK:
        case SLC_SYNCH:
        case SLC_EOR:
            defval (0);

        default:
            *valp = 0;
            *valpp = 0;
             return SLC_NOSUPPORT;
    }
}
int term_change_eof (struct session* sp)
{
# if VEOF == VMIN
    if (!tty_isediting ())
        return 1;
    if (sp->slcs[SLC_EOF].src)
        oldeofc = *sp->slcs[SLC_EOF].src;
    return 0;
# endif
}
void init_term(struct session* sp)
{
    get_term(sp);
    if(sp->max_telnet_mode>NO_LINEMODE)
    {
        tty_setlinemode(sp,1);
	    tty_setecho(sp,0);
	}
	else
	{
	    tty_setlinemode(sp,0);
	    tty_setecho(sp,1);
	}
    tty_binaryin(sp,1);
    tty_binaryout(sp,1);
    tty_setedit(sp,1);
    tty_setsig(sp,1);
    tty_setlitecho(sp,1);
}
