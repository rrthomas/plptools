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
#include "toplevel.h"

#include <stdio.h>
#include <stdlib.h>

#include <klocale.h>
#include <kcmdlineargs.h>
#include <kwin.h>
#include <kaboutdata.h>
#include <kuniqueapp.h>

int main(int argc, char *argv[])
{
    KAboutData about("klipsi", I18N_NOOP("Klipsi"), VERSION,
		     I18N_NOOP("Psion remote clipboard utility"),
		     KAboutData::License_GPL,
		     "(c) 2001, Fritz Elfert",
		     "http://plptools.sourceforge.net",
		     "plptools-developers@sourceforge.net");

    about.addAuthor("Fritz Elfert", I18N_NOOP("Original Developer/Maintainer"),
		    "felfert@users.sourceforge.net",
		    "http://plptools.sourceforge.net");

    KCmdLineArgs::init(argc, argv, &about);
    KUniqueApplication::addCmdLineOptions();

    if (!KUniqueApplication::start()) {
	fprintf(stderr, "%s is already running!\n", about.appName());
	exit(0);
    }
    KUniqueApplication app;

    TopLevel *toplevel = new TopLevel();

    KWin::setSystemTrayWindowFor(toplevel->winId(), 0);
    toplevel->setGeometry(-100, -100, 42, 42 );
    toplevel->show();

    return app.exec();
}
/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
