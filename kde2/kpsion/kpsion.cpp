/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 2000-2002 Fritz Elfert <felfert@to.com>
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

#include "kpsion.h"
#include "kpsionconfig.h"
#include "wizards.h"

#include <kapp.h>
#include <klocale.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kconfig.h>
#include <kiconview.h>
#include <kmessagebox.h>
#include <kfileitem.h>
#include <kprocess.h>

#include <qwhatsthis.h>
#include <qtimer.h>
#include <qlayout.h>
#include <qiodevice.h>
#include <qheader.h>
#include <qdir.h>
#include <qmessagebox.h>

#include <ppsocket.h>
#include <rfsvfactory.h>
#include <rpcsfactory.h>
#include <bufferarray.h>
#include <psiprocess.h>

#define STID_CONNECTION 1

KPsionMainWindow::KPsionMainWindow()
    : KMainWindow() {
    setupActions();

    statusBar()->insertItem(i18n("Idle"), STID_CONNECTION, 1);
    statusBar()->setItemAlignment(STID_CONNECTION,
				  QLabel::AlignLeft|QLabel::AlignVCenter);

    progress = new KPsionStatusBarProgress(statusBar(), "progressBar");
    statusBar()->addWidget(progress, 10);

    connect(progress, SIGNAL(pressed()), this, SLOT(slotProgressBarPressed()));
    connect(this, SIGNAL(setProgress(int)), progress, SLOT(setValue(int)));
    connect(this, SIGNAL(setProgress(int, int)), progress,
			 SLOT(setValue(int, int)));
    connect(this, SIGNAL(setProgressText(const QString &)), progress,
			 SLOT(setText(const QString &)));
    connect(this, SIGNAL(enableProgressText(bool)), progress,
			 SLOT(setTextEnabled(bool)));

    backupRunning = false;
    restoreRunning = false;
    formatRunning = false;
    doScheduledBackup = false;
    quitImmediately = false;

    view = new KIconView(this, "iconview");
    view->setSelectionMode(KIconView::Multi);
    view->setResizeMode(KIconView::Adjust);
    view->setItemsMovable(false);
    connect(view, SIGNAL(clicked(QIconViewItem *)),
	    SLOT(iconClicked(QIconViewItem *)));
    connect(view, SIGNAL(onItem(QIconViewItem *)),
	    SLOT(iconOver(QIconViewItem *)));
    connect(this, SIGNAL(rearrangeIcons(bool)), view,
	    SLOT(arrangeItemsInGrid(bool)));
    KConfig *config = kapp->config();
    KPsionConfig pcfg;

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_UIDS));
    QStringList uids = config->readListEntry(
	pcfg.getOptionName(KPsionConfig::OPT_UIDS));

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_MACHNAME));
    QString tmp = pcfg.getOptionName(KPsionConfig::OPT_MACHNAME);

    for (QStringList::Iterator it = uids.begin(); it != uids.end(); it++)
	machines.insert(*it, config->readEntry(tmp.arg(*it)));

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPDIR));
    backupDir = config->readEntry(
	pcfg.getOptionName(KPsionConfig::OPT_BACKUPDIR));

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_CONNRETRY));
    reconnectTime = config->readNumEntry(
	pcfg.getOptionName(KPsionConfig::OPT_CONNRETRY));

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_SERIALDEV));
    ncpdDevice = config->readEntry(pcfg.getOptionName(
	KPsionConfig::OPT_SERIALDEV), "off");

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_NCPDPATH));
    ncpdPath = config->readEntry(pcfg.getOptionName(
	KPsionConfig::OPT_NCPDPATH), "ncpd");

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_SERIALSPEED));
    ncpdSpeed = config->readEntry(
	pcfg.getOptionName(KPsionConfig::OPT_SERIALSPEED), "115200");

    QWhatsThis::add(view, i18n(
			"<qt>Here, you see your Psion's drives.<br/>"
			"Every drive is represented by an Icon. If you "
			"click on it, it gets selected for the next "
			"operation. E.g.: backup, restore or format.<br/>"
			"To unselect it, simply click on it again.<br/>"
			"Select as many drives a you want, then choose "
			"an operation.</qt>"));
    setCentralWidget(view);

    rfsvSocket = 0L;
    rpcsSocket = 0L;
    plpRfsv = 0L;
    plpRpcs = 0L;

    firstTry = true;
    connected = false;
    shuttingDown = false;

    args = KCmdLineArgs::parsedArgs();
    if (args->isSet("autobackup")) {
	firstTry = false;
	reconnectTime = 0;
    }
    tryConnect();
}

KPsionMainWindow::~KPsionMainWindow() {
    shuttingDown = true;
    if (plpRfsv)
	delete plpRfsv;
    if (plpRpcs)
	delete plpRpcs;
    if (rfsvSocket)
	delete rfsvSocket;
    if (rfsvSocket)
	delete rpcsSocket;
}

QString KPsionMainWindow::
unix2psion(const char * const path) {
    QString tmp = path;
    tmp.replace(QRegExp("/"), "\\");
    tmp.replace(QRegExp("%2f"), "/");
    tmp.replace(QRegExp("%25"), "%");
    return tmp;
}

QString KPsionMainWindow::
psion2unix(const char * const path) {
    QString tmp = path;
    tmp.replace(QRegExp("%"), "%25");
    tmp.replace(QRegExp("/"), "%2f");
    tmp.replace(QRegExp("\\"), "/");
    return tmp;
}

void KPsionMainWindow::
setupActions() {

    KStdAction::quit(this, SLOT(close()), actionCollection());
    KStdAction::showToolbar(this, SLOT(slotToggleToolbar()),
			    actionCollection());
    KStdAction::showStatusbar(this, SLOT(slotToggleStatusbar()),
			      actionCollection());
    KStdAction::preferences(this, SLOT(slotPreferences()),
			    actionCollection());
    new KAction(i18n("Start &Format"), "psion_format", 0, this,
		SLOT(slotStartFormat()), actionCollection(), "format");
    new KAction(i18n("Start Full &Backup"), "psion_backup", 0, this,
		SLOT(slotStartFullBackup()), actionCollection(),
		"fullbackup");
    new KAction(i18n("Start &Incremental Backup"), "psion_backup", 0, this,
		SLOT(slotStartIncBackup()), actionCollection(), "incbackup");
    new KAction(i18n("Start &Restore"), "psion_restore", 0, this,
		SLOT(slotStartRestore()), actionCollection(), "restore");
    createGUI();

    actionCollection()->action("fullbackup")->setEnabled(false);
    actionCollection()->action("incbackup")->setEnabled(false);
    actionCollection()->action("restore")->setEnabled(false);
    actionCollection()->action("format")->setEnabled(false);

    actionCollection()->action("fullbackup")->
	setToolTip(i18n("Full backup of selected drive(s)"));
    actionCollection()->action("incbackup")->
	setToolTip(i18n("Incremental backup of selected drive(s)"));
    actionCollection()->action("restore")->
	setToolTip(i18n("Restore selected drive(s)"));
    actionCollection()->action("format")->
	setToolTip(i18n("Format selected drive(s)"));
}

void KPsionMainWindow::
iconOver(QIconViewItem *i) {
    lastSelected = i->isSelected();
}

