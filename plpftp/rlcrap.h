/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999  Philip Proudman <philip.proudman@btinternet.com>
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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#if HAVE_LIBREADLINE
#include <readline/readline.h>
#ifdef __cplusplus
extern "C" {
#endif
extern CPFunction *cmdgen_ptr;
extern CPFunction *fnmgen_ptr;

typedef char *(*CorrectFunProto)(char *, int);

extern void rlcrap_setpointers(CorrectFunProto, CorrectFunProto);
#ifdef __cplusplus
}
#endif
#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
