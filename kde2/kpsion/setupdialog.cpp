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
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "kpsionconfig.h"
#include "setupdialog.h"

#include <kapplication.h>
#include <kconfig.h>
#include <kdesktopfile.h>
#include <kglobalsettings.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kmessagebox.h>
#include <kstandarddirs.h>

#include <qlayout.h>
#include <qgroupbox.h>
#include <qwhatsthis.h>
#include <qheader.h>

#include <errno.h>

SetupDialog::SetupDialog(QWidget *parent, rfsv *plpRfsv, rpcs *plpRpcs)
    : KDialogBase(Tabbed, i18n("Settings"), Ok|Apply|Default|Cancel, Ok, parent,
		  "settingsDialog", true, true)
{
    int i;
    QString tmp;
    QStringList sl;
    QStringList::Iterator sli;
    QLabel *l;
    KPsionConfig pcfg;

    KConfig *config = kapp->config();

    // Page 1
    page1 = addPage(i18n("&Backup"));
    QBoxLayout *box = new QVBoxLayout(page1, KDialog::spacingHint());

    QGroupBox *gb = new QGroupBox(i18n("Backup folder"), page1, "bdirBox");
    box->addWidget(gb);

    QGridLayout *grid = new QGridLayout(gb, 1, 1, marginHint() * 2,
					spacingHint() * 2);

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPDIR));
    tmp = config->readEntry(pcfg.getOptionName(KPsionConfig::OPT_BACKUPDIR));
    oldBDir = tmp;
    bdirLabel = new QLabel(gb, "bdirLabel");
    bdirLabel->setText(tmp);
    bdirButton = new QPushButton(i18n("Browse"), gb);

    QWhatsThis::add(bdirLabel, i18n(
	"<QT>This is the name of the backup folder. "
	"Click on <B>Browse</B>, for opening a dialog which lets you easily "
	"select the backup folder. If the backup folder is changed and "
	"it already contains backups, these are moved to the new "
	"location.</QT>"));
    QWhatsThis::add(bdirButton, i18n(
	"<QT>Click here, for opening a dialog which lets you easily "
	"select the backup folder. If the backup folder is changed and "
	"it already contains backups, these are moved to the new "
	"location.</QT>"));
    grid->addWidget(bdirLabel, 0, 0);
    grid->addWidget(bdirButton, 0, 1);
    connect(bdirButton, SIGNAL(clicked()), SLOT(slotBdirBrowse()));

    grid->addRowSpacing(0, marginHint());
    grid->setColStretch(0, 1);

    gb = new QGroupBox(i18n("Backup strategy"), page1, "stratBox");
    box->addWidget(gb);

    grid = new QGridLayout(gb, 1, 1, marginHint() * 2, spacingHint() * 2);
    l = new QLabel(i18n("&Incremental backup interval"), gb,
		   "iBackupIntLabel");
    grid->addWidget(l, 0, 0);
    QWhatsThis::add(l, i18n(
	"<QT>If you select an interval here, <B>KPsion</B> creates an entry "
	"in your Autostart directory which performs an incremental backup of "
	"selected drives. If your Psion is not connected at that time, "
	"nothing will happen.</QT>"));

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_INCINTERVAL));
    i = config->readNumEntry(pcfg.getOptionName(KPsionConfig::OPT_INCINTERVAL));
    iIntCombo = new KComboBox(false, gb, "iIntCombo");
    iIntCombo->insertStringList(pcfg.getConfigBackupInterval());
    iIntCombo->setCurrentItem(i);
    grid->addWidget(iIntCombo, 0, 1);
    l->setBuddy(iIntCombo);
    QWhatsThis::add(iIntCombo, i18n(
	"<QT>If you select an interval here, <B>KPsion</B> creates an entry "
	"in your Autostart directory which performs an incremental backup of "
	"selected drives. If your Psion is not connected at that time, "
	"nothing will happen.</QT>"));

    l = new QLabel(i18n("&Full backup interval"), gb, "fBackupIntLabel");
    grid->addWidget(l, 1, 0);
    QWhatsThis::add(l, i18n(
	"<QT>If you select an interval here, <B>KPsion</B> creates an entry "
	"in your Autostart directory which performs a full backup of "
	"selected drives. If your Psion is not connected at that time, "
	"nothing will happen.</QT>"));

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_FULLINTERVAL));
    i = config->readNumEntry(pcfg.getOptionName(KPsionConfig::OPT_FULLINTERVAL));
    fIntCombo = new KComboBox(false, gb, "fIntCombo");
    fIntCombo->insertStringList(pcfg.getConfigBackupInterval());
    fIntCombo->setCurrentItem(i);
    grid->addWidget(fIntCombo, 1, 1);
    l->setBuddy(fIntCombo);
    QWhatsThis::add(fIntCombo, i18n(
	"<QT>If you select an interval here, <B>KPsion</B> creates an entry "
	"in your Autostart directory which performs a full backup of "
	"selected drives. If your Psion is not connected at that time, "
	"nothing will happen.</QT>"));

    l = new QLabel(i18n("Backup &generations"), gb, "backupGenLabel");
    grid->addWidget(l, 2, 0);
    QWhatsThis::add(l, i18n(
	"<QT>Specify the number of backup generations you want to keep "
	"on your machine. One backup generation means a full backup of "
	"a drive plus eventually made incrmental backups of that drive. "
	"This is checked every time, a full backup is done and if the "
	"number of existing backups is exceeded, the oldest backup and "
	"its corresponding incremental backups are deleted.</QT>"));

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPGEN));
    i = config->readNumEntry(pcfg.getOptionName(KPsionConfig::OPT_BACKUPGEN));
    genSpin = new KIntSpinBox(0, 10, 1, i, 10, gb, "backupGenSpin");
    grid->addWidget(genSpin, 2, 1);
    l->setBuddy(genSpin);
    QWhatsThis::add(genSpin, i18n(
	"<QT>Specify the number of backup generations you want to keep "
	"on your machine. One backup generation means a full backup of "
	"a drive plus eventually made incrmental backups of that drive. "
	"This is checked every time, a full backup is done and if the "
	"number of existing backups is exceeded, the oldest backup and "
	"its corresponding incremental backups are deleted.</QT>"));

    grid->addRowSpacing(0, marginHint());
    grid->setColStretch(0, 1);

    // Page 3
    page2 = addPage(i18n("&Connection"));
    grid = new QGridLayout(page2, 1, 1, marginHint() * 2, spacingHint() * 2);

    l = new QLabel(i18n("&Connection retry interval (sec.)"), page2,
		   "rconLabel");
    grid->addWidget(l, 0, 0);
    QWhatsThis::add(l, i18n(
	"<QT>If this is not 0, <B>KPsion</B> attempts to retry connection "
	"setup.</QT>"));

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_CONNRETRY));
    i = config->readNumEntry(pcfg.getOptionName(KPsionConfig::OPT_CONNRETRY));
    rconSpin = new KIntSpinBox(0, 600, 1, i, 10, page2, "rconSpin");
    grid->addWidget(rconSpin, 0, 1);
    l->setBuddy(rconSpin);
    QWhatsThis::add(rconSpin, i18n(
	"<QT>If this is not 0, <B>KPsion</B> attempts to retry connection "
	"setup.</QT>"));

    l = new QLabel(i18n("Serial &device"), page2, "devLabel");
    grid->addWidget(l, 1, 0);
    QWhatsThis::add(l, i18n(
	"<QT>If a device is selected here and the connection can not "
	"established at startup, <B>KPsion</B> will attempt to start "
	"the ncpd daemon with appropriate parameters.</QT>"));

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_SERIALDEV));
    tmp = config->readEntry(pcfg.getOptionName(KPsionConfig::OPT_SERIALDEV));

    devCombo = new KComboBox(false, page2, "devCombo");
    sl = pcfg.getConfigDevices();
    devCombo->insertStringList(sl);
    if (tmp == "off")
	devCombo->setCurrentItem(0);
    else
	for (i = 0, sli = sl.begin(); sli != sl.end(); ++i, ++sli) {
	    if (*sli == tmp)
		devCombo->setCurrentItem(i);
	}
    grid->addWidget(devCombo, 1, 1);
    l->setBuddy(devCombo);
    QWhatsThis::add(devCombo, i18n(
	"<QT>If a device is selected here and the connection can not "
	"established at startup, <B>KPsion</B> will attempt to start "
	"the ncpd daemon with appropriate parameters.</QT>"));

    l = new QLabel(i18n("Serial &speed"), page2, "speedLabel");
    grid->addWidget(l, 2, 0);
    QWhatsThis::add(l, i18n(
	"<QT>If a device is selected at <B>Serial device</B> and the "
	"connection can not established at startup, <B>KPsion</B> will "
	"attempt to start the ncpd daemon with appropriate parameters.</QT>"));

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_SERIALSPEED));
    tmp = config->readEntry(pcfg.getOptionName(KPsionConfig::OPT_SERIALSPEED));

    speedCombo = new KComboBox(false, page2, "speedCombo");
    sl = pcfg.getConfigSpeeds();
    speedCombo->insertStringList(sl);
    for (i = 0, sli = sl.begin(); sli != sl.end(); ++i, ++sli) {
	if (*sli == tmp)
	    speedCombo->setCurrentItem(i);
    }
    grid->addWidget(speedCombo, 2, 1);
    l->setBuddy(speedCombo);
    QWhatsThis::add(speedCombo, i18n(
	"<QT>If a device is selected at <B>Serial device</B> and the "
	"connection can not established at startup, <B>KPsion</B> will "
	"attempt to start the ncpd daemon with appropriate parameters.</QT>"));


    grid->setColStretch(0, 1);

    // Page 4
    page3 = addPage(i18n("&Machines"));
    grid = new QGridLayout(page3, 1, 1, marginHint() * 2, spacingHint() * 2);

    l = new QLabel(page3, "nameLabel");
    l->setText(i18n("Machine &UID"));
    grid->addWidget(l, 0, 0);
    QWhatsThis::add(l, i18n(
	"<QT>This shows the known machines. Select an entry here and you "
	"will be able to change its name and specify the drives that should "
	"be selected for automatic backups. You also can delete a machine "
	"which includes deleting all backups for it.</QT>"));

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_UIDS));
    sl = config->readListEntry(pcfg.getOptionName(KPsionConfig::OPT_UIDS));
    machCombo = new KComboBox(false, page3, "machCombo");
    machCombo->insertStringList(sl);
    grid->addWidget(machCombo, 0, 1);
    l->setBuddy(machCombo);
    QWhatsThis::add(machCombo, i18n(
	"<QT>This shows the known machines. Select an entry here and you "
	"will be able to change its name and specify the drives that should "
	"be selected for automatic backups. You also can delete a machine "
	"which includes deleting all backups for it.</QT>"));

    l = new QLabel(page3, "nameLabel");
    l->setText(i18n("Machine &Name"));
    grid->addWidget(l, 1, 0);
    QWhatsThis::add(l, i18n(
	"<QT>You can change the name of the machine here. The name "
	"is not used internally but only provided for display purposes.</QT>"));

    nameEdit = new KLineEdit(page3, "nameEdit");
    grid->addWidget(nameEdit, 1, 1);
    l->setBuddy(nameEdit);
    QWhatsThis::add(nameEdit, i18n(
	"<QT>You can change the name of the machine here. The name "
	"is not used internally but only provided for display purposes.</QT>"));

    mdelButton = new QPushButton(i18n("Delete"), page3);
    grid->addMultiCellWidget(mdelButton, 0, 1, 2, 2);
    connect(mdelButton, SIGNAL(clicked()), SLOT(slotDeleteMachine()));
    QWhatsThis::add(mdelButton, i18n(
	"<QT>Click here to delete the selected machine from the list of "
	"known machines. This includes deleting all backups of that "
	"machine.</QT>"));

    backupListView = new KListView(page3, "bdriveListView");
    backupListView->addColumn(i18n("Automatic backup drives"));

    backupListView->header()->setClickEnabled(false);
    backupListView->header()->setResizeEnabled(false);
    backupListView->header()->setMovingEnabled(false);
    int height = backupListView->header()->height();
    backupListView->setMinimumWidth(backupListView->columnWidth(0) + 4);
    backupListView->setMinimumHeight(height + 10);
    backupListView->setMaximumHeight(height + 10);
    QWhatsThis::add(backupListView, i18n(
	"<QT>Here, you can select the drives which shall be included in "
	"atomatic backups.</QT>"));

    grid->addMultiCellWidget(backupListView, 2, 2, 0, 2);
    connect(machCombo, SIGNAL(activated(int)), SLOT(slotMachineChanged(int)));
    slotMachineChanged(0);
    grid->setColStretch(0, 1);
    grid->setRowStretch(2, 1);

    connect(this, SIGNAL(defaultClicked()), SLOT(slotDefaultClicked()));
    connect(this, SIGNAL(okClicked()), SLOT(slotSaveSettings()));
    connect(this, SIGNAL(applyClicked()), SLOT(slotSaveSettings()));
}

void SetupDialog::
slotMachineChanged(int idx) {
    KPsionConfig pcfg;
    KConfig *config = kapp->config();
    int height = 0;
    QString mach = machCombo->currentText();

    backupListView->clear();
    nameEdit->clear();
    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_MACHNAME));
    nameEdit->setText(config->readEntry(
	pcfg.getOptionName(KPsionConfig::OPT_MACHNAME).arg(mach)));

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_DRIVES));
    QStringList drives = config->readListEntry(
	pcfg.getOptionName(KPsionConfig::OPT_DRIVES).arg(mach));
    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPDRIVES));
    QStringList bDrives = config->readListEntry(
	pcfg.getOptionName(KPsionConfig::OPT_BACKUPDRIVES).arg(mach));

    QStringList::Iterator it;
    for (it = drives.begin(); it != drives.end(); it++) {
	QCheckListItem *i = new QCheckListItem(backupListView, *it,
					       QCheckListItem::CheckBox);
	height += i->height();
	if (bDrives.find(*it) != bDrives.end())
	    i->setOn(true);
	i->setSelectable(false);
    }
    if (idx == -1)
	return;
    height += backupListView->header()->height();
    backupListView->setMinimumWidth(backupListView->columnWidth(0) + 4);
    backupListView->setMinimumHeight(height + 10);
    backupListView->setMaximumHeight(height + 10);
}

