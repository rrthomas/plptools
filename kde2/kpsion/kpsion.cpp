/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999  Philip Proudman <philip.proudman@btinternet.com>
 *  Copyright (C) 2000, 2001 Fritz Elfert <felfert@to.com>
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
#include "wizards.h"

#include <kapp.h>
#include <klocale.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kconfig.h>
#include <kiconview.h>
#include <kmessagebox.h>
#include <kfileitem.h>

#include <qwhatsthis.h>
#include <qtimer.h>
#include <qlayout.h>
#include <qiodevice.h>
#include <qdir.h>

#include <ppsocket.h>
#include <rfsvfactory.h>
#include <rpcsfactory.h>
#include <bufferarray.h>

#include <iomanip>
#include <strstream>

// internal use for developing offline without
// having a Psion connected.
// !!!!! set to 0 for production code !!!!!
#define OFFLINE 0

#define STID_CONNECTION 1

void KPsionCheckListItem::
init(bool myparent) {
    setSelectable(false);
    dontPropagate = false;
    parentIsKPsionCheckListItem = myparent;
}

void KPsionCheckListItem::
setMetaData(int bType, time_t bWhen) {
	backupType = bType;
	when = bWhen;
}

void KPsionCheckListItem::
stateChange(bool state) {
    QCheckListItem::stateChange(state);

    if (dontPropagate)
	return;
    if (parentIsKPsionCheckListItem)
	((KPsionCheckListItem *)parent())->propagateUp(state);
    propagateDown(state);
}

void KPsionCheckListItem::
propagateDown(bool state) {
    setOn(state);
    KPsionCheckListItem *child = (KPsionCheckListItem *)firstChild();
    while (child) {
	child->propagateDown(state);
	child = (KPsionCheckListItem *)child->nextSibling();
    }
}

void KPsionCheckListItem::
propagateUp(bool state) {
    bool deactivateThis = false;

    KPsionCheckListItem *child = (KPsionCheckListItem *)firstChild();
    while (child) {
	if ((child->isOn() != state) || (!child->isEnabled())) {
	    deactivateThis = true;
	    break;
	}
	child = (KPsionCheckListItem *)child->nextSibling();
    }
    dontPropagate = true;
    if (deactivateThis) {
	setOn(true);
	setEnabled(false);
    } else {
	setEnabled(true);
	setOn(state);
    }
    // Bug in QListView? It doesn't update, when
    // enabled/disabled without activating.
    // -> force it.
    listView()->repaintItem(this);
    dontPropagate = false;
    if (parentIsKPsionCheckListItem)
	((KPsionCheckListItem *)parent())->propagateUp(state);
}

KPsionBackupListView::KPsionBackupListView(QWidget *parent, const char *name)
    : KListView(parent, name) {

    toRestore.clear();
    uid = QString::null;
    KConfig *config = kapp->config();
    config->setGroup("Settings");
    backupDir = config->readEntry("BackupDir");
    addColumn(i18n("Available backups"));
    setRootIsDecorated(true);
}

void KPsionBackupListView::
readBackups(QString uid) {
    QString bdir(backupDir);
    bdir += "/";
    bdir += uid;
    QDir d(bdir);
    const QFileInfoList *fil =
	d.entryInfoList("*.tar.gz", QDir::Files|QDir::Readable, QDir::Name);
    QFileInfoListIterator it(*fil);
    QFileInfo *fi;
    while ((fi = it.current())) {
	bool isValid = false;
	KTarGz tgz(fi->absFilePath());
	const KTarEntry *te;
	QString bTypeName;
	int bType;
	QDateTime date;

	tgz.open(IO_ReadOnly);
	te = tgz.directory()->entry("KPsionFullIndex");
	if (te && (!te->isDirectory())) {
	    date.setTime_t(te->date());
	    bTypeName = i18n("Full");
	    bType = FULL;
	    isValid = true;
	} else {
	    te = tgz.directory()->entry("KPsionIncrementalIndex");
	    if (te && (!te->isDirectory())) {
		date.setTime_t(te->date());
		bTypeName = i18n("Incremental");
		bType = INCREMENTAL;
		isValid = true;
	    }
	}

	if (isValid) {
	    QString n =	i18n("%1 backup, created at %2").arg(bTypeName).arg(date.toString());

	    KPsionCheckListItem *i =
		new KPsionCheckListItem(this, n,
					KPsionCheckListItem::CheckBox);
	    i->setMetaData(bType, te->date());
	    i->setPixmap(0, KGlobal::iconLoader()->loadIcon("mime_empty", KIcon::Small));
	    QStringList files = tgz.directory()->entries();
	    for (QStringList::Iterator f = files.begin();
		 f != files.end(); f++)
		if ((*f != "KPsionFullIndex") &&
		    (*f != "KPsionIncrementalIndex"))
		    listTree(i, tgz.directory()->entry(*f), 0);
	}
	tgz.close();
	++it;
    }
}

