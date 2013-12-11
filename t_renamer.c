/*
 * t_renamer.c
 *
 * renamer for tagutil.
 */
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_renamer.h"


/*
 * t_rename_pattern definition
 *
 * a t_rename_pattern is a list of parsed tokens. Token are either a tag
 * reference or a string litteral.
 * */
struct t_rename_token {
	int		is_tag; /* 0 means string litteral, != 0 means a tag */
	const char	*value;
	TAILQ_ENTRY(t_rename_token)	entries;
};
TAILQ_HEAD(t_rename_pattern, t_rename_token);


/* helper for t_rename_parse() */
static struct t_rename_token	*t_rename_token_new(int is_tag, const char *value);
/* helper for t_rename_safe(), taken from mkdir(3) */
static int	build(char *path, mode_t omode);


/* alloc and initialize a new t_rename_token */
static struct t_rename_token *
t_rename_token_new(int is_tag, const char *value)
{
	struct t_rename_token *token;
	char *p;
	size_t len;

	len = strlen(value);
	token = malloc(sizeof(struct t_rename_token) + len + 1);
	if (token != NULL) {
		token->is_tag = is_tag;
		token->value = p = (char *)(token + 1);
		memcpy(p, value, len + 1);
	}
	return (token);
}
#define	t_rename_strtok(s)	t_rename_token_new(0, (s))
#define	t_rename_tagtok(s)	t_rename_token_new(1, (s))


struct t_rename_pattern *
t_rename_parse(const char *source)
{
	const char sep = '%';
	const char *c = source;
	struct t_rename_pattern *pattern = NULL;
	struct t_rename_token *token;
	struct sbuf *sb = NULL;
	enum {
		PARSING_STRING,
		PARSING_SIMPLE_TAG,
		PARSING_BRACE_TAG
	} state;

	sb = sbuf_new_auto();
	if (sb == NULL)
		goto error_label;

	pattern = malloc(sizeof(struct t_rename_pattern));
	if (pattern == NULL)
		goto error_label;
	TAILQ_INIT(pattern);

	state = PARSING_STRING;
	while (*c != '\0') {
		/* if we parse a litteral string, check if this is the start of
		   a tag */
		if (state == PARSING_STRING && *c == sep) {
			/* avoid to add a empty token. This can happen when
			   when parsing two consecutive tags like `%tag%tag' */
			if (sbuf_len(sb) > 0) {
				if (sbuf_finish(sb) == -1)
					goto error_label;
				token = t_rename_strtok(sbuf_data(sb));
				if (token == NULL)
					goto error_label;
				TAILQ_INSERT_TAIL(pattern, token, entries);
			}
			sbuf_clear(sb);
			c += 1;
			if (*c == '{') {
				c += 1;
				state = PARSING_BRACE_TAG;
			} else {
				state = PARSING_SIMPLE_TAG;
			}
		/* if we parse a tag, check for the end of it */
		} else if ((state == PARSING_SIMPLE_TAG && (isspace(*c) || *c == sep)) ||
		           (state == PARSING_BRACE_TAG  && *c == '}')) {
			if (sbuf_len(sb) == 0)
				warnx("empty tag in rename pattern");
			if (sbuf_finish(sb) == -1)
				goto error_label;
			token = t_rename_tagtok(sbuf_data(sb));
			if (token == NULL)
				goto error_label;
			sbuf_clear(sb);
			TAILQ_INSERT_TAIL(pattern, token, entries);
			if (state == PARSING_BRACE_TAG) {
				/* eat the closing `}' */
				c += 1;
			}
			state = PARSING_STRING;
		} else {
			/* default case for both string and tags. `\' escape
			   everything */
			if (*c == '\\') {
				c += 1;
				if (*c == '\0')
					break;
			}
			sbuf_putc(sb, *c);
			c += 1;
		}
	}
	/* we've hit the end of the source. Check in which state we are and try
	   to finish cleany */
	switch (state) {
	case PARSING_BRACE_TAG:
		warnx("missing closing `}' at the end of the rename pattern");
		goto error_label;
	case PARSING_SIMPLE_TAG:
		if (sbuf_len(sb) == 0)
			warnx("empty tag at the end of the rename pattern");
	case PARSING_STRING:
		/* all is right */;
	}
	/* finish the last tag unless it is the empty string */
	if (state != PARSING_STRING || sbuf_len(sb) > 0) {
		if (sbuf_finish(sb) == -1)
			goto error_label;
		token = t_rename_token_new(state != PARSING_STRING, sbuf_data(sb));
		if (token == NULL)
			goto error_label;
		TAILQ_INSERT_TAIL(pattern, token, entries);
	}

	sbuf_delete(sb);
	return (pattern);
	/* NOTREACHED */
error_label:
	sbuf_delete(sb);
	t_rename_pattern_delete(pattern);
	return (NULL);
}