void SetupDialog::
slotDeleteMachine() {
    KPsionConfig pcfg;
    KConfig *config = kapp->config();

    QString mach = machCombo->currentText();

    int res = KMessageBox::questionYesNo(this, i18n(
	"<QT>Removing a machine also removes all backups of this machine.<BR/>"
	"<B>This can not be reverted!</B><BR/>"
	"Do you really want to remove the machine %1 (%2)?</QT>"
	).arg(mach).arg(nameEdit->text()));
    if (res != KMessageBox::Yes)
	return;

    QString bdir = bdirLabel->text() + "/" + mach;
    QDir d(bdir);
    if (d.exists()) {
	d.setFilter(QDir::Files);
	QStringList entries = d.entryList();
	QStringList::Iterator ei;
	for (ei = entries.begin(); ei != entries.end(); ++ei) {
	    if (!d.remove(*ei)) {
		KMessageBox::error(this,
				   i18n("Could not remove backup file %1.").arg(*ei));
		return;
	    }
	}
	d.rmdir(bdir);
    }

    machCombo->removeItem(machCombo->currentItem());

    QStringList sl = config->readListEntry(
	pcfg.getOptionName(KPsionConfig::OPT_DRIVES).arg(mach));
    config->sync();

    QString dcfgName =
	KGlobal::dirs()->saveLocation("config", QString::null, false);
    dcfgName += "kpsionrc";
    KSimpleConfig dcfg(dcfgName);
    dcfg.setGroup(pcfg.getSectionName(KPsionConfig::OPT_MACHNAME));
    dcfg.deleteEntry(pcfg.getOptionName(
	KPsionConfig::OPT_MACHNAME).arg(mach), false);
    dcfg.setGroup(pcfg.getSectionName(KPsionConfig::OPT_DRIVES));
    dcfg.deleteEntry(pcfg.getOptionName(
	KPsionConfig::OPT_DRIVES).arg(mach), false);
    dcfg.setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPDRIVES));
    dcfg.deleteEntry(pcfg.getOptionName(
	KPsionConfig::OPT_BACKUPDRIVES).arg(mach), false);
    QStringList::Iterator it;
    for (it = sl.begin(); it != sl.end(); it++) {
	dcfg.setGroup(pcfg.getSectionName(KPsionConfig::OPT_LASTFULL));
	dcfg.deleteEntry(pcfg.getOptionName(
	    KPsionConfig::OPT_LASTFULL).arg(mach).arg(*it), false);
	dcfg.setGroup(pcfg.getSectionName(KPsionConfig::OPT_LASTINC));
	dcfg.deleteEntry(pcfg.getOptionName(
	    KPsionConfig::OPT_LASTINC).arg(mach).arg(*it), false);
    }
    dcfg.sync();
    config->reparseConfiguration();
    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_UIDS));
    sl = config->readListEntry(pcfg.getOptionName(KPsionConfig::OPT_UIDS));
    sl.remove(mach);
    config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_UIDS), sl);

    slotMachineChanged(-1);
}

