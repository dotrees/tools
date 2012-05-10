#include "ring_buffer.h"
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <sys/time.h>
#include <semaphore.h>
#include <signal.h>
#include <stdlib.h>
#include "bitops.h"

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

typedef struct {
	sem_t rb_sem;
	volatile int waiting;
} RBEVENT;

RBEVENT ev;

#define RBEV_INIT(ev) {sem_init(&((ev).rb_sem), 0, 0); (ev).waiting = 0;}
#define RBEV_WAIT(ev) {sem_wait(&((ev).rb_sem));}
#define RBEV_WAKE(ev) {sem_post(&((ev).rb_sem));}

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
	int old_value;
	int c = 0;

	while(1)
	{
		if(!rb_canRead(pbuf))
		{
			// 等待标志位置位，并判断old_value
			// 1. old_value == 0 说明等待标志位没有被置位
			// 2. old_value != 0 说明等待标志位已被置位
		/*	old_value = test_and_set_bit(0, &(ev.waiting));
			if(old_value == 0)
			{
				// 需要等待信号到来
				sem_wait(&(ev.rb_sem));
				printf("do sem_wait\n");
				continue;
			}
			else
			{
				// 
				continue;
			}*/
			
			if(c++ < 50)
			{
				sched_yield();
				continue;
			}

			ev.waiting = 1;
			sem_wait(&(ev.rb_sem));
		}
		else
		{
			ev.waiting = 0;
		}
		c = 0;
	/*	{
		#ifdef USLEEP
			usleep(1);
			continue;
		#else
			sem_wait(&bin_sem);
		#endif
		}*/

		#ifdef DELAY
		gettimeofday(&tv2, NULL);
		interval = tv2.tv_usec - tv1.tv_usec;
		printf("interval = %d\n", interval);
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
	int rs = 10000;
	pthread_t thread_id;
	ev.waiting = 0;
	
	if(signal(SIGINT, sig_handler) == SIG_ERR)
	{   
	    fprintf(stderr, "unable create handle for SIGINT [%s: %d]", __FILE__, __LINE__);
	    exit(0);
	}

	// rb_create
	rb_create(rs, sizeof(POINT), pbuf);
	
	// deal with thread
	if(pthread_create(&thread_id,NULL,thread_function,NULL))
	{
		perror("cannot create new thread");
		return -1;
	}
	if(pthread_detach(thread_id) != 0)
	{
		perror("cannot detach thread");
		return -1;
	}

	// RBEV_INIT
	RBEV_INIT(ev);

	// rb_canWrite
	gettimeofday(&tv1, NULL);
	while(1)
	{
		if(!rb_canWrite(pbuf))
		{
		//	printf("rb_canWrite fail\n");
			continue;
		}
		p_point = &rb_writePeek(pbuf, POINT);
		p_point->a = rand()%100;
		p_point->b = rand()%100;

		rb_writeInValue(pbuf, POINT, *p_point);
		
		if(ev.waiting == 1)
		{
			sem_post(&(ev.rb_sem));
			printf("do sem_post\n");
		}
	/*	if(test_and_clear_bit(0, &(ev.waiting)) != 0)		// 如果等待标识位置位，则唤醒
		{
			printf("do sem_post\n");
			sem_post(&(ev.rb_sem));
		}*/

	#ifdef DELAY
		gettimeofday(&tv1, NULL);
		sleep(1);
	#endif
	//	sleep(1);
	}
	
	return 0;
}