void KPsionMainWindow::
switchActions() {
    QIconViewItem *i;
    bool rwSelected = false;
    bool anySelected = false;

    if (backupRunning | restoreRunning | formatRunning)
	view->setEnabled(false);
    else {
	for (i = view->firstItem(); i; i = i->nextItem()) {
	    if (i->isSelected()) {
		anySelected = true;
		if (i->key() != "Z") {
		    rwSelected = true;
		    break;
		}
	    }
	}
	view->setEnabled(true);
    }
    actionCollection()->action("restore")->setEnabled(rwSelected);
    actionCollection()->action("format")->setEnabled(rwSelected);
    actionCollection()->action("fullbackup")->setEnabled(anySelected);
    actionCollection()->action("incbackup")->setEnabled(anySelected);
}

void KPsionMainWindow::
iconClicked(QIconViewItem *i) {
    if (i == 0L)
	return;
    lastSelected = !lastSelected;
    i->setSelected(lastSelected);
    switchActions();
}

void KPsionMainWindow::
insertDrive(char letter, const char * const name) {
    QString tmp;

    if (name && strlen(name))
	tmp = QString::fromLatin1("%1 (%2:)").arg(name).arg(letter);
    else
	tmp = QString::fromLatin1("%1:").arg(letter);
    drives.insert(letter,tmp);
    QIconViewItem *it =
	new QIconViewItem(view, tmp,
			  KGlobal::iconLoader()->loadIcon("psion_drive",
							  KIcon::Desktop));
    tmp = QString::fromLatin1("%1").arg(letter);
    it->setKey(tmp);
    it->setDropEnabled(false);
    it->setDragEnabled(false);
    it->setRenameEnabled(false);
}

bool KPsionMainWindow::
shouldQuit() {
    return quitImmediately;
}

void KPsionMainWindow::
syncTime(QString uid) {
}

void KPsionMainWindow::
queryPsion() {
    u_int32_t devbits;
    Enum <rfsv::errs> res;

    statusBar()->changeItem(i18n("Retrieving machine info ..."),
			    STID_CONNECTION);

    Enum<rpcs::machs> machType;
    if (plpRpcs->getMachineType(machType) != rfsv::E_PSI_GEN_NONE) {
	QString msg = i18n("Could not get Psion machine type");
	statusBar()->changeItem(msg, STID_CONNECTION);
	KMessageBox::error(this, msg);
	return;
    }
    if (machType == rpcs::PSI_MACH_S5) {
	rpcs::machineInfo mi;
	if ((res = plpRpcs->getMachineInfo(mi)) != rfsv::E_PSI_GEN_NONE) {
	    QString msg = i18n("Could not get Psion machine info");
	    statusBar()->changeItem(msg, STID_CONNECTION);
	    KMessageBox::error(this, msg);
	    return;
	}
	machineUID = mi.machineUID;
	S5mx = (strcmp(mi.machineName, "SERIES5mx") == 0);
    } else {
	// On a SIBO, first check for a file 'SYS$PT.CFG' on the default
	// directory. If it exists, read the UID from it. Otherwise
	// calculate a virtual machine UID from the OwnerInfo data and
	// write it to that file.
	bufferArray b;
	u_int32_t handle;
	u_int32_t count;

	res = plpRfsv->fopen(plpRfsv->opMode(rfsv::PSI_O_RDONLY),
			     "SYS$PT.CFG", handle);
	if (res == rfsv::E_PSI_GEN_NONE) {
	    res = plpRfsv->fread(handle, (unsigned char *)&machineUID,
				 sizeof(machineUID), count);
	    plpRfsv->fclose(handle);
	    if ((res != rfsv::E_PSI_GEN_NONE) || (count != sizeof(machineUID))) {
		QString msg = i18n("Could not read SIBO UID file");
		statusBar()->changeItem(msg, STID_CONNECTION);
		KMessageBox::error(this, msg);
		return;
	    }
	} else {
	    if ((res = plpRpcs->getOwnerInfo(b)) != rfsv::E_PSI_GEN_NONE) {
		QString msg = i18n("Could not get Psion owner info");
		statusBar()->changeItem(msg, STID_CONNECTION);
		KMessageBox::error(this, msg);
		return;
	    }
	    machineUID = 0;
	    string oi = "";
	    while (!b.empty()) {
		oi += b.pop().getString();
		oi += "\n";
	    }
	    const char *p = oi.c_str();
	    unsigned long long z;
	    int i = 0;

	    while (*p) {
                z = *p;
                machineUID ^= (z << i);
                p++; i++;
                i &= ((sizeof(machineUID) * 8) - 1);
	    }
	    res = plpRfsv->fcreatefile(plpRfsv->opMode(rfsv::PSI_O_RDWR),
				 "SYS$PT.CFG", handle);
	    if (res == rfsv::E_PSI_GEN_NONE) {
		res = plpRfsv->fwrite(handle, (const unsigned char *)&machineUID,
				      sizeof(machineUID), count);
		plpRfsv->fclose(handle);
	    }
	    if (res != rfsv::E_PSI_GEN_NONE) {
		QString msg = i18n("Could not write SIBO UID file %1").arg((const char *)res);
		statusBar()->changeItem(msg, STID_CONNECTION);
		KMessageBox::error(this, msg);
		return;
	    }
        }
	S5mx = false;
    }

    QString uid = getMachineUID();
    bool machineFound = false;
    KConfig *config = kapp->config();
    KPsionConfig pcfg;

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPDRIVES));
    QString tmp = pcfg.getOptionName(KPsionConfig::OPT_BACKUPDRIVES);
    machineName = i18n("an unknown machine");
    psionMap::Iterator it;
    for (it = machines.begin(); it != machines.end(); it++) {
	if (uid == it.key()) {
	    machineName = it.data();
	    backupDrives = config->readListEntry(tmp.arg(it.key()));
	    machineFound = true;
	}
    }

    drives.clear();
    statusBar()->changeItem(i18n("Retrieving drive list ..."),
			    STID_CONNECTION);
    if ((res = plpRfsv->devlist(devbits)) != rfsv::E_PSI_GEN_NONE) {
	QString msg = i18n("Could not get list of drives");
	statusBar()->changeItem(msg, STID_CONNECTION);
	KMessageBox::error(this, msg);
	return;
    }
    for (int i = 0; i < 26; i++) {
	if ((devbits & 1) != 0) {
	    PlpDrive drive;
	    if (plpRfsv->devinfo('A' + i, drive) == rfsv::E_PSI_GEN_NONE)
		insertDrive('A' + i, drive.getName().c_str());
	}
	devbits >>= 1;
    }

    if (!machineFound) {
	if (args->isSet("autobackup")) {
	    connected = false;
	    if (plpRfsv)
		delete plpRfsv;
	    if (plpRpcs)
		delete plpRpcs;
	    if (rfsvSocket)
		delete rfsvSocket;
	    if (rfsvSocket)
		delete rpcsSocket;
	    quitImmediately = true;
	    return;
	}
	NewPsionWizard *wiz = new NewPsionWizard(this, "newpsionwiz");
	wiz->exec();
    }
    statusBar()->changeItem(i18n("Connected to %1").arg(machineName),
			    STID_CONNECTION);

    syncTime(uid);

    if (args->isSet("autobackup")) {
	// Check, if scheduled backups to perform

	doScheduledBackup = false;
	KPsionConfig pcfg;
	QIconViewItem *i;
	QStringList::Iterator it;
	QDateTime d;
	KConfig *config = kapp->config();
	QString uid = getMachineUID();
	QString key;
	int fi = pcfg.getIntervalDays(config, KPsionConfig::OPT_FULLINTERVAL);
	int ii = pcfg.getIntervalDays(config, KPsionConfig::OPT_INCINTERVAL);

	// Check for Full Backup
	config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_LASTFULL));
	for (it = backupDrives.begin(); it != backupDrives.end(); ++it) {
	    key =
		pcfg.getOptionName(KPsionConfig::OPT_LASTFULL).arg(uid).arg(*it);
	    d.setTime_t(config->readNumEntry(key));

	    if (fi && d.daysTo(QDateTime::currentDateTime()) >= fi) {
		fullBackup = true;
		for (i = view->firstItem(); i; i = i->nextItem()) {
		    if (i->key() == *it) {
			i->setSelected(true);
			doScheduledBackup = true;
		    }
		}
	    }
	}
	if (!doScheduledBackup) {
	    // Check for Incremental Backup
	    fullBackup = false;
	    view->clearSelection();
	    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_LASTINC));
	    for (it = backupDrives.begin(); it != backupDrives.end(); ++it) {
		key =
		    pcfg.getOptionName(KPsionConfig::OPT_LASTINC).arg(uid).arg(*it);
		d.setTime_t(config->readNumEntry(key));
		if (ii && d.daysTo(QDateTime::currentDateTime()) >= ii) {
		    for (i = view->firstItem(); i; i = i->nextItem()) {
			if (i->key() == *it) {
			    i->setSelected(true);
			    doScheduledBackup = true;
			}
		    }
		}
	    }
	}
	if (!doScheduledBackup) {
	    quitImmediately = true;
	    return;
	}

    }
    if (doScheduledBackup || args->isSet("backup") ||
	args->isSet("restore") || args->isSet("format")) {
	view->setEnabled(false);
	actionCollection()->action("restore")->setEnabled(false);
	actionCollection()->action("format")->setEnabled(false);
	actionCollection()->action("fullbackup")->setEnabled(false);
	actionCollection()->action("incbackup")->setEnabled(false);
	QTimer::singleShot(1000, this, SLOT(slotAutoAction()));
    }
}

