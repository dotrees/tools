
#include "telnet_server.h"
/*
 * local variables
 */
//static unsigned char *def_slcbuf = (unsigned char *) 0;
//static int def_slclen = 0;
//static int slcchange;		/* change to slc is requested */
//static unsigned char *slcptr;	/* pointer into slc buffer */
//static unsigned char slcbuf[NSLC * 6];	/* buffer for slc negotiation */
/*
 * send_slc
 *
 * Write out the current special characters to the client.
 */
void send_slc (struct session *sp)
{
   register int i;
  /*
   * Send out list of triplets of special characters
   * to client.  We only send info on the characters
   * that are currently supported.
   */
    for (i = 1; i <= NSLC; i++)
    {
        if ((sp->slcs[i].sysflag & SLC_LEVELBITS) == SLC_NOSUPPORT)
	        continue;
        add_slc (&sp->slcptr,(unsigned char) i, sp->slcs[i].curflag,sp->slcs[i].curval);
    }
}				/* end of send_slc */

/*
 * default_slc
 *
 * Set pty special characters to all the defaults.
 */
void default_slc (struct session *sp)
{
    int i;
    for (i = 1; i <= NSLC; i++)
    {
        sp->slcs[i].curval = sp->slcs[i].sysval;
        if (sp->slcs[i].curval == (cc_t) (_POSIX_VDISABLE))
	        sp->slcs[i].curflag = SLC_NOSUPPORT;
        else
	        sp->slcs[i].curflag = sp->slcs[i].sysflag;
        if (sp->slcs[i].src)
	    {
	        *(sp->slcs[i].src) = sp->slcs[i].sysval;
	    }
    }
     //sp->slcchange = 1;

}				/* end of default_slc */

/*
 * get_slc_defaults
 *
 * Initialize the slc mapping table.
 */
void get_slc_defaults (struct session* sp)
{
    int i;
    for (i = 1; i <= NSLC; i++)
    {
        sp->slcs[i].sysflag =spcset (sp->termbuf.cc_c,i, &(sp->slcs[i].sysval), &(sp->slcs[i].src));
        sp->slcs[i].curflag = SLC_NOSUPPORT;
        sp->slcs[i].curval = 0;
    }
}				/* end of get_slc_defaults */

/*
 * add_slc
 *
 * Add an slc triplet to the slc buffer.
 */
void add_slc (unsigned char **slcptr,char func, char flag,cc_t val)
{
//printf("in add slc\n");
    if ((*((*slcptr)++) = (unsigned char) func) == 0xff)
        *((*slcptr)++) = 0xff;

    if ((*((*slcptr)++) = (unsigned char) flag) == 0xff)
        *((*slcptr)++) = 0xff;

    if ((*((*slcptr)++) = (unsigned char) val) == 0xff)
        *((*slcptr)++) = 0xff;
	//printf("in add slc slc len=%d\n",slcptr-slcbuf);

}				/* end of add_slc */

/*
 * start_slc
 *
 * Get ready to process incoming slc's and respond to them.
 *
 * The parameter getit is non-zero if it is necessary to grab a copy
 * of the terminal control structures.
 */
void start_slc (struct session *sp)
{
//printf("in start slc\n");
    sprintf ((char *) sp->slcbuf, "%c%c%c%c", IAC, SB, TELOPT_LINEMODE, LM_SLC);
    sp->slcptr = sp->slcbuf + 4;
}				/* end of start_slc */

/*
 * end_slc
 *
 * Finish up the slc negotiation.  If something to send, then send it.
 */
int end_slc (struct session *sp,register unsigned char **bufp)
{
//printf("in end_slc\n");
    int len,i;
    net_flush (sp);

  /*
   * If the pty state has not yet been fully processed and there is a
   * deferred slc request from the client, then do not send any
   * sort of slc negotiation now.  We will respond to the client's
   * request very soon.
   */
    if (sp->def_slcbuf && (terminit (sp) == 0))
    {
  //printf("terminit=%d\n",terminit());
        return (0);
    }
    if (sp->slcptr > (sp->slcbuf + 4))
    {
  //printf("slc len=%d\n",slcptr-slcbuf);
        if (bufp)//get the slc set
	    {
	        *bufp = &sp->slcbuf[4];
	        return (sp->slcptr -sp->slcbuf - 4);
	    }
        else
	    {
	        sprintf ((char *)sp->slcptr, "%c%c%c", IAC, SE,0);
		    //printf("in end slc len=%d\n",slcptr-slcbuf+2);
	        net_put_len(sp,sp->slcbuf,sp->slcptr-sp->slcbuf+2);
		  
	        net_flush (sp);		/* force it out immediately */	
	    }
    }
	if(sp->slcchange)
	{
	    for(i=1;i<=NSLC;i++)
	    {
	        if(sp->slcs[i].src)
		        *(sp->slcs[i].src)=sp->slcs[i].curval;
	    }
	}
    return (0);
}				/* end of end_slc */

