/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999-2002 Fritz Elfert <felfert@to.com>
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
#ifndef _PSIPROCESS_H_
#define _PSIPROCESS_H_

#include <string>

class rpcs;

/**
 * A class, describing a Process on the Psion.
 * Objects of this type are used by @ref rpcs::queryPrograms
 * for returning the currently running processes.
 *
 * @author Fritz Elfert <felfert@to.com>
 */
class PsiProcess {

public:
    /**
    * Default constructor
    */
    PsiProcess();

    /**
    * A copy constructor.
    * Mainly used by STL container classes.
    *
    * @param d The object to be used as initializer.
    */
    PsiProcess(const PsiProcess &p);

    /**
    * Initializing Constructor
    */
    PsiProcess(const int, const char * const, const char * const, bool);

    /**
    * Default destructor.
    */
    ~PsiProcess() {};

    /**
    * Retrieves the PID of a process.
    *
    * @returns The PID of this instance.
    */
    int getPID();

    /**
    * Retrieve the file name of a process.
    *
    * @returns The name of this instance.
    */
    const char *getName();

    /**
    * Retrieve the file name of a process.
    *
    * @returns The arguments of this instance.
    */
    const char *getArgs();

    /**
    * Retrieve the file name and PID of a process.
    *
    * @returns The name and PID this instance in the format
    *          name.$pid .
    */
    const char *getProcId();

    /**
    * Assignment operator
    * Mainly used by STL container classes.
    *
    * @param e The new value to assign.
    *
    * @returns The modified object.
    */
    PsiProcess &operator=(const PsiProcess &p);

    /**
    * Prints the object contents.
    * The output is in human readable similar to the
    * output of a "ls" command.
    */
    friend ostream &operator<<(ostream &o, const PsiProcess &p);

private:
    friend class rpcs;

    void setArgs(string _args);

    int pid;
    string  name;
    string  args;
    bool s5mx;
};

#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */