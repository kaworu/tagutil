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


/* type of action that can be performed */
enum t_actionkind {
	/* user options */
	T_ACTION_ADD,		/* add:TAG=VALUE	add tag */
	T_ACTION_SHOWBACKEND,	/* backend		show backend */
	T_ACTION_CLEAR,	/* clear:TAG		clear tag */
	T_ACTION_EDIT,		/* edit			edit with $EDITOR */
	T_ACTION_LOAD,		/* load:PATH		load file */
	T_ACTION_SHOW,		/* print		display tags */
	T_ACTION_SHOWPATH,	/* path			display only file path */
	T_ACTION_RENAME,	/* rename:PATTERN	rename files */
	T_ACTION_SET,		/* set:TAG=VALUE	set tags */
	T_ACTION_FILTER,	/* filter:FILTER	filter */
	/* internal */
	T_ACTION_RELOAD,
	T_ACTION_SAVE_IF_DIRTY,
};

/* one action to do. */
struct t_action {
	enum t_actionkind kind;
	void	*data; /* argument of the action */
	bool	write; /* true if the action need write access */
	bool (*apply)(struct t_action *self,
	    struct t_file *file);
	TAILQ_ENTRY(t_action)	entries;
};
/* action queue head */
TAILQ_HEAD(t_actionQ, t_action);

/*
 * show usage and exit.
 */
_t__dead2
void usage(void);

/*
 * Create a queue of action based on argc/argv.
 *
 * @param argcp
 *   A pointer to argc.
 *
 * @param argvp
 *   A pointer to argv.if not NULL, write will be set
 *
 * @param writep
 *   If not null, the pointer is set to true if any action actually need write
 *   access.
 *
 * @return
 *   A new t_actionQ that resulted from argc/argv parsing.
 */
struct t_actionQ	*t_actionQ_new(int *argcp, char ***argvp, bool *writep);

/*
 * destroy (free memory) of an action queue.
 */
void	t_actionQ_delete(struct t_actionQ *aQ);
#endif /* not T_ACTION_H */
