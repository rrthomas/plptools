/*
 * $Id$
 */

#ifndef _BUILTINS_H_
#define _BUILTINS_H_

typedef struct builtin_node_t {
	char *name;
	unsigned long flags;
	long attr;
	long size;
	long (*getsize)(struct builtin_node_t *node);
	long (*read)(struct builtin_node_t *node, char *buf, unsigned long offset, long len);
	long (*write)(struct builtin_node_t *node, char *buf, unsigned long offset, long len);
	long (*sattr)(struct builtin_node_t *node, unsigned long sa, unsigned long da);
} builtin_node;

#define BF_EXISTS_ALWAYS 1
#define BF_NOCACHE       2

/**
 * Register a builtin handler for an entry in /proc
 */
extern int register_builtin(builtin_node *);

/**
 * Deregister a previously registered handler.
 */
extern int unregister_builtin(char *);

#endif /* _BUILTINS_H_ */

