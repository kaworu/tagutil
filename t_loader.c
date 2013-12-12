/*
 * t_loader.c
 *
 * Routine to load a YAML tag file in a tune for tagutil.
 */
#include "t_config.h"
#include "t_loader.h"

#include "t_tune.h"
#include "t_yaml.h"


int
t_load(struct t_tune *tune, const char *yamlfile)
{
	FILE *fp;
	char *errmsg;
	struct t_taglist *tlist;
	int ret;

	assert_not_null(tune);
	assert_not_null(yamlfile);

	if (strcmp(yamlfile, "-") == 0)
		fp = stdin;
	else {
		fp = fopen(yamlfile, "r");
		if (fp == NULL) {
			warn("%s: fopen", yamlfile);
			return (-1);
		}
	}

	tlist = t_yaml2tags(fp, &errmsg);
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