void KPsionBackupListView::
listTree(KPsionCheckListItem *cli, const KTarEntry *te, int level) {
    KPsionCheckListItem *i =
	new KPsionCheckListItem(cli, te->name(),
				KPsionCheckListItem::CheckBox);
    if (te->isDirectory()) {
	if (level)
	    i->setPixmap(0, KGlobal::iconLoader()->loadIcon("folder",
							    KIcon::Small));
	else
	    i->setPixmap(0, KGlobal::iconLoader()->loadIcon("hdd_unmount",
							    KIcon::Small));
	KTarDirectory *td = (KTarDirectory *)te;
	QStringList files = td->entries();
	for (QStringList::Iterator f = files.begin(); f != files.end(); f++)
	    listTree(i, td->entry(*f), level + 1);
    } else
	i->setPixmap(0, KGlobal::iconLoader()->loadIcon("mime_empty",
							KIcon::Small));
}

PlpDir &KPsionBackupListView::
getRestoreList() {
    return toRestore;
}

KPsionMainWindow::KPsionMainWindow()
    : KMainWindow() {
    setupActions();

    statusBar()->insertItem(i18n("Idle"), STID_CONNECTION, 1);
    statusBar()->setItemAlignment(STID_CONNECTION,
				  QLabel::AlignLeft|QLabel::AlignVCenter);

    backupRunning = false;
    restoreRunning = false;
    formatRunning = false;

    view = new KIconView(this, "iconview");
    view->setSelectionMode(KIconView::Multi);
    view->setResizeMode(KIconView::Adjust);
    view->setItemsMovable(false);
    connect(view, SIGNAL(clicked(QIconViewItem *)),
	    SLOT(iconClicked(QIconViewItem *)));
    connect(view, SIGNAL(onItem(QIconViewItem *)),
	    SLOT(iconOver(QIconViewItem *)));
    KConfig *config = kapp->config();
    config->setGroup("Psion");
    QStringList uids = config->readListEntry("MachineUIDs");
    for (QStringList::Iterator it = uids.begin(); it != uids.end(); it++) {
	QString tmp = QString::fromLatin1("Name_%1").arg(*it);
	machines.insert(*it, config->readEntry(tmp));
    }
    config->setGroup("Settings");
    backupDir = config->readEntry("BackupDir");
    config->setGroup("Connection");
    reconnectTime = config->readNumEntry("Retry");
    ncpdDevice = config->readEntry("Device", i18n("off"));
    ncpdSpeed = config->readNumEntry("Speed", 115200);

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

void KPsionMainWindow::
setupActions() {

    KStdAction::quit(this, SLOT(close()), actionCollection());
    KStdAction::showToolbar(this, SLOT(slotToggleToolbar()),
			    actionCollection());
    KStdAction::showStatusbar(this, SLOT(slotToggleStatusbar()),
			      actionCollection());
    KStdAction::saveOptions(this, SLOT(slotSaveOptions()),
			    actionCollection());
    KStdAction::preferences(this, SLOT(slotPreferences()),
			    actionCollection());
    new KAction(i18n("Start &Format"), 0L, 0, this,
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
#if OFFLINE
    actionCollection()->action("restore")->setEnabled(true);
#else
    actionCollection()->action("restore")->setEnabled(false);
#endif
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
#if OFFLINE
    actionCollection()->action("restore")->setEnabled(true);
#else
    actionCollection()->action("restore")->setEnabled(rwSelected);
#endif
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
			  KFileItem(KURL(), "inode/x-psion-drive", 0).pixmap(0));
    tmp = QString::fromLatin1("%1").arg(letter);
    it->setKey(tmp);
    it->setDropEnabled(false);
    it->setDragEnabled(false);
    it->setRenameEnabled(false);
}

void KPsionMainWindow::
queryPsion() {
    u_int32_t devbits;
    Enum <rfsv::errs> res;

    statusBar()->changeItem(i18n("Retrieving machine info ..."),
			    STID_CONNECTION);
#if OFFLINE
    machineUID = 0x1000118a0c428fa3ULL;
    S5mx = true;
    insertDrive('C', "Intern");
    insertDrive('D', "Flash");
    insertDrive('Z', "RomDrive");
#else
    rpcs::machineInfo mi;
    if ((res = plpRpcs->getMachineInfo(mi)) != rfsv::E_PSI_GEN_NONE) {
	QString msg = i18n("Could not get Psion machine info");
	statusBar()->changeItem(msg, STID_CONNECTION);
	KMessageBox::error(this, msg);
	return;
    }
    machineUID = mi.machineUID;
    S5mx = (strcmp(mi.machineName, "SERIES5mx") == 0);
#endif

    QString uid = getMachineUID();
    bool machineFound = false;
    KConfig *config = kapp->config();
    config->setGroup("Psion");
    machineName = i18n("an unknown machine");
    psionMap::Iterator it;
    for (it = machines.begin(); it != machines.end(); it++) {
	if (uid == it.key()) {
	    machineName = it.data();
	    QString tmp =
		QString::fromLatin1("BackupDrives_%1").arg(it.key());
	    backupDrives = config->readListEntry(tmp);
	    machineFound = true;
	}
    }
#if (!(OFFLINE))
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
	    if (plpRfsv->devinfo(i, drive) == rfsv::E_PSI_GEN_NONE)
		insertDrive('A' + i, drive.getName().c_str());
	}
	devbits >>= 1;
    }
