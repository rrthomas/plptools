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
#ifndef _KPSIONCONFIG_H_
#define _KPSIONCONFIG_H_

#include <kconfig.h>

#include <qstringlist.h>
#include <qmap.h>

typedef QMap<int,QString> cfgMap;

class KPsionConfig {
public:

    enum cfgOptions {
	OPT_BACKUPDIR = 0,
	OPT_INCINTERVAL = 1,
	OPT_FULLINTERVAL = 2,
	OPT_CONNRETRY = 3,
	OPT_SERIALDEV = 4,
	OPT_SERIALSPEED = 5,
	OPT_BACKUPGEN = 6,
	OPT_UIDS = 7,
	OPT_MACHNAME = 8,
	OPT_BACKUPDRIVES = 9,
	OPT_DRIVES = 10,
	OPT_LASTFULL = 11,
	OPT_LASTINC = 12,
    };

    enum cfgDefaults {
	DEF_BACKUPDIR = 0,
	DEF_INCINTERVAL = 1,
	DEF_FULLINTERVAL = 2,
	DEF_CONNRETRY = 3,
	DEF_SERIALDEV = 4,
	DEF_SERIALSPEED = 5,
	DEF_BACKUPGEN = 6,
    };

    KPsionConfig();

    QStringList getConfigDevices();
    QStringList getConfigSpeeds();
    QStringList getConfigBackupInterval();
    const QString getOptionName(int);
    const QString getSectionName(int);
    const QString getStrDefault(int);
    int getIntDefault(int);
    int getIntervalDays(KConfig *config, int optIdx);

private:
    cfgMap optionNames;
    cfgMap defaults;

};
#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
