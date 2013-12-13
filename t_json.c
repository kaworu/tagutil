/*
 * t_json.c
 *
 * json tagutil interface, using jansson.
 */
#include <sys/param.h>

#include <string.h>
#include <stdlib.h>

/* jansson headers */
#include "jansson.h"

#include "t_config.h"
#include "t_toolkit.h"
#include "t_taglist.h"
#include "t_tune.h"
#include "t_json.h"

int
t_json_whdl(void *sb, unsigned char *buffer, size_t size)
{

	assert_not_null(sb);
	assert_not_null(buffer);

	if (sb == NULL || buffer == NULL)
		return (0); /* error */
	else
		(void)sbuf_bcat(sb, buffer, size);
	
	return (1); /* success */
}


char *
t_tags2json(const struct t_taglist *tlist, const char *path)
{
	assert(0 && "Unimplemented");
	/* NOTREACHED */
}


/*
 * Our parser FSM. we only handle this JSON subset:
 *
 *  STREAM-START
 *      (DOCUMENT-START
 *          (SEQUENCE-START
 *              (MAPPING-START SCALAR SCALAR MAPPING-END)*
 *           SEQUENCE-END)?
 *       DOCUMENT-END)?
 *  STREAM-END
 */
struct t_taglist *
t_json2tags(FILE *fp, char **errmsg_p)
{
	assert(0 && "Unimplemented");
	/* NOTREACHED */
}
