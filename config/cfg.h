/*	$OpenBSD: conf.h,v 1.12 2000/10/07 07:00:06 niklas Exp $	*/
/*	$EOM: conf.h,v 1.13 2000/09/18 00:01:47 ho Exp $	*/

#ifndef _CONF_H_
#define _CONF_H_

#include <sys/types.h>
#include <sys/queue.h>
#include <stdio.h>


struct conf_list_node {
	TAILQ_ENTRY (conf_list_node) link;
	char *field;
};

struct conf_list {
	int cnt;
	TAILQ_HEAD (conf_list_fields_head, conf_list_node) fields;
};

//extern char *conf_path;
extern int conf_begin (void);
extern int conf_decode_base64 (u_int8_t *out, u_int32_t *len, u_char *buf);
extern int conf_end (int, int);
extern void conf_free_list (struct conf_list *);
extern int conf_get_line (FILE *, char *, u_int32_t);
extern struct conf_list *conf_get_list (char *, char *);
extern struct conf_list *conf_get_tag_list (char *);
extern int conf_get_num (char *, char *, int);
extern char *conf_get_str (char *, char *);
extern void conf_init (char *filename);
extern int conf_match_num (char *, char *, int);
extern void conf_reinit (void);
extern int conf_remove (int, char *, char *);
extern int conf_remove_section (int, char *);
extern int conf_set (int, char *, char *, char *, int, int);
extern void conf_report (void);

#endif /* _CONF_H_ */
