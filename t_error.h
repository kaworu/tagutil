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
#define	T_ERROR_MSG_MEMBER	char *t__errmsg

/* error message getter */
#define	t_error_msg(o)	((o)->t__errmsg)

/* initializer */
#define	t_error_init(o)	do { t_error_msg(o) = NULL; } while (/*CONSTCOND*/0)

/* set the error message (with printflike syntax) */
#define	t_error_set(o, fmt, ...) \
    do { (void)xasprintf(&t_error_msg(o), fmt, ##__VA_ARGS__); } while (/*CONSTCOND*/0)

/* reset the error message. free it if needed, set to NULL */
#define	t_error_clear(o) \
    do { free(t_error_msg(o)); t_error_init(o); } while (/*CONSTCOND*/0)


/*
 * error macros can be used on t_error struct or any struct that define a member:
 *      char *t__errmsg; (via the T_ERROR_MSG_MEMBER macro).
 *  on this purpose. You should never access to ->t__errmsg but use the macros
 *  defined for this purpose.
 *
 *  At any time, t__errmsg should either a valid malloc()'d pointer either NULL,
 *  that way we can always pass it to free().
 */
struct t_error {
	T_ERROR_MSG_MEMBER;
};


/*
 * create a t_error struct.
 */
t__unused
static inline struct t_error	*t_error_new(void);

/*
 * free a t_error struct.
 */
t__unused static inline void	t_error_delete(struct t_error *e);


static inline struct t_error *
t_error_new(void)
{
	struct t_error *e;

	e = malloc(sizeof(struct t_error));
	if (e != NULL)
		t_error_init(e);

	return (e);
}


static inline void
t_error_delete(struct t_error *e)
{

	assert_not_null(e);

	t_error_clear(e);
	freex(e);
}
#endif /* not T_ERROR_H */