void KPsionMainWindow::
slotAutoAction() {
    QIconViewItem *i;

    if (doScheduledBackup) {
	doBackup();
	QTimer::singleShot(1000, this, SLOT(close()));
	return;
    }

    if (args->isSet("backup")) {
	bool any = false;

	QCStringList argl = args->getOptionList("backup");
	QCStringList::Iterator it;

	for (it = argl.begin(); it != argl.end(); ++it) {
	    QString drv((*it).upper());

	    for (i = view->firstItem(); i; i = i->nextItem()) {
		if (i->key() == drv) {
		    i->setSelected(true);
		    any = true;
		}
	    }
	}
	if (any) {
	    fullBackup = true;
	    doBackup();
	}
	QTimer::singleShot(1000, this, SLOT(close()));
	return;
    }

    if (args->isSet("restore")) {
	bool any = false;

	QCStringList argl = args->getOptionList("restore");
	QCStringList::Iterator it;

	for (it = argl.begin(); it != argl.end(); ++it) {
	    QString drv((*it).upper());
	    if (drv == "Z") {
		KMessageBox::sorry(this, i18n(
		    "<QT>The selected drive <B>Z:</B> is "
		    "a <B>ROM</B> drive and therefore cannot be restored.</QT>"));
		continue;
	    }
	    for (i = view->firstItem(); i; i = i->nextItem()) {
		if (i->key() == drv) {
		    i->setSelected(true);
		    any = true;
		}
	    }
	}
	if (any)
	    slotStartRestore();
	QTimer::singleShot(1000, this, SLOT(close()));
	return;
    }

    if (args->isSet("format")) {
	bool any = false;
	QCStringList argl = args->getOptionList("format");
	QCStringList::Iterator it;

	for (it = argl.begin(); it != argl.end(); ++it) {
	    QString drv((*it).upper());
	    for (i = view->firstItem(); i; i = i->nextItem()) {
		if (i->key() == drv) {
		    i->setSelected(true);
		    any = true;
		}
	    }
	}
	if (any)
	    slotStartFormat();
	QTimer::singleShot(1000, this, SLOT(close()));
	return;
    }
    QTimer::singleShot(1000, this, SLOT(close()));
}

QString KPsionMainWindow::
getMachineUID() {
    // ??! None of QString's formatting methods knows about long long.
    char tmp[20];
    sprintf(tmp, "%16llx", machineUID);
    return QString(tmp);
}

bool KPsionMainWindow::
queryClose() {
    QString msg = 0L;

    if (backupRunning)
	msg = i18n("A backup is running.\nDo you really want to quit?");
    if (restoreRunning)
	msg = i18n("A restore is running.\nDo you really want to quit?");
    if (formatRunning)
	msg = i18n("A format is running.\nDo you really want to quit?");

    if ((!msg.isNull()) &&
	(KMessageBox::warningYesNo(this, msg) == KMessageBox::No))
	return false;
    return true;
}

void KPsionMainWindow::
startupNcpd() {
    if (ncpdDevice == "off")
	return;
    KProcess proc;
    ppsocket *testSocket;
    time_t start_time = time(0L) + 2;
    bool connectOk = false;

    testSocket = new ppsocket();
    if (!testSocket->connect(NULL, 7501)) {

	statusBar()->changeItem(i18n("Starting ncpd daemon ..."),
				STID_CONNECTION);
	proc << ncpdPath;
	proc << "-s" << ncpdDevice << "-b" << ncpdSpeed;
	proc.start(KProcess::DontCare);
	while ((time(0L) < start_time) &&
	       (!(connectOk = testSocket->connect(NULL, 7501))))
	    kapp->processEvents();
    }
    delete testSocket;
    if (connectOk) {
	// 2 more seconds for ncpd to negotiate with the Psion.
	start_time = time(0L) + 2;
	while (time(0L) < start_time)
	    kapp->processEvents();
    }
}

void KPsionMainWindow::
tryConnect() {
    if (shuttingDown || connected)
	return;
    bool showMB = firstTry;
    firstTry = false;

    if (plpRfsv)
	delete plpRfsv;
    if (plpRpcs)
	delete plpRpcs;
    if (rfsvSocket)
	delete rfsvSocket;
    if (rfsvSocket)
	delete rpcsSocket;

    startupNcpd();
    rfsvSocket = new ppsocket();
    statusBar()->changeItem(i18n("Connecting ..."), STID_CONNECTION);
    if (!rfsvSocket->connect(NULL, 7501)) {
	if (args->isSet("autobackup")) {
	    quitImmediately = true;
	    return;
	}
	statusMsg = i18n("RFSV could not connect to ncpd at %1:%2. ").arg("localhost").arg(7501);
	if (reconnectTime) {
	    nextTry = reconnectTime;
	    statusMsg += i18n(" (Retry in %1 seconds.)");
	    QTimer::singleShot(1000, this, SLOT(slotUpdateTimer()));

	    statusBar()->changeItem(statusMsg.arg(reconnectTime),
				    STID_CONNECTION);
	    if (showMB)
		KMessageBox::error(this, statusMsg.arg(reconnectTime));
	} else {
	    statusBar()->changeItem(statusMsg, STID_CONNECTION);
	    if (showMB)
		KMessageBox::error(this, statusMsg);
	}
	return;
    }
    rfsvfactory factory(rfsvSocket);
    plpRfsv = factory.create(false);
    if (plpRfsv == 0L) {
	if (args->isSet("autobackup")) {
	    quitImmediately = true;
	    return;
	}
	statusMsg = i18n("RFSV could not establish link: %1.").arg(KGlobal::locale()->translate(factory.getError()));
	delete rfsvSocket;
	rfsvSocket = 0L;
	if (reconnectTime) {
	    nextTry = reconnectTime;
	    statusMsg += i18n(" (Retry in %1 seconds.)");
	    QTimer::singleShot(1000, this, SLOT(slotUpdateTimer()));
	    statusBar()->changeItem(statusMsg.arg(reconnectTime),
				    STID_CONNECTION);
	    if (showMB)
		KMessageBox::error(this, statusMsg.arg(reconnectTime));
	} else {
	    statusBar()->changeItem(statusMsg, STID_CONNECTION);
	    if (showMB)
		KMessageBox::error(this, statusMsg);
	}
	return;
    }

    rpcsSocket = new ppsocket();
    if (!rpcsSocket->connect(NULL, 7501)) {
	if (args->isSet("autobackup")) {
	    quitImmediately = true;
	    return;
	}
	statusMsg = i18n("RPCS could not connect to ncpd at %1:%2.").arg("localhost").arg(7501);
	delete plpRfsv;
	plpRfsv = 0L;
	delete rfsvSocket;
	rfsvSocket = 0L;
	if (reconnectTime) {
	    nextTry = reconnectTime;
	    statusMsg += i18n(" (Retry in %1 seconds.)");
	    QTimer::singleShot(1000, this, SLOT(slotUpdateTimer()));
	    statusBar()->changeItem(statusMsg.arg(reconnectTime),
				    STID_CONNECTION);
	    if (showMB)
		KMessageBox::error(this, statusMsg.arg(reconnectTime));
	} else {
	    statusBar()->changeItem(statusMsg, STID_CONNECTION);
	    if (showMB)
		KMessageBox::error(this, statusMsg);
	}
	return;
    }
    rpcsfactory factory2(rpcsSocket);
    plpRpcs = factory2.create(false);
    if (plpRpcs == 0L) {
	if (args->isSet("autobackup")) {
	    quitImmediately = true;
	    return;
	}
	statusMsg = i18n("RPCS could not establish link: %1.").arg(KGlobal::locale()->translate(factory2.getError()));
	delete plpRfsv;
	plpRfsv = 0L;
	delete rfsvSocket;
	rfsvSocket = 0L;
	delete rpcsSocket;
	rpcsSocket = 0L;
	if (reconnectTime) {
	    nextTry = reconnectTime;
	    statusMsg += i18n(" (Retry in %1 seconds.)");
	    QTimer::singleShot(1000, this, SLOT(slotUpdateTimer()));
	    statusBar()->changeItem(statusMsg.arg(reconnectTime),
				    STID_CONNECTION);
	    if (showMB)
		KMessageBox::error(this, statusMsg.arg(reconnectTime));
	} else {
	    statusBar()->changeItem(statusMsg, STID_CONNECTION);
	    if (showMB)
		KMessageBox::error(this, statusMsg);
	}
	return;
    }
    connected = true;
    queryPsion();
}

void KPsionMainWindow::
slotUpdateTimer() {
    nextTry--;
    if (nextTry <= 0)
	tryConnect();
    else {
	statusBar()->changeItem(statusMsg.arg(nextTry), STID_CONNECTION);
	QTimer::singleShot(1000, this, SLOT(slotUpdateTimer()));
    }
}

void KPsionMainWindow::
slotProgressBarPressed() {
}

void KPsionMainWindow::
slotStartFullBackup() {
    fullBackup = true;
    doBackup();
}

void KPsionMainWindow::
slotStartIncBackup() {
    fullBackup = false;
    doBackup();
}

const KTarEntry *KPsionMainWindow::
findTarEntry(const KTarEntry *te, QString path, QString rpath) {
    const KTarEntry *fte = NULL;
    if (te->isDirectory() && (path.left(rpath.length()) == rpath)) {
	KTarDirectory *td = (KTarDirectory *)te;
	QStringList files = td->entries();
	for (QStringList::Iterator f = files.begin(); f != files.end(); f++) {
	    QString tmp = rpath;
	    if (tmp.length())
		tmp += "/";
	    tmp += *f;
	    fte = findTarEntry(td->entry(*f), path, tmp);
	    if (fte != 0L)
		break;
	}
    } else {
	if (path == rpath)
	    fte = te;
    }
    return fte;
}

void KPsionMainWindow::
updateBackupStamps() {
    KConfig *config = kapp->config();
    KPsionConfig pcfg;
    QString uid = getMachineUID();
    int cfgBtype = (fullBackup)
	? KPsionConfig::OPT_LASTFULL : KPsionConfig::OPT_LASTINC;

    config->setGroup(pcfg.getSectionName(cfgBtype));
    for (QIconViewItem *i = view->firstItem(); i; i = i->nextItem()) {
	QString key = pcfg.getOptionName(cfgBtype).arg(uid).arg(i->key());
	if (i->isSelected())
	    config->writeEntry(key, time(0));
    }
}