char *
t_rename_eval(struct t_tune *tune, struct t_rename_pattern *pattern)
{
	struct t_rename_token *token;
	struct sbuf *sb = NULL;
	struct t_taglist *tlist = NULL, *l = NULL;
	char *s = NULL, *ret;

	assert_not_null(tune);
	assert_not_null(pattern);

	sb = sbuf_new_auto();
	if (sb == NULL)
		goto error;

	tlist = t_tune_tags(tune);
	if (tlist == NULL)
		goto error;

	TAILQ_FOREACH(token, pattern, entries) {
		if (token->is_tag) {
			l = t_taglist_find_all(tlist, token->value);
			if (l == NULL)
				goto error;
			if (l->count > 0) {
				/* tag exist */
				if ((s = t_taglist_join(l, " + ")) == NULL)
					goto error;
				if (l->count > 1) {
					warnx("%s: has many `%s' tags, joined with `+'",
					    t_tune_path(tune), token->value);
				}
			}
			t_taglist_delete(l);
			l = NULL;
			if (s != NULL) {
				char *slash = strchr(s, '/');
				/* check for slash in tag value */
				if (slash != NULL) {
					warnx("%s: tag `%s' has / in value, replacing by `-'",
					    t_tune_path(tune), token->value);
					do {
						*slash = '-';
						slash = strchr(slash, '/');
					} while (slash != NULL);
				}
			}
		}
		if (s != NULL) {
			(void)sbuf_cat(sb, s);
			free(s);
			s = NULL;
		} else
			(void)sbuf_cat(sb, token->value);
	}

	ret = NULL;
	if (sbuf_len(sb) > MAXPATHLEN)
		warnx("t_rename_eval result is too long (>MAXPATHLEN)");
	else {
		if (sbuf_finish(sb) != -1)
			ret = strdup(sbuf_data(sb));
	}

	sbuf_delete(sb);
	t_taglist_delete(tlist);
	return (ret);
error:
	free(s);
	t_taglist_delete(l);
	sbuf_delete(sb);
	t_taglist_delete(tlist);
	return (NULL);
}


void
t_rename_pattern_delete(struct t_rename_pattern *pattern)
{
	struct t_rename_token *t1, *t2;

	if (pattern == NULL)
		return;

	t1 = TAILQ_FIRST(pattern);
	while (t1 != NULL) {
		t2 = TAILQ_NEXT(t1, entries);
		free(t1);
		t1 = t2;
	}
	free(pattern);
}


int
t_rename_safe(const char *opath, const char *npath)
{
	extern int dflag;
	int failed = 0;
	struct stat st;
	const char *s;
	char odir[MAXPATHLEN], ndir[MAXPATHLEN];

	assert_not_null(opath);
	assert_not_null(npath);

	if ((s = t_dirname(opath)) == NULL) {
		warn("dirname");
		return (-1);
	}
	assert(strlcpy(odir, s, sizeof(odir)) < sizeof(odir));
	if ((s = t_dirname(npath)) == NULL) {
		warn("dirname");
		return (-1);
	}
	assert(strlcpy(ndir, s, sizeof(odir)) < sizeof(odir));

	if (strcmp(odir, ndir) != 0) {
		/* srcdir != destdir, we need to check if destdir is OK */
		if (dflag) { /* we are asked to create the directory */
			char *d = strdup(ndir);
			if (d == NULL)
				return (-1);
			(void)build(d, S_IRWXU | S_IRWXG | S_IRWXO);
			free(d);
		}
		if (stat(ndir, &st) != 0) {
			failed = 1;
			if (errno == ENOENT && !dflag)
				warn("%s (forgot -d ?):", ndir);
		} else if (!S_ISDIR(st.st_mode)) {
			failed = 1;
			errno  = ENOTDIR;
			warn("%s", ndir);
		}
	}
	if (failed)
		return (-1);

	if (stat(npath, &st) == 0) {
		errno = EEXIST;
		warn("%s", npath);
		return (-1);
	}

	if (rename(opath, npath) == -1) {
		warn("rename");
		return (-1);
	}

	return (0);
}


/*-
 * Copyright (c) 1983, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#if 0
__FBSDID("$FreeBSD: src/bin/mkdir/mkdir.c,v 1.33 2006/10/10 20:18:20 ru Exp $");
#endif

/*
 * Returns 1 if a directory has been created,
 * 2 if it already existed, and 0 on failure.
 */
static int
build(char *path, mode_t omode)
{
	struct stat sb;
	mode_t numask, oumask;
	int first, last, retval;
	char *p;

	p = path;
	oumask = 0;
	retval = 1;
	if (p[0] == '/')		/* Skip leading '/'. */
		++p;
	for (first = 1, last = 0; !last ; ++p) {
		if (p[0] == '\0')
			last = 1;
		else if (p[0] != '/')
			continue;
		*p = '\0';
		if (!last && p[1] == '\0')
			last = 1;
		if (first) {
			/*
			 * POSIX 1003.2:
			 * For each dir operand that does not name an existing
			 * directory, effects equivalent to those caused by the
			 * following command shall occcur:
			 *
			 * mkdir -p -m $(umask -S),u+wx $(dirname dir) &&
			 *    mkdir [-m mode] dir
			 *
			 * We change the user's umask and then restore it,
			 * instead of doing chmod's.
			 */
			oumask = umask(0);
			numask = oumask & ~(S_IWUSR | S_IXUSR);
			(void)umask(numask);
			first = 0;
		}
		if (last)
			(void)umask(oumask);
		if (mkdir(path, last ? omode : S_IRWXU | S_IRWXG | S_IRWXO) < 0) {
			if (errno == EEXIST || errno == EISDIR) {
				if (stat(path, &sb) < 0) {
					warn("build: %s", path);
					retval = 0;
					break;
				} else if (!S_ISDIR(sb.st_mode)) {
					if (last)
						errno = EEXIST;
					else
						errno = ENOTDIR;
					retval = 0;
					break;
				}
				if (last)
					retval = 2;
			} else {
				retval = 0;
				break;
			}
		}
		if (!last)
		    *p = '/';
	}
	if (!first && !last)
		(void)umask(oumask);
	return (retval);
}
