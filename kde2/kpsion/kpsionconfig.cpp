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
#include <kstddirs.h>

#include <iostream>

KPsionConfig::KPsionConfig() {
    optionNames.insert(OPT_BACKUPDIR, QString("Settings/BackupDir"));
    optionNames.insert(OPT_BACKUPGEN, QString("Settings/BackupGenerations"));
    optionNames.insert(OPT_INCINTERVAL, QString("Settings/IncrementalInterval"));
    optionNames.insert(OPT_FULLINTERVAL, QString("Settings/FullInterval"));
    optionNames.insert(OPT_CONNRETRY, QString("Connection/Retry"));
    optionNames.insert(OPT_SERIALDEV, QString("Connection/Device"));
    optionNames.insert(OPT_SERIALSPEED, QString("Connection/Speed"));
    optionNames.insert(OPT_NCPDPATH, QString("Connection/NcpdPath"));
    optionNames.insert(OPT_UIDS, QString("Psion/MachineUIDs"));
    optionNames.insert(OPT_MACHNAME, QString("Psion/Name_%1"));
    optionNames.insert(OPT_BACKUPDRIVES, QString("Psion/BackupDrives_%1"));
    optionNames.insert(OPT_DRIVES, QString("Psion/Drives_%1"));
    optionNames.insert(OPT_LASTFULL, QString("Psion/LastFull_%1_%1"));
    optionNames.insert(OPT_LASTINC, QString("Psion/LastInc_%1_%1"));
    optionNames.insert(OPT_SYNCTIME, QString("Psion/SyncTime_%1"));

    defaults.insert(DEF_INCINTERVAL, QString("1"));
    defaults.insert(DEF_FULLINTERVAL, QString("7"));
    defaults.insert(DEF_CONNRETRY, QString("30"));
    defaults.insert(DEF_SERIALDEV, QString("0"));
    defaults.insert(DEF_SERIALSPEED, QString("4"));
    defaults.insert(DEF_BACKUPGEN, QString("3"));
    defaults.insert(DEF_NCPDPATH, QString("ncpd"));
    defaults.insert(DEF_SYNCTIME, QString("false"));
}

const QString KPsionConfig::
getStrDefault(int optIdx) {
    if (optIdx != DEF_BACKUPDIR)
	return QString::null;
    return locateLocal("data", "kpsion/backups");
}

int KPsionConfig::
getIntDefault(int optIdx) {
    cfgMap::Iterator it = defaults.find(optIdx);
    if (it == defaults.end())
	return 0;
    return (*it).toInt();
}

bool KPsionConfig::
getBoolDefault(int optIdx) {
    cfgMap::Iterator it = defaults.find(optIdx);
    if (it == defaults.end())
	return false;
    return ((*it).compare("true") == 0);
}

const QString KPsionConfig::
getOptionName(int optIdx) {
    cfgMap::Iterator it = optionNames.find(optIdx);
    if (it == optionNames.end())
	return QString::null;
    int slash = (*it).find('/');
    return (*it).mid(slash + 1);
}

const QString KPsionConfig::
getSectionName(int optIdx) {
    cfgMap::Iterator it = optionNames.find(optIdx);
    if (it == optionNames.end())
	return QString::null;
    int slash = (*it).find('/');
    return (*it).left(slash);
}

QStringList KPsionConfig::
getConfigDevices() {
    QStringList l;

    l += i18n("off");
    l += QString("/dev/ttyS0");
    l += QString("/dev/ttyS1");
    l += QString("/dev/ttyS2");
    l += QString("/dev/ttyS3");
    l += QString("/dev/ircomm0");
    l += QString("/dev/ircomm1");
    l += QString("/dev/ircomm2");
    l += QString("/dev/ircomm3");

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

QStringList KPsionConfig::
getConfigBackupInterval() {
    QStringList l;

    l += i18n("none");
    l += i18n("daily");
    l += i18n("every 2 days");
    l += i18n("every 3 days");
    l += i18n("every 4 days");
    l += i18n("every 5 days");
    l += i18n("every 6 days");
    l += i18n("weekly");
    l += i18n("every 2 weeks");
    l += i18n("every 3 weeks");
    l += i18n("monthly");

    return l;
}

int KPsionConfig::
getIntervalDays(KConfig *config, int optIdx) {
    config->setGroup(getSectionName(optIdx));
    int i = config->readNumEntry(getOptionName(optIdx));
    switch (i) {
	case 8:
	    return 14;
	case 9:
	    return 21;
	case 10:
	    return 28;
    }
    return i;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