void KPsionMainWindow::
doBackup() {
    backupRunning = true;
    switchActions();
    QStringList processDrives;
    toBackup.clear();

    // Collect list of files to backup
    backupSize = 0;
    backupCount = 0;
    progressTotal = 0;
    emit enableProgressText(true);
    emit setProgress(0);
    for (QIconViewItem *i = view->firstItem(); i; i = i->nextItem()) {
	if (i->isSelected()) {
	    QString drv = i->key();
	    drv += ":";
	    int drvChar = drv[0].latin1();
	    PlpDrive drive;
	    if (plpRfsv->devinfo(drvChar, drive) != rfsv::E_PSI_GEN_NONE) {
		statusBar()->changeItem(i18n("Connected to %1").arg(machineName),
					STID_CONNECTION);
		emit enableProgressText(false);
		emit setProgress(0);
		KMessageBox::error(this, i18n("Could not retrieve drive details for drive %1").arg(drv));
		backupRunning = false;
		switchActions();
		return;
	    }
	    emit setProgressText(i18n("Scanning drive %1").arg(drv));

	    progressLocal = drive.getSize() - drive.getSpace();
	    progressLocalCount = 0;
	    progressLocalPercent = -1;
	    progress->setValue(0);
	    collectFiles(drv);
	    processDrives += drv;
	}
    }
    emit setProgressText(i18n("%1 files need backup").arg(backupSize));
    if (backupCount == 0) {
	emit enableProgressText(false);
	emit setProgress(0);
	statusBar()->changeItem(i18n("Connected to %1").arg(machineName),
				STID_CONNECTION);
	updateBackupStamps();
	if (!args->isSet("autobackup"))
	    KMessageBox::information(this, i18n("No files need backup"));
	backupRunning = false;
	switchActions();
	return;
    }

    progressCount = 0;
    progressTotal = backupSize;
    progressPercent = -1;

    // Create tgz with index file.
    QString archiveName = backupDir;
    if (archiveName.right(1) != "/")
	archiveName += "/";
    archiveName += getMachineUID();
    QDir archiveDir(archiveName);
    if (!archiveDir.exists())
	if (!archiveDir.mkdir(archiveName)) {
	    emit enableProgressText(false);
	    emit setProgress(0);
	    statusBar()->changeItem(i18n("Connected to %1").arg(machineName),
				    STID_CONNECTION);
	    KMessageBox::error(this, i18n("Could not create backup folder %1").arg(archiveName));
	    backupRunning = false;
	    switchActions();
	    return;
	}

    archiveName += (fullBackup) ? "/F-" : "/I-";
    time_t now = time(0);
    char tstr[30];
    strftime(tstr, sizeof(tstr), "%Y-%m-%d-%H-%M-%S.tmp.gz",
	     localtime(&now));
    archiveName += tstr;
    backupTgz = new KTarGz(archiveName);
    backupTgz->open(IO_WriteOnly);
    createIndex();

    // Kill all running applications on the Psion
    // and save their state.
    killSave();

    bool badBackup = false;
    Enum<rfsv::errs> res;
    // Now the real backup
    progressTotalText = i18n("Backup %1% done");
    for (int i = 0; i < toBackup.size(); i++) {
	PlpDirent e = toBackup[i];
	const char *fn = e.getName();
	QString unixname = psion2unix(fn);
	QByteArray ba;
	QDataStream os(ba, IO_WriteOnly);

	emit setProgressText(QString("%1").arg(fn));
	progressLocal = e.getSize();
	progressLocalCount = 0;
	progressLocalPercent = -1;
	emit setProgress(0);

	u_int32_t handle;

	kapp->processEvents();
	bool tryLoop = true;
	do {
	    res = plpRfsv->fopen(plpRfsv->opMode(rfsv::PSI_O_RDONLY), fn,
				 handle);
	    if (res == rfsv::E_PSI_GEN_NONE) {
		unsigned char *buff = new unsigned char[RFSV_SENDLEN];
		u_int32_t len;
		do {
		    if ((res = plpRfsv->fread(handle, buff, RFSV_SENDLEN,
					      len)) == rfsv::E_PSI_GEN_NONE) {
			os.writeRawBytes((char *)buff, len);
			updateProgress(len);
		    }
		} while ((len > 0) && (res == rfsv::E_PSI_GEN_NONE));
		delete[]buff;
		plpRfsv->fclose(handle);
	    }
	    if (res != rfsv::E_PSI_GEN_NONE) {
		switch (KMessageBox::warningYesNoCancel(
			    this, i18n(
				"<QT>Could not backup<BR/><B>%1</B><BR/>"
				"<FONT COLOR=RED>%2</FONT><BR/></QT>"
				).arg(fn).arg((const char *)res),
			    QString::null, i18n("Retry"), i18n("Ignore"))) {
		    case KMessageBox::Cancel:
			badBackup = true;
			tryLoop = false;
			break;
		    case KMessageBox::No:
			e.setName("!");
			tryLoop = false;
			break;
		    case KMessageBox::Yes:
			break;
		}
	    } else {
		tryLoop = false;
	    }
	} while (tryLoop);
	if (badBackup)
	    break;
	if (res != rfsv::E_PSI_GEN_NONE)
	    continue;
	backupTgz->writeFile(unixname, "root", "root", ba.size(), ba.data());
    }

    if (!badBackup) {
	// Reset archive attributes of all backuped files.
	emit setProgressText(i18n("Resetting archive attributes"));
	progressLocal = backupSize;
	progressLocalCount = 0;
	progressLocalPercent = -1;
	emit setProgress(0);
	kapp->processEvents();
	progressTotal = 0;
	for (int i = 0; i < toBackup.size(); i++) {
	    PlpDirent e = toBackup[i];
	    const char *fn = e.getName();
	    if ((e.getAttr() & rfsv::PSI_A_ARCHIVE) &&
		(strcmp(fn, "!") != 0)) {
		kapp->processEvents();
		res = plpRfsv->fsetattr(fn, 0, rfsv::PSI_A_ARCHIVE);
		if (res != rfsv::E_PSI_GEN_NONE) {
		    if (KMessageBox::warningYesNo(this, i18n("<QT>Could not set attributes of<BR/><B>%1</B><BR/><FONT COLOR=red>%2</FONT><BR/>Continue?</QT>").arg(fn)) == KMessageBox::No) {
			break;
		    }
		}
	    }
	    updateProgress(e.getSize());
	}
	updateBackupStamps();
    }
    // Restart previously running applications on the Psion
    // from saved state info.
    runRestore();

    backupTgz->close();
    delete backupTgz;
    emit enableProgressText(false);
    emit setProgress(0);

    if (badBackup)
	::unlink(archiveName.latin1());
    else {
	QString newName = archiveName;
	newName.replace(QRegExp("\\.tmp\\.gz$"), ".tar.gz");
	// Rename Tarfile to its final name;
	if (::rename(archiveName.latin1(), newName.latin1()) != 0)
	    KMessageBox::sorry(this, i18n("<QT>Could not rename backup archive from<BR/><B>%1</B> to<BR/><B>%2</B></QT>").arg(archiveName).arg(newName));
	else
	    removeOldBackups(processDrives);
    }

    backupRunning = false;
    switchActions();
    statusBar()->changeItem(i18n("Connected to %1").arg(machineName),
			    STID_CONNECTION);
    emit enableProgressText(false);
    emit setProgress(0);
    statusBar()->message(i18n("Backup done"), 2000);
}

class Barchive {
public:
    Barchive()
	: n(""), d(0) {}
    Barchive(const QString &name, time_t date)
	: n(name), d(date) {}

    QString name() const { return n; }
    time_t date() const { return d; }
    bool operator==(const Barchive &a) { return (a.n == n); }
private:
    QString n;
    time_t d;
};

typedef QValueList<Barchive>ArchList;

void KPsionMainWindow::
removeOldBackups(QStringList &drives) {

    if (!fullBackup)
	return;

    KConfig *config = kapp->config();
    KPsionConfig pcfg;

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPGEN));
    int bgen = config->readNumEntry(
	pcfg.getOptionName(KPsionConfig::OPT_BACKUPGEN));

    if (bgen == 0)
	return;

    statusBar()->changeItem(i18n("Removing old backups ..."), STID_CONNECTION);
    QString bdir(backupDir);
    bdir += "/";
    bdir += getMachineUID();
    QDir d(bdir);
    kapp->processEvents();
    const QFileInfoList *fil =
	d.entryInfoList("*.tar.gz", QDir::Files|QDir::Readable, QDir::Name);
    QFileInfoListIterator it(*fil);
    QFileInfo *fi;
    ArchList alist;

    // Build a list of full-backups sorted by date
    while ((fi = it.current())) {
	kapp->processEvents();

	KTarGz tgz(fi->absFilePath());
	const KTarEntry *te;

	tgz.open(IO_ReadOnly);
	te = tgz.directory()->entry("KPsionFullIndex");
	if (te && (!te->isDirectory())) {
	    for (QStringList::Iterator d = drives.begin(); d != drives.end();
		 d++) {
		const KTarEntry *de = tgz.directory()->entry(*d);
		if (de && (de->isDirectory())) {
		    Barchive a(tgz.fileName(), te->date());
		    if (!alist.contains(a)) {
			if (alist.isEmpty() || (alist.first().date()>te->date()))
			    alist.prepend(a);
			else
			    alist.append(a);
			}
		}
	    }
	}
	tgz.close();
	++it;
    }

    // Remove entries from the beginning of the list if there are more than
    // bgen entries. This leaves at most bgen of the youngest backups in the
    // list.
    while (alist.count() > bgen) {
	Barchive r = alist.first();
	alist.remove(r);
    }

    // Finally iterate over all backups and delete those which are older
    // than the first entry in alist.

    (void)it.toFirst();

    while ((fi = it.current())) {
	kapp->processEvents();

	KTarGz tgz(fi->absFilePath());
	const KTarEntry *te;
	bool valid = false;
	bool del = false;

	tgz.open(IO_ReadOnly);
	te = tgz.directory()->entry("KPsionFullIndex");
	if (te && (!te->isDirectory()))
	    valid = true;
	else {
	    te = tgz.directory()->entry("KPsionIncrementalIndex");
	    if (te && (!te->isDirectory()))
		valid = true;
	}
	if (valid) {
	    Barchive a(tgz.fileName(), te->date());
	    if (alist.isEmpty() ||
		((!alist.contains(a)) && (te->date() < alist.first().date())))
		del = true;
	}
	tgz.close();
	if (del)
	    ::remove(fi->absFilePath().data());
	++it;
    }
}