/*
 * process_slc
 *
 * Figure out what to do about the client's slc
 */
void process_slc (struct session* sp,unsigned char func, unsigned char flag,cc_t val)
{
//printf("slc process %s %d %d\n",SLC_NAME(func),flag, val);
    int hislevel, mylevel, ack;
  /*
   * Ensure that we know something about this function
   */
    if (func > NSLC)
    {
        add_slc (&sp->slcptr,func, SLC_NOSUPPORT, 0);
        return;
    }

  /*
   * Process the special case requests of 0 SLC_DEFAULT 0
   * and 0 SLC_VARIABLE 0.  Be a little forgiving here, don't
   * worry about whether the value is actually 0 or not.
   */
    if (func == 0)
    {
        if ((flag = flag & SLC_LEVELBITS) == SLC_DEFAULT)
	    {
	        default_slc (sp);
	        send_slc (sp);
	    }
        else if (flag == SLC_VARIABLE)
	    {
	        send_slc (sp);
	    }
        return;
    }

  /*
   * Appears to be a function that we know something about.  So
   * get on with it and see what we know.
   */
    hislevel = flag & SLC_LEVELBITS;
    mylevel = sp->slcs[func].curflag & SLC_LEVELBITS;
    ack = flag & SLC_ACK;
  /*
   * ignore the command if:
   * the function value and level are the same as what we already have;
   * or the level is the same and the ack bit is set
   */
    if (hislevel == mylevel && (val == sp->slcs[func].curval || ack))
    {
  //printf("same level :%s hislevel=%d var=%d\n",SLC_NAME(func),hislevel,val);
        return;
    }
    else if (ack)
    {
      /*
       * If we get here, we got an ack, but the levels don't match.
       * This shouldn't happen.  If it does, it is probably because
       * we have sent two requests to set a variable without getting
       * a response between them, and this is the first response.
       * So, ignore it, and wait for the next response.
       */
	  // printf("bad slc will ignore it\n\r");
        return;
    }
    else
    {
        change_slc (sp,func, flag, val);
    }

}				/* end of process_slc */

/*
 * change_slc
 *
 * Process a request to change one of our special characters.
 * Compare client's request with what we are capable of supporting.
 */
void change_slc (struct session* sp,char func, char flag, cc_t val)
{
//printf("change %s\n",SLC_NAME(func));
    int hislevel, mylevel;
    hislevel = flag & SLC_LEVELBITS;
    mylevel = sp->slcs[func&0xff].sysflag & SLC_LEVELBITS;
  /*
   * If client is setting a function to NOSUPPORT
   * or DEFAULT, then we can easily and directly
   * accomodate the request.
   */
    if (hislevel == SLC_NOSUPPORT)
    {
   //printf("not support\n");
        sp->slcs[func&0xff].curflag = flag;
        sp->slcs[func&0xff].curval = (cc_t) _POSIX_VDISABLE;
        flag |= SLC_ACK;
        add_slc (&sp->slcptr,func, flag, val);
        return;
    }
    if (hislevel == SLC_DEFAULT)
    {
      /*
       * Special case here.  If client tells us to use
       * the default on a function we don't support, then
       * return NOSUPPORT instead of what we may have as a
       * default level of DEFAULT.
       */
        if (mylevel == SLC_DEFAULT)
	    {
	        sp->slcs[func&0xff].curflag = SLC_NOSUPPORT;
			//printf("not support\n");
	    }
        else
	    {
	        sp->slcs[func&0xff].curflag = sp->slcs[func&0xff].sysflag;
			//printf("default\n");
	    }
        sp->slcs[func&0xff].curval = sp->slcs[func&0xff].sysval;
	  //printf("add before\n");
        add_slc (&sp->slcptr,func, sp->slcs[func&0xff].curflag, sp->slcs[func&0xff].curval);
	 // printf("slc add %s %d %d\n",SLC_NAME(func),sp->slcs[func&0xff].curflag, sp->slcs[func&0xff].curval);
        return;
    }
  /*
   * Client wants us to change to a new value or he
   * is telling us that he can't change to our value.
   * Some of the slc's we support and can change,
   * some we do support but can't change,
   * and others we don't support at all.
   * If we can change it then we have a pointer to
   * the place to put the new value, so change it,
   * otherwise, continue the negotiation.
   */
    if (sp->slcs[func&0xff].src)
    {
      /*
       * We can change this one.
       */
        sp->slcs[func&0xff].curval = val;
        *(sp->slcs[func&0xff].src) = val;
        sp->slcs[func&0xff].curflag = flag;
        flag |= SLC_ACK;
        sp->slcchange = 1;
	  //printf("add before\n");
        add_slc (&sp->slcptr,func, flag, val);
    }
    else
    {
      /*
       * It is not possible for us to support this
       * request as he asks.
       *
       * If our level is DEFAULT, then just ack whatever was
       * sent.
       *
       * If he can't change and we can't change,
       * then degenerate to NOSUPPORT.
       *
       * Otherwise we send our level back to him, (CANTCHANGE
       * or NOSUPPORT) and if CANTCHANGE, send
       * our value as well.
       */
        if (mylevel == SLC_DEFAULT)
	    {
	        sp->slcs[func&0xff].curflag = flag;
	        sp->slcs[func&0xff].curval = val;
	        flag |= SLC_ACK;
	    }
        else if (hislevel == SLC_CANTCHANGE && mylevel == SLC_CANTCHANGE)
	    {
	        flag &= ~SLC_LEVELBITS;
	        flag |= SLC_NOSUPPORT;
	        sp->slcs[func&0xff].curflag = flag;
	    }
        else
	    {
	        flag &= ~SLC_LEVELBITS;
	        flag |= mylevel;
	        sp->slcs[func&0xff].curflag = flag;
	        if (mylevel == SLC_CANTCHANGE)
	        {
	            sp->slcs[func&0xff].curval = sp->slcs[func&0xff].sysval;
	            val = sp->slcs[func&0xff].curval;
	        }
	    }
	   //printf("add before\n");
        add_slc (&sp->slcptr,func, flag, val);
	  // printf("slc add %s %d %d\n",SLC_NAME(func),flag, val);
    }
}				/* end of change_slc */

