#include "ring_buffer.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>

typedef struct{
	int a;
	int b;
} POINT;

typedef POINT* PPOINT;

PRING_BUFFER pbuf = NULL;
PPOINT p_point = NULL;

struct timeval tv1;
struct timeval tv2;
int interval;
int count = 0;

#define SEM
#define DELAY

static void sig_handler(int signo)
{
	switch(signo)
	{
	    case SIGINT:
	        fprintf(stdout, "AP sig_handler SIGINT\n");
		#ifndef DELAY
			gettimeofday(&tv2, NULL);
			interval = (tv2.tv_sec - tv1.tv_sec)*1000000 + tv2.tv_usec - tv1.tv_usec;
			printf("interval = %d\n", interval);
			printf("count = %d\n", count);
			printf("%.2f pps\n", count*1000000.0/interval);
		#endif
	        exit(1);
	    default:
	        break;
	}
}

void *thread_function()
{
	while(1)
	{
		RBEV_READ(ev, pbuf);

		#ifdef DELAY
		gettimeofday(&tv2, NULL);
		interval = tv2.tv_usec - tv1.tv_usec;
		printf("interval = %d\n", interval);		/* this is delay */
		#endif

		p_point = &rb_readPeek(pbuf, POINT);
		rb_readOutValue(pbuf, POINT, *p_point);
		count++;
	}
	
	pthread_exit(NULL);
}

int main()
{
	int ret;
	pthread_t tid;
	ev.waiting = 0;

	/* signal */
	if(signal(SIGINT, sig_handler) == SIG_ERR)
	{   
	    fprintf(stderr, "unable create handle for SIGINT [%s: %d]", __FILE__, __LINE__);
	    exit(0);
	}

	/* rb_create */
	rb_create(10000, sizeof(POINT), pbuf);
	
	/* deal with thread */
	if(pthread_create(&tid,NULL,thread_function,NULL))
	{
		perror("cannot create new thread");
		return -1;
	}
	if(pthread_detach(tid) != 0)
	{
		perror("cannot detach thread");
		return -1;
	}

	/* sem_init */
	RBEV_INIT(ev);

	gettimeofday(&tv1, NULL);
	while(1)
	{
		if(!rb_canWrite(pbuf))
			continue;

		/* write in value */
		p_point = &rb_writePeek(pbuf, POINT);
		p_point->a = rand()%100;
		p_point->b = rand()%100;
		rb_writeInValue(pbuf, POINT, *p_point);
	
		/* RBEV_WRITE */
		RBEV_WRITE(ev);
	#ifdef DELAY
		gettimeofday(&tv1, NULL);
		sleep(1);
	#endif
	}
	
	return 0;
}

