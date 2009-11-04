#ifndef	T_ERROR_H
#define	T_ERROR_H
/*
 * t_error.h
 *
 * error handling for tagutil.
 */
#include "t_config.h"
#include "t_toolkit.h"


/* error handling macros */

/* used for any struct that need to behave like a t_error */
#define	T_ERROR_MSG_MEMBER	char *__errmsg

/* error message getter */
#define	t_error_msg(o)	((o)->__errmsg)

/* initializer */
#define	t_error_init(o)	do { t_error_msg(o) = NULL; } while (/*CONSTCOND*/0)

/* set the error message (with printflike syntax) */
#define	t_error_set(o, fmt, ...) \
    do { (void)xasprintf(&t_error_msg(o), fmt, ##__VA_ARGS__); } while (/*CONSTCOND*/0)

/* reset the error message. free it if needed, set to NULL */
#define	t_error_clear(o) \
    do { freex(t_error_msg(o)); } while (/*CONSTCOND*/0)


/*
 * error macros can be used on t_error struct or any struct that define a member:
 *      char *__errmsg; (via the T_ERROR_MSG_MEMBER macro).
 *  on this purpose. You should never access to ->__errmsg but use the macros
 *  defined for this  purpose.
 *
 *  At any time, __errmsg should either a valid malloc()'d pointer either NULL,
 *  that way we can always pass it to free().
 */
struct t_error {
	T_ERROR_MSG_MEMBER;
};


/*
 * create a t_error struct.
 */
_t__unused
static inline struct t_error *
t_error_new(void);

/*
 * free a t_error struct.
 */
_t__unused _t__nonnull(1)
static inline void
t_error_destroy(struct t_error *restrict e);


static inline struct t_error *
t_error_new(void)
{
	struct t_error *e;

	e = xmalloc(sizeof(struct t_error));
	t_error_init(e);

	return (e);
}


static inline void
t_error_destroy(struct t_error *restrict e)
{

	assert_not_null(e);

	t_error_clear(e);
	freex(e);
}
#endif /* not T_ERROR_H */