/*
 * check_slc
 *
 * Check the special characters in use and notify the client if any have
 * changed.  Only those characters that are capable of being changed are
 * likely to have changed.  If a local change occurs, kick the support level
 * and flags up to the defaults.
 */
void check_slc (struct session *sp)
{
//printf("in check slc\n\r");
    int i;
    for (i = 1; i <= NSLC; i++)
    {
        if (i == SLC_EOF && term_change_eof (sp))
	        continue;
        if (sp->slcs[i].sysval != sp->slcs[i].curval)
	    {
	        sp->slcs[i].curval = sp->slcs[i].sysval;
	        if (sp->slcs[i].sysval == (cc_t) _POSIX_VDISABLE)
	            sp->slcs[i].curflag = SLC_NOSUPPORT;
	        else
	            sp->slcs[i].curflag = sp->slcs[i].sysflag;
	        add_slc (&sp->slcptr,(unsigned char) i, sp->slcs[i].curflag,sp->slcs[i].curval);
	    }
    }
}				/* check_slc */
/*
 * do_opt_slc
 *
 * Process an slc option buffer.  Defer processing of incoming slc's
 * until after the terminal state has been processed.  Save the first slc
 * request that comes along, but discard all others.
 *
 * ptr points to the beginning of the buffer, len is the length.
 */
void do_opt_slc (struct session* sp,register unsigned char *ptr, register int len)
{
//printf("in do_opt_slc\n");
    unsigned char func, flag;
    cc_t val;
    unsigned char *end = ptr + len;
    if (sp->_terminit)
    {				/* go ahead */
  //printf("has terminit\n");
        while (ptr < end)
	    {
	        func = *ptr++;
	        if (ptr >= end)
	            break;
	        flag = *ptr++;
	        if (ptr >= end)
	            break;
	        val = (cc_t) * ptr++;
	        process_slc (sp,func, flag, val);
	    }
    }
    else
    {
      /*
       * save this slc buffer if it is the first, otherwise dump
       * it.
       */
        if (sp->def_slcbuf == (unsigned char *) 0)
	   {
	        sp->def_slclen = len;
	        sp->def_slcbuf = (unsigned char *) malloc ((unsigned) len);
	        if (sp->def_slcbuf == (unsigned char *) 0)
	            return;		/* too bad */
	        memmove (sp->def_slcbuf, ptr, len);
	    }
    }
}				/* end of do_opt_slc */

/*
 * deferslc
 *
 * Do slc stuff that was deferred.
 */
void deferslc (struct session *sp)
{
//printf("in deferslc\n");
    if (sp->def_slcbuf)
    {
        start_slc (sp);
        do_opt_slc (sp,sp->def_slcbuf, sp->def_slclen);
        end_slc (sp,0);
        free (sp->def_slcbuf);
        sp->def_slcbuf = (unsigned char *) 0;
        sp->def_slclen = 0;
   }
}				/* end of deferslc */
int isdel(struct session *sp,char c)
{
    if(sp->telnet_mode<=KLUDGE_LINEMODE)
        return c==8||c==127;
    return sp->slcs[SLC_EC].curflag==SLC_NOSUPPORT?(c==8||c==127):(c==sp->slcs[SLC_EC].curval||c==8);
}