bool KPsionMainWindow::
askOverwrite(PlpDirent e) {
    if (overWriteAll)
	return true;
    const char *fn = e.getName();
    if (overWriteList.contains(QString(fn)))
	return true;
    PlpDirent old;
    Enum<rfsv::errs> res = plpRfsv->fgeteattr(fn, old);
    if (res != rfsv::E_PSI_GEN_NONE) {
	KMessageBox::error(this, i18n(
	    "<QT>Could not get attributes of<BR/>"
	    "<B>%1</B><BR/>Reason: %2</QT>").arg(fn).arg(KGlobal::locale()->translate(res)));
	return false;
    }

    // Don't ask if size and attribs are same
    if ((old.getSize() == e.getSize()) &&
	((old.getAttr() & ~rfsv::PSI_A_ARCHIVE) ==
	 (e.getAttr() & ~rfsv::PSI_A_ARCHIVE)))
	return true;

    QDateTime odate;
    QDateTime ndate;

    odate.setTime_t(old.getPsiTime().getTime());
    ndate.setTime_t(e.getPsiTime().getTime());

    // Dates
    QString od = KGlobal::locale()->formatDateTime(odate, false);
    QString nd = KGlobal::locale()->formatDateTime(ndate, false);

    // Sizes
    QString os = QString("%1 (%2)").arg(KIO::convertSize(old.getSize())).arg(KGlobal::locale()->formatNumber(old.getSize(), 0));
    QString ns = QString("%1 (%2)").arg(KIO::convertSize(e.getSize())).arg(KGlobal::locale()->formatNumber(e.getSize(), 0));

    // Attributes
    QString oa(plpRfsv->attr2String(old.getAttr()).c_str());
    QString na(plpRfsv->attr2String(e.getAttr()).c_str());

    KDialogBase dialog(i18n("Overwrite"), KDialogBase::Yes | KDialogBase::No |
		       KDialogBase::Cancel, KDialogBase::No,
		       KDialogBase::Cancel, this, "overwriteDialog", true, true,
		       QString::null, QString::null, i18n("Overwrite &All"));

    QWidget *contents = new QWidget(&dialog);
    QHBoxLayout * lay = new QHBoxLayout(contents);
    lay->setSpacing(KDialog::spacingHint()*2);
    lay->setMargin(KDialog::marginHint()*2);

    lay->addStretch(1);
    QLabel *label1 = new QLabel(contents);
    label1->setPixmap(QMessageBox::standardIcon(QMessageBox::Warning,
						kapp->style().guiStyle()));
    lay->add(label1);
    lay->add(new QLabel(i18n(
	"<QT>The file <B>%1</B> exists already on the Psion with "
	"different size and/or attributes."
	"<P><B>On the Psion:</B><BR/>"
	"  Size: %2<BR/>"
	"  Date: %3<BR/>"
	"  Attributes: %4</P>"
	"<P><B>In backup:</B><BR/>"
	"  Size: %5<BR/>"
	"  Date: %6<BR/>"
	"  Attributes: %7</P>"
	"Do you want to overwrite it?</QT>").arg(fn).arg(os).arg(od).arg(oa).arg(ns).arg(nd).arg(na), contents));
    lay->addStretch(1);

    dialog.setMainWidget(contents);
    dialog.enableButtonSeparator(false);

    int result = dialog.exec();
    switch (result) {
	case KDialogBase::Yes:
	    return true;
	case KDialogBase::No:
	    return false;
	case KDialogBase::Cancel:
	    overWriteAll = true;
	    return true;
	default: // Huh?
	    break;
    }

    return false; // Default
}

