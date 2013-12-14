/*
 * t_format.c
 *
 * output formats functions for tagutil. Very similar to t_backend.
 */
#include "t_config.h"
#include "t_toolkit.h"

#include "t_format.h"


struct t_format	*t_yaml_format(void) t__weak;
struct t_format	*t_json_format(void) t__weak;


const struct t_formatQ *
t_all_formats(void)
{
	static int initialized = 0;
	static struct t_formatQ fQ = TAILQ_HEAD_INITIALIZER(fQ);

	if (!initialized) {
		/* add each available format (order matter, first is the
		   default) */

		/* YAML */
		if (t_yaml_format != NULL)
			TAILQ_INSERT_TAIL(&fQ, t_yaml_format(), entries);

		/* JSON */
		if (t_json_format != NULL)
			TAILQ_INSERT_TAIL(&fQ, t_json_format(), entries);

		initialized = 1;
	}

	return (&fQ);
}
