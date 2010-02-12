#ifndef T_ACTION_H
#define T_ACTION_H
/*
 * @(#)t_action.h
 *
 * tagutil actions.
 */
#include "t_config.h"
#include "t_toolkit.h"
#include "t_file.h"

#define	GETOPT_STRING	"bedhpNPYa:c:f:r:s:x:"

extern bool	dflag, Nflag, Yflag;	/* -d, -N and -Y */


enum t_actionkind {
	/* user options */
	T_ADD,		/* -a TAG=VALUE	add tag */
	T_SHOWBACKEND,	/* -b		show backend */
	T_CLEAR,	/* -c TAG	clear tag */
	T_EDIT,		/* -e		edit */
	T_LOAD,		/* -f PATH	load file */
	T_SHOW,		/* -p		display tags */
	T_SHOWPATH,	/* -P		display only names */
	T_RENAME,	/* -r PATTERN	rename files */
	T_SET,		/* -s TAG=VALUE	set tags */
	T_FILTER,	/* -x FILTER	filter */

	/* internal */
	T_RELOAD,
	T_SAVE_IF_DIRTY,
};


struct t_action {
	enum t_actionkind kind;
	void	*data;
	bool	rw; /* true if the action need read and write access */
	bool (*apply)(struct t_action *restrict self, struct t_file **filep);
	TAILQ_ENTRY(t_action)	entries;
};
TAILQ_HEAD(t_actionQ, t_action);


/*
 * show usage and exit.
 */
_t__dead2
void usage(void);

/*
 * TODO
 */
_t__nonnull(1) _t__nonnull(2)
struct t_actionQ *	t_actionQ_create(int *argcp, char ***argvp);

/*
 * TODO
 */
struct t_action *	t_action_new(enum t_actionkind kind, char *arg);

/*
 * TODO
 */
_t__nonnull(1)
void	t_action_destroy(struct t_action *a);

/*
 * TODO
 */
_t__nonnull(1)
void	t_actionQ_destroy(struct t_actionQ *restrict aQ);
#endif /* not T_ACTION_H */