#endif
    if (!machineFound) {
	NewPsionWizard *wiz = new NewPsionWizard(this, "newpsionwiz");
	wiz->exec();
    }
    statusBar()->changeItem(i18n("Connected to %1").arg(machineName),
			    STID_CONNECTION);
}

QString KPsionMainWindow::
getMachineUID() {
    // ??! None of QString's formatting methods knows about long long.
    ostrstream s;
    s << hex << setw(16) << machineUID;
    QString ret = s.str();
    ret = ret.left(16);
    return ret;
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
tryConnect() {
#if (!(OFFLINE))
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

    rfsvSocket = new ppsocket();
    statusBar()->changeItem(i18n("Connecting ..."), STID_CONNECTION);
    if (!rfsvSocket->connect(NULL, 7501)) {
	statusMsg = i18n("RFSV could not connect to ncpd at %1:%2. ").arg("localhost").arg(7501);
	if (reconnectTime) {
	    nextTry = reconnectTime;
	    statusMsg += i18n(" (Retry in %1 seconds.)");
	    QTimer::singleShot(1000, this, SLOT(slotUpdateTimer()));
	}
	statusBar()->changeItem(statusMsg.arg(reconnectTime),
				STID_CONNECTION);
	if (showMB)
	    KMessageBox::error(this, statusMsg.arg(reconnectTime));
	return;
    }
    rfsvfactory factory(rfsvSocket);
    plpRfsv = factory.create(false);
    if (plpRfsv == 0L) {
	statusMsg = i18n("RFSV could not establish link: %1.").arg(factory.getError());
	delete rfsvSocket;
	rfsvSocket = 0L;
	if (reconnectTime) {
	    nextTry = reconnectTime;
	    statusMsg += i18n(" (Retry in %1 seconds.)");
	    QTimer::singleShot(1000, this, SLOT(slotUpdateTimer()));
	}
	statusBar()->changeItem(statusMsg.arg(reconnectTime),
				STID_CONNECTION);
	if (showMB)
	    KMessageBox::error(this, statusMsg.arg(reconnectTime));
	return;
    }

    rpcsSocket = new ppsocket();
    if (!rpcsSocket->connect(NULL, 7501)) {
	statusMsg = i18n("RPCS could not connect to ncpd at %1:%2.").arg("localhost").arg(7501);
	delete plpRfsv;
	plpRfsv = 0L;
	delete rfsvSocket;
	rfsvSocket = 0L;
	if (reconnectTime) {
	    nextTry = reconnectTime;
	    statusMsg += i18n(" (Retry in %1 seconds.)");
	    QTimer::singleShot(1000, this, SLOT(slotUpdateTimer()));
	}
	statusBar()->changeItem(statusMsg.arg(reconnectTime),
				STID_CONNECTION);
	if (showMB)
	    KMessageBox::error(this, statusMsg.arg(reconnectTime));
	return;
    }
    rpcsfactory factory2(rpcsSocket);
    plpRpcs = factory2.create(false);
    if (plpRpcs == 0L) {
	statusMsg = i18n("RPCS could not establish link: %1.").arg(factory.getError());
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
	}
	statusBar()->changeItem(statusMsg.arg(reconnectTime),
				STID_CONNECTION);
	if (showMB)
	    KMessageBox::error(this, statusMsg.arg(reconnectTime));
	return;
    }
#endif
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
slotStartFullBackup() {
    fullBackup = true;
    doBackup();
}

void KPsionMainWindow::
slotStartIncBackup() {
    fullBackup = false;
    doBackup();
}

void KPsionMainWindow::
doBackup() {
    backupRunning = true;
    switchActions();
    toBackup.clear();
    KDialog *d = new KDialog(this, "backupDialog", false);
    d->setCaption(i18n("Backup"));
    QGridLayout *gl = new QGridLayout(d);
    progressLabel = new KSqueezedTextLabel(d);
    gl->addWidget(progressLabel, 1, 1);
    progress = new KProgress(0, 100, 0, KProgress::Horizontal, d,
			     "backupProgress");

    gl->addWidget(progress, 2, 1);
    gl->addRowSpacing(0, KDialog::marginHint());
    gl->addRowSpacing(3, KDialog::marginHint());
    gl->addColSpacing(0, KDialog::marginHint());
    gl->addColSpacing(2, KDialog::marginHint());
    gl->setColStretch(1, 1);
    gl->setRowStretch(1, 1);
    d->setMinimumSize(250, 80);
    d->show();

    // Collect list of files to backup
    backupSize = 0;
    backupCount = 0;
    progressTotal = 0;
    for (QIconViewItem *i = view->firstItem(); i; i = i->nextItem()) {
	if (i->isSelected()) {
	    QString drv = i->key();
	    drv += ":";
	    int drvNum = *(drv.data()) - 'A';
	    PlpDrive drive;
	    if (plpRfsv->devinfo(drvNum, drive) != rfsv::E_PSI_GEN_NONE) {
		KMessageBox::error(this, i18n("Could not retrieve drive details for drive %1").arg(drv));
		d->hide();
		delete d;
		backupRunning = false;
		switchActions();
		return;
	    }
	    progressLabel->setText(i18n("Scanning drive %1").arg(drv));

	    progressLocal = drive.getSize() - drive.getSpace();
	    progressLocalCount = 0;
	    progressLocalPercent = -1;
	    progress->setValue(0);
	    collectFiles(drv);
	}
    }
    progressLabel->setText(i18n("%1 files need backup").arg(backupSize));
    if (backupCount == 0) {
	KMessageBox::information(this, i18n("No files need backup"));
	d->hide();
	delete d;
	backupRunning = false;
	switchActions();
	return;
    }

    statusBar()->message(i18n("Backup"));
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
	    KMessageBox::error(this, i18n("Could not create backup folder %1").arg(archiveName));
	    d->hide();
	    delete d;
	    statusBar()->clear();
	    backupRunning = false;
	    switchActions();
	    return;
	}

    archiveName += (fullBackup) ? "/F-" : "/I-";
    time_t now = time(0);
    char tstr[30];
    strftime(tstr, sizeof(tstr), "%Y-%m-%d-%H-%M-%S.tar.gz",
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
    for (int i = 0; i < toBackup.size(); i++) {
	PlpDirent e = toBackup[i];
	const char *fn = e.getName();
	const char *p;
	char *q;
	char unixname[1024];

	for (p = fn, q = unixname; *p; p++, q++)
	    switch (*p) {
		case '%':
		    *q++ = '%';
		    *q++ = '2';
		    *q = '5';
		    break;
		case '/':
		    *q++ = '%';
		    *q++ = '2';
		    *q= 'f';
		    break;
		case '\\':
		    *q = '/';
		    break;
		default:
		    *q = *p;
	    }
	*q = '\0';

	ostrstream os;

	progressLabel->setText(i18n("Backing up %1").arg(fn));
	progressLocal = e.getSize();
	progressLocalCount = 0;
	progressLocalPercent = -1;
	progress->setValue(0);

	u_int32_t handle;

	kapp->processEvents();
	res = plpRfsv->fopen(plpRfsv->opMode(rfsv::PSI_O_RDONLY), fn,
			     handle);
	if (res != rfsv::E_PSI_GEN_NONE) {
	    if (KMessageBox::warningYesNo(this, i18n("<QT>Could not open<BR/><B>%1</B></QT>").arg(fn)) == KMessageBox::No) {
		badBackup = true;
		break;
	    } else {
		e.setName("!");
		continue;
	    }
	}
	unsigned char *buff = new unsigned char[RFSV_SENDLEN];
	u_int32_t len;
	do {
	    if ((res = plpRfsv->fread(handle, buff, RFSV_SENDLEN, len)) ==
		rfsv::E_PSI_GEN_NONE) {
		os.write(buff, len);
		updateProgress(len);
	    }
	    kapp->processEvents();
	} while ((len > 0) && (res == rfsv::E_PSI_GEN_NONE));
	delete[]buff;
	plpRfsv->fclose(handle);
	if (res != rfsv::E_PSI_GEN_NONE) {
	    if (KMessageBox::warningYesNo(this, i18n("<QT>Could not read<BR/><B>%1</B></QT>").arg(fn)) == KMessageBox::No) {
		badBackup = true;
		break;
	    } else {
		e.setName("!");
		continue;
	    }
	}
	backupTgz->writeFile(unixname, "root", "root", os.pcount(),
			     os.str());
    }

    if (!badBackup) {
	// Reset archive attributes of all backuped files.
	progressLabel->setText(i18n("Resetting archive attributes ..."));
	progressLocal = backupSize;
	progressLocalCount = 0;
	progressLocalPercent = -1;
	progress->setValue(0);
	for (int i = 0; i < toBackup.size(); i++) {
	    PlpDirent e = toBackup[i];
	    const char *fn = e.getName();
	    if ((e.getAttr() & rfsv::PSI_A_ARCHIVE) &&
		(strcmp(fn, "!") != 0)) {
		kapp->processEvents();
		res = plpRfsv->fsetattr(fn, 0, rfsv::PSI_A_ARCHIVE);
		if (res != rfsv::E_PSI_GEN_NONE) {
		    if (KMessageBox::warningYesNo(this, i18n("<QT>Could not set attributes of<BR/><B>%1</B></QT>").arg(fn)) == KMessageBox::No) {
			break;
		    }
		}
	    }
	    updateProgress(e.getSize());
	}
    }
    // Restart previously running applications on the Psion
    // from saved state info.
    runRestore();

    backupTgz->close();
    delete backupTgz;
    if (badBackup)
	unlink(archiveName.data());
    d->hide();
    delete d;
    backupRunning = false;
    switchActions();
    statusBar()->message(i18n("Backup done"), 2000);
}

void KPsionMainWindow::
slotStartRestore() {
    restoreRunning = true;
    switchActions();

    KDialog *d = new KDialog(this, "restoreDialog", true);
    d->setCaption(i18n("Restore"));
    QGridLayout *gl = new QGridLayout(d);
    //progressLabel = new KSqueezedTextLabel(d);
    KPsionBackupListView *v = new KPsionBackupListView(d, "restoreSelector");
    gl->addWidget(v, 1, 1);
    //progress = new KProgress(0, 100, 0, KProgress::Horizontal, d, "restoreProgress");

    //gl->addWidget(progress, 2, 1);
    gl->addRowSpacing(0, KDialog::marginHint());
    gl->addRowSpacing(3, KDialog::marginHint());
    gl->addColSpacing(0, KDialog::marginHint());
    gl->addColSpacing(2, KDialog::marginHint());
    gl->setColStretch(1, 1);
    gl->setRowStretch(1, 1);
    d->setMinimumSize(250, 80);
    v->readBackups(getMachineUID());
    d->exec();

    d->hide();
    delete d;
    restoreRunning = false;
    switchActions();
    statusBar()->message(i18n("Restore done"), 2000);
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
	progress->setValue(progressLocalPercent);
    if (progressTotal > 0) {
	progressCount += amount;
	lastPercent = progressPercent;
	if (progressTotal)
	    progressPercent = progressCount * 100 / progressTotal;
	else
	    progressPercent = 100;
	if (progressPercent != lastPercent)
	    statusBar()->message(i18n("Backup %1% complete").arg(progressPercent));
    }
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
		kapp->processEvents();
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
    bufferArray tmp;

    savedCommands.clear();
    if ((res = plpRpcs->queryDrive('C', tmp)) != rfsv::E_PSI_GEN_NONE) {
	cerr << "Could not get process list, Error: " << res << endl;
	return;
    } else {
	while (!tmp.empty()) {
	    QString pbuf;
	    bufferStore cmdargs;
	    bufferStore bs = tmp.pop();
	    int pid = bs.getWord(0);
	    const char *proc = bs.getString(2);
	    if (S5mx)
		pbuf.sprintf("%s.$%02d", proc, pid);
	    else
		pbuf.sprintf("%s.$%d", proc, pid);
	    bs = tmp.pop();
	    if (plpRpcs->getCmdLine(pbuf.data(), cmdargs) == 0) {
		QString cmdline(cmdargs.getString(0));
		cmdline += " ";
		cmdline += bs.getString(0);
		savedCommands += cmdline;
	    }
	    progressLabel->setText(i18n("Stopping %1").arg(cmdargs.getString(0)));
	    kapp->processEvents();
	    plpRpcs->stopProgram(pbuf);
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
	    progressLabel->setText(i18n("Starting %1").arg(cmd));
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
    ostrstream os;
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
    backupTgz->writeFile("Index", "root", "root", os.pcount(), os.str());
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
