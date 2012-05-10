#include "ring_buffer.h"
#include <stdio.h>
#include <pthread.h>

typedef struct{
	int a;
	int b;
} POINT;

typedef POINT* PPOINT;

PRING_BUFFER pbuf = NULL;
PPOINT p_point = NULL;

void *thread_function()
{
	while(1){
	//	usleep(rand()%1000 * 1000);
		if(!rb_canRead(pbuf))
		{
			usleep(100);
			continue;
		}
		p_point = &rb_readPeek(pbuf, POINT);
		rb_readOutValue(pbuf, POINT, *p_point);
		printf("a=%u\n", p_point->a);
	}
	
	pthread_exit(NULL);
}

int main()
{
	int ret;
	int mem_req;
	int rs = 100;
	pthread_t thread_id;
	
	// rb_memory_required
	mem_req = rb_memory_required(rs, sizeof(POINT));
	printf("mem_req: %d\n", mem_req);
	
	// rb_create
	ret = rb_create(rs, sizeof(POINT), pbuf);
	if(!ret){
		printf("rb_create fail\n");
		return -1;
	}
	printf("0x%x\n", pbuf);
	
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
	
	// rb_canWrite
	ret = 0;
	while(1){
		usleep(rand()%1000 * 100);
		p_point = &rb_writePeek(pbuf, POINT);
		p_point->a = time(NULL);
		rb_writeInValue(pbuf, POINT, *p_point);
		printf("%d\n", pbuf->wp);
	}
	
	return 0;
}
