/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2001 Fritz Elfert <felfert@to.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#ifndef _BUILTINS_H_
#define _BUILTINS_H_

struct builtin_node_t;

typedef struct builtin_child_t {
	struct builtin_child_t *next;
	struct builtin_node_t *node;
} builtin_child;

/**
 * Support for procfs-like builtin virtual files/directories
 */
typedef struct builtin_node_t {
	struct builtin_node_t *next;
	struct builtin_node_t *parent;
	builtin_child *childs;
	char *private_data;
	char *name;
	unsigned long flags;
	long attr;
	long size;
	long (*getsize)(struct builtin_node_t *node);
	long (*read)(struct builtin_node_t *node, char *buf, unsigned long offset, long len);
	long (*write)(struct builtin_node_t *node, char *buf, unsigned long offset, long len);
	long (*sattr)(struct builtin_node_t *node, unsigned long sa, unsigned long da);
	long (*getlinks)(struct builtin_node_t *node);
	long (*getdents)(struct builtin_node_t *node, dentry **entries);
} builtin_node;

#define BF_EXISTS_ALWAYS 1
#define BF_NOCACHE       2
#define BF_ISPROCESS     4

/**
 * Register a builtin handler for an entry in /proc
 */
builtin_node *register_builtin(char *parent, builtin_node *node);

/**
 * Deregister a previously registered handler.
 */
extern int unregister_builtin(builtin_node *);

extern char *builtin_path(builtin_node *node);
extern long generic_getlinks(builtin_node *node);
extern long generic_getdents(builtin_node *node, dentry **entries);

#endif /* _BUILTINS_H_ */

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
