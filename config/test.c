#include <stdio.h>


#define CONF_SAVE(w,f)  do { \
	    char *p = f; \
	    if (p != NULL) \
	    (w) = p; \
} while (0)


int main()
{
	int port;
	char *file_path = "./my.conf";
	char *sid;

	conf_init(file_path);

	port = conf_get_num("Linkmapd-Receive", "Lsnr-Port", 9001);
//	CONF_SAVE(database_ip, conf_get_str("Database","Server-Addr"));
	sid = conf_get_str("Database","Sid");

	printf("port=%d\n", port);
	printf("sid=%s\n", sid);

	return 0;
}