void SetupDialog::
slotDefaultClicked() {
    KPsionConfig pcfg;

    bdirLabel->setText(pcfg.getStrDefault(KPsionConfig::DEF_BACKUPDIR));
    iIntCombo->setCurrentItem(pcfg.getIntDefault(KPsionConfig::DEF_INCINTERVAL));
    fIntCombo->setCurrentItem(pcfg.getIntDefault(KPsionConfig::DEF_FULLINTERVAL));
    genSpin->setValue(pcfg.getIntDefault(KPsionConfig::DEF_BACKUPGEN));
    rconSpin->setValue(pcfg.getIntDefault(KPsionConfig::DEF_CONNRETRY));
    devCombo->setCurrentItem(pcfg.getIntDefault(KPsionConfig::DEF_SERIALDEV));
    speedCombo->setCurrentItem(pcfg.getIntDefault(KPsionConfig::DEF_SERIALSPEED));
}

void SetupDialog::
slotBdirBrowse() {
    QString dir = KFileDialog::getExistingDirectory(bdirLabel->text(), this,
						    i18n("Backup folder"));
    checkBackupDir(dir);
}

void SetupDialog::
slotSaveSettings() {
    KConfig *config = kapp->config();
    KPsionConfig pcfg;

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPDIR));
    config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_BACKUPDIR),
		       bdirLabel->text());

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPGEN));
    config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_BACKUPGEN),
		       genSpin->value());

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_INCINTERVAL));
    config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_INCINTERVAL),
		       iIntCombo->currentItem());

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_FULLINTERVAL));
    config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_FULLINTERVAL),
		       fIntCombo->currentItem());

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_CONNRETRY));
    config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_CONNRETRY),
		       rconSpin->value());

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_SERIALDEV));
    if (devCombo->currentItem() == 0)
	config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_SERIALDEV),
			   "off");
    else
	config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_SERIALDEV),
			   devCombo->currentText());

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_SERIALSPEED));
    config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_SERIALSPEED),
		       speedCombo->currentText());

    QString asFile = KGlobalSettings::autostartPath() + "/PsionBackup.desktop";
    // Create or remove autostart entry
    if (iIntCombo->currentItem() || fIntCombo->currentItem()) {
	KDesktopFile f(asFile);
	f.setGroup("Desktop Entry");
	f.writeEntry("Type", "Application");
	f.writeEntry("Exec", "kpsion --autobackup");
	f.writeEntry("Icon", "kpsion");
	f.writeEntry("Terminal", false);
	f.writeEntry("Comment", "Scheduled backup of your Psion");
    } else
	unlink(asFile.latin1());
}