void KPsionMainWindow::
slotStartRestore() {
    restoreRunning = true;
    switchActions();

    kapp->setOverrideCursor(Qt::waitCursor);
    statusBar()->changeItem(i18n("Reading backups ..."), STID_CONNECTION);
    update();
    kapp->processEvents();
    KPsionRestoreDialog restoreDialog(this, getMachineUID());
    kapp->restoreOverrideCursor();
    statusBar()->changeItem(i18n("Selecting backups ..."), STID_CONNECTION);

    for (QIconViewItem *i = view->firstItem(); i; i = i->nextItem()) {
	if (i->isSelected() && (i->key() != "Z"))
	    restoreDialog.autoSelect(i->key());
    }

    overWriteList.clear();
    overWriteAll = false;

    if (restoreDialog.exec() == KDialogBase::Accepted) {
	QStringList tars = restoreDialog.getSelectedTars();
	QStringList fmtDrives = restoreDialog.getFormatDrives();
	QStringList::Iterator t;

	backupSize = 0;
	backupCount = 0;
	for (t = tars.begin(); t != tars.end(); t++) {
	    PlpDir toRestore = restoreDialog.getRestoreList(*t);
	    for (int r = 0; r < toRestore.size(); r++) {
		PlpDirent e = toRestore[r];
		backupSize += e.getSize();
		backupCount++;
	    }
	}
	if (backupCount == 0) {
	    restoreRunning = false;
	    switchActions();
	    statusBar()->changeItem(i18n("Connected to %1").arg(machineName),
				    STID_CONNECTION);
	    return;
	}

	progressTotalText = i18n("Restore %1% done");
	progressTotal = backupSize;
	progressCount = 0;
	progressPercent = -1;
	emit setProgressText("");
	emit enableProgressText(true);
	emit setProgress(0);

	// Kill all running applications on the Psion
	// and save their state.
	killSave();

	for (t = tars.begin(); t != tars.end(); t++) {
	    PlpDir toRestore = restoreDialog.getRestoreList(*t);
	    if (toRestore.size() > 0) {
		KTarGz tgz(*t);
		const KTarEntry *te;
		QString pDir("");

		tgz.open(IO_ReadOnly);
		for (int r = 0; r < toRestore.size(); r++) {
		    PlpDirent e = toRestore[r];
		    PlpDirent olde;

		    const char *fn = e.getName();
		    QString unixname = psion2unix(fn);
		    Enum<rfsv::errs> res;

		    progressLocal = e.getSize();
		    progressLocalCount = 0;
		    progressLocalPercent = -1;
		    emit setProgressText(QString("%1").arg(fn));
		    emit setProgress(0);

		    te = findTarEntry(tgz.directory(), unixname);
		    if (te != 0L) {
			u_int32_t handle;
			QString cpDir(fn);
			QByteArray ba = ((KTarFile *)te)->data();
			int bslash = cpDir.findRev('\\');
			if (bslash > 0)
			    cpDir = cpDir.left(bslash);

			QString drv = cpDir.left(1);
			if (fmtDrives.find(drv) != fmtDrives.end()) {
			    int tSave = progressCount;
			    doFormat(drv);
			    fmtDrives.remove(drv);
			    progressTotalText = i18n("Restore %1% done");
			    progressTotal = backupSize;
			    progressCount = tSave;
			    progressLocal = e.getSize();
			    progressLocalCount = 0;
			    progressLocalPercent = -1;
			    emit setProgressText(QString("%1").arg(fn));
			    emit enableProgressText(true);
			    emit setProgress(0);
			}
			if (pDir != cpDir) {
			    pDir = cpDir;
			    res = plpRfsv->mkdir(pDir);
			    if ((res != rfsv::E_PSI_GEN_NONE) &&
				(res != rfsv::E_PSI_FILE_EXIST)) {
				KMessageBox::error(this, i18n(
				    "<QT>Could not create directory<BR/>"
				    "<B>%1</B><BR/>Reason: %2</QT>").arg(pDir).arg(KGlobal::locale()->translate(res)));
				continue;
			    }
			}
			res = plpRfsv->fcreatefile(
			    plpRfsv->opMode(rfsv::PSI_O_RDWR), fn, handle);
			if (res == rfsv::E_PSI_FILE_EXIST) {
			    if (!askOverwrite(e))
				continue;
			    res = plpRfsv->freplacefile(
				plpRfsv->opMode(rfsv::PSI_O_RDWR), fn, handle);
			}
			if (res != rfsv::E_PSI_GEN_NONE) {
			    KMessageBox::error(this, i18n(
				"<QT>Could not create<BR/>"
				"<B>%1</B><BR/>Reason: %2</QT>").arg(fn).arg(KGlobal::locale()->translate(res)));
			    continue;
			}
			const unsigned char *data =
			    (const unsigned char *)(ba.data());
			long len = ba.size();

			do {
			    u_int32_t written;
			    u_int32_t count =
				(len > RFSV_SENDLEN) ? RFSV_SENDLEN : len;
			    res = plpRfsv->fwrite(handle, data, count, written);
			    if (res != rfsv::E_PSI_GEN_NONE)
				break;
			    len -= written;
			    data += written;
			    updateProgress(written);
			} while (len > 0);
			plpRfsv->fclose(handle);
			if (res != rfsv::E_PSI_GEN_NONE) {
			    KMessageBox::error(this, i18n(
				"<QT>Could not write to<BR/>"
				"<B>%1</B><BR/>Reason: %2</QT>").arg(fn).arg(KGlobal::locale()->translate(res)));
			    continue;
			}
			u_int32_t oldattr;
			res = plpRfsv->fgetattr(fn, oldattr);
			if (res != rfsv::E_PSI_GEN_NONE) {
			    KMessageBox::error(this, i18n(
				"<QT>Could not get attributes of<BR/>"
				"<B>%1</B><BR/>Reason: %2</QT>").arg(fn).arg(KGlobal::locale()->translate(res)));
			    continue;
			}
			u_int32_t mask = e.getAttr() ^ oldattr;
			u_int32_t sattr = e.getAttr() & mask;
			u_int32_t dattr = ~sattr & mask;
			int retry = 10;
			// Retry, because file somtimes takes some time
			// to close;
			do {
			    res = plpRfsv->fsetattr(fn, sattr, dattr);
			    if (res != rfsv::E_PSI_GEN_NONE)
				usleep(100000);
			    retry--;
			} while ((res != rfsv::E_PSI_GEN_NONE) && (retry > 0));
			if (res != rfsv::E_PSI_GEN_NONE) {
			    KMessageBox::error(this, i18n(
				"<QT>Could not set attributes of<BR/>"
				"<B>%1</B><BR/>Reason: %2</QT>").arg(fn).arg(KGlobal::locale()->translate(res)));
			    continue;
			}
			retry = 10;
			do {
			    res = plpRfsv->fsetmtime(fn, e.getPsiTime());
			    if (res != rfsv::E_PSI_GEN_NONE)
				usleep(100000);
			    retry--;
			} while ((res != rfsv::E_PSI_GEN_NONE) && (retry > 0));
			if (res != rfsv::E_PSI_GEN_NONE) {
			    KMessageBox::error(this, i18n(
				"<QT>Could not set modification time of<BR/>"
				"<B>%1</B><BR/>Reason: %2</QT>").arg(fn).arg(KGlobal::locale()->translate(res)));
			    continue;
			}
			overWriteList += QString(fn);
		    }
		}
		tgz.close();
	    }
	}
	// Restart previously running applications on the Psion
	// from saved state info.
	runRestore();
    } else {
	restoreRunning = false;
	switchActions();
	statusBar()->changeItem(i18n("Connected to %1").arg(machineName),
				STID_CONNECTION);
	return;
    }

    restoreRunning = false;
    switchActions();
    emit setProgress(0);
    emit enableProgressText(false);
    statusBar()->changeItem(i18n("Connected to %1").arg(machineName),
			    STID_CONNECTION);
    statusBar()->message(i18n("Restore done"), 2000);
}

void KPsionMainWindow::
doFormat(QString drive) {
    int handle;
    int count;
    const char dchar = drive[0].latin1();
    QString dname("");

    PlpDrive drv;
    if (plpRfsv->devinfo(dchar, drv) == rfsv::E_PSI_GEN_NONE)
	dname = QString(drv.getName().c_str());

    statusBar()->changeItem(i18n("Formatting drive %1:").arg(dchar),
				    STID_CONNECTION);
    update();

    emit setProgressText(QString(""));
    emit setProgress(0);
    emit enableProgressText(true);

    Enum<rfsv::errs> res = plpRpcs->formatOpen(dchar, handle, count);
    if (res != rfsv::E_PSI_GEN_NONE) {
	KMessageBox::error(this, i18n(
	    "<QT>Could not format drive %1:<BR/>"
	    "%2</QT>").arg(dchar).arg(KGlobal::locale()->translate(res)));
	emit setProgress(0);
	emit enableProgressText(false);
	statusBar()->changeItem(i18n("Connected to %1").arg(machineName),
				STID_CONNECTION);
	return;
    }
    progressTotal = 0;
    progressLocal = count;
    progressLocalCount = 0;
    progressLocalPercent = -1;
    updateProgress(0);
    for (int i = 0; i < count; i++) {
	res = plpRpcs->formatRead(handle);
	if (res != rfsv::E_PSI_GEN_NONE) {
	    KMessageBox::error(this, i18n(
		"<QT>Error during format of drive %1:<BR/>"
		"%2</QT>").arg(dchar).arg(KGlobal::locale()->translate(res)));
	    emit setProgress(0);
	    emit enableProgressText(false);
	    statusBar()->changeItem(
		i18n("Connected to %1").arg(machineName),
		STID_CONNECTION);
	    count = 0;
	    return;
	}
	updateProgress(1);
    }
    setDriveName(dchar, dname);

    emit setProgress(0);
    emit enableProgressText(false);
    statusBar()->changeItem(i18n("Connected to %1").arg(machineName),
			    STID_CONNECTION);
    statusBar()->message(i18n("Format done"), 2000);
}

void KPsionMainWindow::
slotStartFormat() {
    if (KMessageBox::warningYesNo(this, i18n(
				      "<QT>This erases <B>ALL</B> data "
				      "on the drive(s).<BR/>Do you really "
				      "want to proceed?"
				      )) == KMessageBox::No)
	return;
    formatRunning = true;
    switchActions();
    killSave();

    for (QIconViewItem *i = view->firstItem(); i; i = i->nextItem()) {
	if (i->isSelected() && (i->key() != "Z"))
	    doFormat(i->key());
    }

    runRestore();
    formatRunning = false;
    switchActions();
}

