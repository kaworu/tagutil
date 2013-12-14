/*
 * t_loader.c
 *
 * Routine to load a YAML tag file in a tune for tagutil.
 */
#include "t_config.h"
#include "t_loader.h"
#include "t_format.h"
#include "t_tune.h"


int
t_load(struct t_tune *tune, const char *fmtfile)
{
	int ret;
	char *errmsg;
	struct t_taglist *tlist;
	extern const struct t_format *Fflag;
	FILE *fp;

	assert_not_null(tune);
	assert_not_null(fmtfile);

	if (strlen(fmtfile) == 0 || strcmp(fmtfile, "-") == 0)
		fp = stdin;
	else {
		fp = fopen(fmtfile, "r");
		if (fp == NULL) {
			warn("%s: fopen", fmtfile);
			return (-1);
		}
	}

	tlist = Fflag->fmt2tags(fp, &errmsg);
	if (fp != stdin)
		(void)fclose(fp);
	if (tlist == NULL) {
		warnx("%s", errmsg);
		free(errmsg);
		return (-1);
	 }
	ret = t_tune_set_tags(tune, tlist);
	t_taglist_delete(tlist);
	return (ret);
}
