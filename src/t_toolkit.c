/*
 * t_toolkit.c
 *
 * handy functions toolkit for tagutil.
 *
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

#include <locale.h>
#include <iconv.h>

#include "t_config.h"
#include "t_toolkit.h"
#include "t_action.h"


/* accept NULL as src */
static char *	t_iconv_convert(int tou8, const char *src);


char *
t_strtoupper(char *str)
{
	char *s;

	assert_not_null(str);

	for (s = str; *s != '\0'; s++)
		*s = toupper(*s);
	return (str);
}


char *
t_strtolower(char *str)
{
	char *s;

	assert_not_null(str);

	for (s = str; *s != '\0'; s++)
		*s = tolower(*s);
	return (str);
}


char *
t_iconv_utf8_to_loc(const char *src)
{

	return (t_iconv_convert(0, src));
}


char *
t_iconv_loc_to_utf8(const char *src)
{

	return (t_iconv_convert(1, src));
}


/*	$OpenBSD: dirname.c,v 1.14 2013/04/05 12:59:54 kurt Exp $	*/

/*
 * Copyright (c) 1997, 2004 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <sys/param.h>

/* A slightly modified copy of this file exists in libexec/ld.so */

char *
t_dirname(const char *path)
{
	static char dname[MAXPATHLEN];
	size_t len;
	const char *endp;

	/* Empty or NULL string gets treated as "." */
	if (path == NULL || *path == '\0') {
		dname[0] = '.';
		dname[1] = '\0';
		return (dname);
	}

	/* Strip any trailing slashes */
	endp = path + strlen(path) - 1;
	while (endp > path && *endp == '/')
		endp--;

	/* Find the start of the dir */
	while (endp > path && *endp != '/')
		endp--;

	/* Either the dir is "/" or there are no slashes */
	if (endp == path) {
		dname[0] = *endp == '/' ? '/' : '.';
		dname[1] = '\0';
		return (dname);
	} else {
		/* Move forward past the separating slashes */
		do {
			endp--;
		} while (endp > path && *endp == '/');
	}

	len = endp - path + 1;
	if (len >= sizeof(dname)) {
		errno = ENAMETOOLONG;
		return (NULL);
	}
	memcpy(dname, path, len);
	dname[len] = '\0';
	return (dname);
}


/*	$OpenBSD: basename.c,v 1.14 2005/08/08 08:05:33 espie Exp $	*/

/*
 * Copyright (c) 1997, 2004 Todd C. Miller <Todd.Miller@courtesan.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <sys/param.h>

char *
t_basename(const char *path)
{
	static char bname[MAXPATHLEN];
	size_t len;
	const char *endp, *startp;

	/* Empty or NULL string gets treated as "." */
	if (path == NULL || *path == '\0') {
		bname[0] = '.';
		bname[1] = '\0';
		return (bname);
	}

	/* Strip any trailing slashes */
	endp = path + strlen(path) - 1;
	while (endp > path && *endp == '/')
		endp--;

	/* All slashes becomes "/" */
	if (endp == path && *endp == '/') {
		bname[0] = '/';
		bname[1] = '\0';
		return (bname);
	}

	/* Find the start of the base */
	startp = endp;
	while (startp > path && *(startp - 1) != '/')
		startp--;

	len = endp - startp + 1;
	if (len >= sizeof(bname)) {
		errno = ENAMETOOLONG;
		return (NULL);
	}
	memcpy(bname, startp, len);
	bname[len] = '\0';
	return (bname);
}


static char *
t_iconv_convert(int tou8, const char *const_src)
{
	static int setlocale_called = 0;
	int success = 0;
	size_t n, srclen, destlen;
	char *dest, *ret = NULL, *src = NULL;
	iconv_t cd = (iconv_t)-1;

	if (const_src == NULL)
		goto cleanup;

	if (!setlocale_called) {
		setlocale(LC_ALL, "");
		setlocale_called = 1;
	}

	if (tou8)
		cd = iconv_open("utf-8", "");
	else
		cd = iconv_open("", "utf-8");
	if (cd == (iconv_t)-1)
		goto cleanup;

	srclen = strlen(const_src);
	ret = dest = calloc(srclen + 1, 4 * sizeof(char));
	if (ret == NULL)
		goto cleanup;
	destlen = srclen * 4;

#if defined(ICONV_SECOND_ARGUMENT_IS_CONST)
	n = iconv(cd, &const_src, &srclen, &dest, &destlen);
#else
	src = strdup(const_src);
	if (src == NULL)
		goto cleanup;
	n = iconv(cd, &src, &srclen, &dest, &destlen);
#endif
	success = (n != -1);

cleanup:
	if (!success) {
		free(ret);
		ret = NULL;
	}
	free(src);
	if (cd != (iconv_t)-1)
		iconv_close(cd);
	return (ret);
}