bool SetupDialog::
showPage(int index) {
    switch (activePageIndex()) {
	case 1:
	    QString dir(bdirLabel->text());
	    if (!checkBackupDir(dir))
		return false;
    }
    return KDialogBase::showPage(index);
}

void SetupDialog::
closeEvent(QCloseEvent *e) {
    reject();
}

bool SetupDialog::
checkBackupDir(QString &dir) {
    KConfig *config = kapp->config();
    KPsionConfig pcfg;
    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPDIR));
    QString tmp =
	config->readEntry(pcfg.getOptionName(KPsionConfig::OPT_BACKUPDIR));

    bool rmFlag = false;

    if (!bdirCreated.isEmpty()) {
	if (bdirCreated != dir) {
	    rmFlag = true;
	    bdirCreated = "";
	}
    }
    if (!dir.isEmpty()) {
	QDir d(dir);
	if (!d.exists()) {
	    if (KMessageBox::questionYesNo(this,
					   i18n("<QT>The folder <B>%1</B> does <B>not</B> exist.<BR/>Should it be created?</QT>").arg(dir)) == KMessageBox::No) {
		bdirLabel->setText(tmp);
		return false;
	    }
	    if (mkdir(dir.data(), 0700) != 0) {
		QString msg = i18n("<QT>The specified folder<BR/><B>%1</B><BR/>could <B>not</B> be created");
		switch (errno) {
		    case EACCES:
		    case EPERM:
		    case EROFS:
			msg += i18n(", because you either don't have sufficient rights to do that, or the filesystem is readonly.");
			// Insufficient permissions/ readonly FS
			break;
		    case ENOSPC:
			msg += i18n(", because the filesystem has not enough space.");
			// No space
			break;
		    case EEXIST:
			// shouldn't happen, we checked already
			// for existence.
			msg += i18n(", because there already exists another object with the same name.");
			break;
		    case EFAULT:
		    case ENOMEM:
		    case ENAMETOOLONG:
			// shouldn't happen.
			msg += ".";
			break;
		    case ENOENT:
			// propably dangling symlink
			msg += i18n(", because you specified a path which probably contains a dangling symbolic link.");
			break;
		    case ENOTDIR:
			msg += i18n(", because you specified a path which contains an element which is not a folder.");
			// path element not dir.
			break;
		    case ELOOP:
			msg += i18n(", because you specified a path which contains too many symbolic links.");
			// Too many symlinks
			break;


		}
		bdirLabel->setText(tmp);
		msg += i18n("<BR/>Please select another folder.</QT>");
		KMessageBox::error(this, msg.arg(dir));
		return false;
	    }
	    bdirCreated = dir;
	}
	QDir od(oldBDir);
	if ((!oldBDir.isEmpty()) && (oldBDir != dir) && (od.exists())) {
	    QStringList entries = od.entryList();
	    QStringList::Iterator ui;
	    QStringList::Iterator ei;
	    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_UIDS));
	    QStringList uids = config->readListEntry(
		pcfg.getOptionName(KPsionConfig::OPT_UIDS));
	    for (ei = entries.begin(); ei != entries.end(); ++ei) {
		for (ui = uids.begin(); ui != uids.end(); ++ui) {
		    if ((*ei) == (*ui)) {
			QString from = oldBDir;
			QString to = dir;

			from += "/"; from += *ui;
			to += "/"; to += *ui;
			if (::rename(from.latin1(), to.latin1()) != 0) {
			    KMessageBox::error(this,
					       i18n("Could not move existing backup "
						    "for machine %1 to %2.").arg(*ui).arg(to));
			}
		    }
		}
	    }
	}
	if (rmFlag)
	    ::rmdir(oldBDir.latin1());
	bdirLabel->setText(dir);
	oldBDir = dir;
	return true;
    }
    bdirLabel->setText(tmp);
    return false;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
