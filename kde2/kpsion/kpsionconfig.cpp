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

#include "kpsionconfig.h"

#include <klocale.h>

#include <iostream.h>

KPsionConfig::KPsionConfig() {
    optionNames.insert(OPT_BACKUPDIR, QString("Settings/BackupDir"));
    optionNames.insert(OPT_BACKUPGEN, QString("Settings/BackupGenerations"));
    optionNames.insert(OPT_INCINTERVAL, QString("Settings/IncrementalInterval"));
    optionNames.insert(OPT_FULLINTERVAL, QString("Settings/FullInterval"));
    optionNames.insert(OPT_CONNRETRY, QString("Connection/Retry"));
    optionNames.insert(OPT_SERIALDEV, QString("Connection/Device"));
    optionNames.insert(OPT_SERIALSPEED, QString("Connection/Speed"));
    optionNames.insert(OPT_UIDS, QString("Psion/MachineUIDs"));
    optionNames.insert(OPT_MACHNAME, QString("Psion/Name_%1"));
    optionNames.insert(OPT_BACKUPDRIVES, QString("Psion/BackupDrives_%1"));
}

const QString KPsionConfig::
getOptionName(int optIdx) {

    optMap::Iterator it = optionNames.find(optIdx);
    if (it == optionNames.end())
	return QString::null;
    int slash = (*it).find('/');
    return (*it).mid(slash + 1);
}

const QString KPsionConfig::
getSectionName(int optIdx) {
    optMap::Iterator it = optionNames.find(optIdx);
    if (it == optionNames.end())
	return QString::null;
    int slash = (*it).find('/');
    return (*it).left(slash);
}

QStringList KPsionConfig::
getConfigDevices() {
    QStringList l;

    l += i18n("off");
    l += i18n("/dev/ttyS0");
    l += i18n("/dev/ttyS1");
    l += i18n("/dev/ttyS2");
    l += i18n("/dev/ttyS3");
    l += i18n("/dev/ircomm0");
    l += i18n("/dev/ircomm1");
    l += i18n("/dev/ircomm2");
    l += i18n("/dev/ircomm3");

    return l;
}

QStringList KPsionConfig::
getConfigSpeeds() {
    QStringList l;

    l += QString("9600");
    l += QString("19200");
    l += QString("38400");
    l += QString("57600");
    l += QString("115200");

    return l;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