void KPsionMainWindow::
setDriveName(const char dchar, QString dname) {
    KDialogBase dialog(this, "drivenameDialog", true, i18n("Assign drive name"),
		       KDialogBase::Ok | KDialogBase::Cancel, KDialogBase::Ok,
		       true);

    QWidget *w = new QWidget(&dialog);
    QGridLayout *gl = new QGridLayout(w, 1, 1, KDialog::marginHint()*2,
				      KDialog::spacingHint()*2);

    QLabel *l = new QLabel(i18n(
	"<QT>Formatting of drive %1: finished. Please assign a name for "
	"that drive.</QT>").arg(dchar), w);
    gl->addMultiCellWidget(l, 0, 0, 0, 1);

    l = new QLabel(i18n("New name of drive %1:").arg(dchar), w);
    gl->addWidget(l, 1, 0);

    KLineEdit *e = new KLineEdit(dname, w, "nameEntry");
    gl->addWidget(e, 1, 1);

    gl->setColStretch(1, 1);
    dialog.setMainWidget(w);

    int result = dialog.exec();
    QString newname = QString("%1:").arg(dchar);
    QString dstr;
    dstr = dchar;

    switch (result) {
	case QDialog::Accepted:
	    if (!e->text().isEmpty()) {
		Enum<rfsv::errs> res;
		res = plpRfsv->setVolumeName(dchar, e->text());
		if (res == rfsv::E_PSI_GEN_NONE)
		    newname = QString("%1 (%2:)").arg(e->text()).arg(dchar);
	    }
	    drives.replace(dchar, newname);
	    for (QIconViewItem *i = view->firstItem(); i; i = i->nextItem()) {
		if (i->key() == dstr) {
		    i->setText(newname);
		    break;
		}
	    }
	    emit rearrangeIcons(true);
	    break;
	default: // Huh?
	    break;
    }
}

void KPsionMainWindow::
slotToggleToolbar() {
    if (toolBar()->isVisible())
	toolBar()->hide();
    else
	toolBar()->show();
}

void KPsionMainWindow::
slotToggleStatusbar() {
    if (statusBar()->isVisible())
	statusBar()->hide();
    else
	statusBar()->show();
}

void KPsionMainWindow::
slotSaveOptions() {
}

void KPsionMainWindow::
slotPreferences() {
    SetupDialog d(this, plpRfsv, plpRpcs);
    d.exec();
}

void KPsionMainWindow::
updateProgress(unsigned long amount) {
    progressLocalCount += amount;
    int lastPercent = progressLocalPercent;
    if (progressLocal)
	progressLocalPercent = progressLocalCount * 100 / progressLocal;
    else
	progressLocalPercent = 100;
    if (progressLocalPercent != lastPercent)
	emit setProgress(progressLocalPercent);
    if (progressTotal > 0) {
	progressCount += amount;
	lastPercent = progressPercent;
	if (progressTotal)
	    progressPercent = progressCount * 100 / progressTotal;
	else
	    progressPercent = 100;
	if (progressPercent != lastPercent)
	    statusBar()->changeItem(progressTotalText.arg(progressPercent),
		STID_CONNECTION);
    }
    kapp->processEvents();
}

void KPsionMainWindow::
collectFiles(QString dir) {
    Enum<rfsv::errs> res;
    PlpDir files;
    QString tmp = dir;

    kapp->processEvents();
    tmp += "\\";
    if ((res = plpRfsv->dir(tmp.data(), files)) != rfsv::E_PSI_GEN_NONE) {
	// messagebox "Couldn't get directory ...."
    } else
	for (int i = 0; i < files.size(); i++) {
	    PlpDirent e = files[i];


	    long attr = e.getAttr();
	    tmp = dir;
	    tmp += "\\";
	    tmp += e.getName();
	    if (attr & rfsv::PSI_A_DIR) {
		collectFiles(tmp);
	    } else {
		updateProgress(e.getSize());
		if ((attr & rfsv::PSI_A_ARCHIVE) || fullBackup) {
		    backupCount++;
		    backupSize += e.getSize();
		    e.setName(tmp.data());
		    toBackup.push_back(e);
		}
	    }
	}
}

void KPsionMainWindow::
killSave() {
    Enum<rfsv::errs> res;
    processList tmp;

    savedCommands.clear();
    if ((res = plpRpcs->queryPrograms(tmp)) != rfsv::E_PSI_GEN_NONE) {
	cerr << "Could not get process list, Error: " << res << endl;
	return;
    } else {
	for (processList::iterator i = tmp.begin(); i != tmp.end(); i++) {
	    savedCommands += i->getArgs();
	    emit setProgressText(i18n("Stopping %1").arg(i->getName()));
	    kapp->processEvents();
	    plpRpcs->stopProgram(i->getProcId());
	}
    }
    time_t tstart = time(0) + 5;
    while (true) {
	kapp->processEvents();
	usleep(100000);
	kapp->processEvents();
	if ((res = plpRpcs->queryPrograms(tmp)) != rfsv::E_PSI_GEN_NONE) {
	    cerr << "Could not get process list, Error: " << res << endl;
	    return;
	}
	if (tmp.empty())
	    break;
	if (time(0) > tstart) {
	    KMessageBox::error(this, i18n(
		"<QT>Could not stop all processes.<BR/>"
		"Please stop running programs manually on the Psion, "
		"then klick <B>Ok</B>."));
	    tstart = time(0) + 5;
	}
    }
    return;
}

void KPsionMainWindow::
runRestore() {
    Enum<rfsv::errs> res;

    for (QStringList::Iterator it = savedCommands.begin(); it != savedCommands.end(); it++) {
	int firstBlank = (*it).find(' ');
	QString cmd = (*it).left(firstBlank);
	QString arg = (*it).mid(firstBlank + 1);

	if (!cmd.isEmpty()) {
	    // Workaround for broken programs like Backlite.
	    // These do not storethe full program path.
	    // In that case we try running the arg1 which
	    // results in starting the program via recog. facility.
	    emit setProgressText(i18n("Starting %1").arg(cmd));
	    kapp->processEvents();
	    if ((arg.length() > 2) && (arg[1] == ':') && (arg[0] >= 'A') &&
		(arg[0] <= 'Z'))
		res = plpRpcs->execProgram(arg.data(), "");
	    else
		res = plpRpcs->execProgram(cmd.data(), arg.data());
	    if (res != rfsv::E_PSI_GEN_NONE) {
		// If we got an error here, that happened probably because
		// we have no path at all (e.g. Macro5) and the program is
		// not registered in the Psion's path properly. Now try
		// the ususal \System\Apps\<AppName>\<AppName>.app
		// on all drives.
		if (cmd.find('\\') == -1) {
		    driveMap::Iterator it;
		    for (it = drives.begin(); it != drives.end(); it++) {
			QString newcmd = QString::fromLatin1("%1:\\System\\Apps\\%2\\%3").arg(it.key()).arg(cmd).arg(cmd);
			res = plpRpcs->execProgram(newcmd.data(), "");
			if (res == rfsv::E_PSI_GEN_NONE)
			    break;
			newcmd += ".app";
			res = plpRpcs->execProgram(newcmd.data(), "");
			if (res == rfsv::E_PSI_GEN_NONE)
			    break;

		    }
		}
	    }
	}
    }
    return;
}

void KPsionMainWindow::
createIndex() {
    QByteArray ba;
    QTextOStream os(ba);
    os << "#plpbackup index " <<
	(fullBackup ? "F" : "I") << endl;
    for (int i = 0; i < toBackup.size(); i++) {
	PlpDirent e = toBackup[i];
	PsiTime t = e.getPsiTime();
	long attr = e.getAttr() &
	    ~rfsv::PSI_A_ARCHIVE;
	os << hex
	   << setw(8) << setfill('0') <<
	    t.getPsiTimeHi() << " "
	   << setw(8) << setfill('0') <<
	    t.getPsiTimeLo() << " "
	   << setw(8) << setfill('0') <<
	    e.getSize() << " "
	   << setw(8) << setfill('0') <<
	    attr << " "
	   << setw(0) << e.getName() << endl;
	kapp->processEvents();
    }
    QString idxName =
	QString::fromLatin1("KPsion%1Index").arg(fullBackup ?
						 "Full" : "Incremental");
    backupTgz->writeFile(idxName, "root", "root", ba.size(), ba.data());
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
