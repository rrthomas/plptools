/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 2001 Fritz Elfert <felfert@to.com>
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

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>

#include "kpsionconfig.h"
#include "wizards.h"

#include <kapplication.h>
#include <kdebug.h>
#include <kdialog.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kmessagebox.h>

#include <qlayout.h>
#include <qwhatsthis.h>
#include <qheader.h>

FirstTimeWizard::FirstTimeWizard(QWidget *parent, const char *name)
    : KWizard(parent, name, true)
{
    QStringList sl;
    KPsionConfig pcfg;

    setCaption(i18n("KPsion Configuration"));
    QWhatsThis::add(nextButton(),
		    i18n("Click this button to continue with the next page."));
    QWhatsThis::add(backButton(),
		    i18n("Click this button, to go to a previous page."));
    QWhatsThis::add(cancelButton(),
		    i18n("<QT>If you click this button, the setup of <B>KPSion</B> will be aborted and next time you start <B>KPsion</B>, it will run this setup again.</QT>"));

    bdirDefault = pcfg.getStrDefault(KPsionConfig::DEF_BACKUPDIR);
    bdirCreated = "";

    // Page 1
    page1 = new QWidget(this, "welcome");
    QGridLayout *grid = new QGridLayout(page1);

    QLabel *l = new QLabel(page1, "welcome message");
    l->setText(i18n(
	"<QT>"
	"<H2>Welcome to KPsion!</H2>"
	"It looks like you started <B>KPsion</B> the first time. "
	"At least, i could not find any valid configuration.</BR>"
	"On the following pages, we will gather some information, "
	"which is needed for working with <B>KPsion</B>.</BR>"
	" </BR>"
	"Have fun."
	"</QT>"
	));
    grid->addWidget(l, 1, 1, Qt::AlignTop);
    grid->setColStretch(1, 1);
    grid->setRowStretch(1, 1);
    grid->addRowSpacing(0, KDialog::marginHint());
    grid->addRowSpacing(2, KDialog::marginHint());
    grid->addColSpacing(0, KDialog::marginHint());
    grid->addColSpacing(2, KDialog::marginHint());
    addPage(page1, i18n("<QT><BIG><B>Welcome<B></BIG></QT>"));

    // Page 2
    page2 = new QWidget(this, "step1");
    grid = new QGridLayout(page2);

    l = new QLabel(page2, "step1");
    l->setText(i18n(
	"<QT>"
	"First, we need a folder for storing backups of "
	"your Psion. You probably don't want others to "
	"have access to it, so it's best to choose a "
	"location somewhere in your home directory. "
	"Please browse through existing folders and select a suitable "
	"location or simply accept the default shown below."
	"</QT>"
	));
    grid->addMultiCellWidget(l, 1, 1, 1, 2, Qt::AlignTop);

    bdirLabel = new QLabel(page2, "bdirLabel");
    bdirLabel->setText(bdirDefault);
    bdirButton = new QPushButton(i18n("Browse"), page2);

    QWhatsThis::add(bdirLabel,
		    i18n("This is the name of the backup folder."));
    QWhatsThis::add(bdirButton,
		    i18n("Click here, for opening a dialog which lets you easily select the backup folder."));
    grid->addWidget(bdirLabel, 3, 1);
    grid->addWidget(bdirButton, 3, 2);

    grid->setRowStretch(1, 1);
    grid->setColStretch(1, 1);

    grid->addRowSpacing(2, KDialog::spacingHint());

    grid->addRowSpacing(0, KDialog::marginHint());
    grid->addRowSpacing(4, KDialog::marginHint());
    grid->addColSpacing(0, KDialog::marginHint());
    grid->addColSpacing(3, KDialog::marginHint());

    connect(bdirButton, SIGNAL(clicked()), SLOT(slotBdirBrowse()));
    addPage(page2, i18n("<QT><BIG><B>Step 1</B></BIG> - Specify backup directory</QT>"));
    // Page 3
    page3 = new QWidget(this, "step2");
    grid = new QGridLayout(page3);

    l = new QLabel(page3, "step2");
    l->setText(i18n(
	"<QT>"
	"Next, please specify some information regarding "
	"backup policy:<UL><LI>How many generations of backups "
	"do you want to keep?</LI><LI>Should I perform automatic "
	"backups?</LI><LI>If yes, how often do you want backups"
	"to happen?</LI></UL>"
	"</QT>"
	));
    grid->addMultiCellWidget(l, 1, 1, 1, 2, Qt::AlignTop);

    l = new QLabel(
	i18n("&Incremental backup interval"), page3, "iBackupIntLabel");
    grid->addWidget(l, 3, 1);
    iIntCombo = new KComboBox(false, page3, "iIntCombo");
    iIntCombo->insertStringList(pcfg.getConfigBackupInterval());
    iIntCombo->setCurrentItem(pcfg.getIntDefault(KPsionConfig::DEF_INCINTERVAL));
    grid->addWidget(iIntCombo, 3, 2);
    l->setBuddy(iIntCombo);

    l = new QLabel(i18n("&Full backup interval"), page3, "fBackupIntLabel");
    grid->addWidget(l, 5, 1);
    fIntCombo = new KComboBox(false, page3, "fIntCombo");
    fIntCombo->insertStringList(pcfg.getConfigBackupInterval());
    fIntCombo->setCurrentItem(pcfg.getIntDefault(KPsionConfig::DEF_FULLINTERVAL));
    grid->addWidget(fIntCombo, 5, 2);
    l->setBuddy(fIntCombo);

    l = new QLabel(i18n("Backup &generations"), page3, "backupGenLabel");
    grid->addWidget(l, 7, 1);
    genSpin = new KIntSpinBox(0, 10, 1,
			      pcfg.getIntDefault(KPsionConfig::DEF_BACKUPGEN),
			      10, page3, "backupGenSpin");
    grid->addWidget(genSpin, 7, 2);
    l->setBuddy(genSpin);

    grid->setRowStretch(1, 1);
    grid->setColStretch(1, 1);

    grid->addRowSpacing(2, KDialog::spacingHint());
    grid->addRowSpacing(4, KDialog::spacingHint());
    grid->addRowSpacing(6, KDialog::spacingHint());

    grid->addRowSpacing(0, KDialog::marginHint());
    grid->addRowSpacing(8, KDialog::marginHint());
    grid->addColSpacing(0, KDialog::marginHint());
    grid->addColSpacing(3, KDialog::marginHint());

    addPage(page3, i18n("<QT><BIG><B>Step 2</B></BIG> - Backup policy</QT>"));

    // Page 4
    page4 = new QWidget(this, "step3");
    grid = new QGridLayout(page4);

    l = new QLabel(page4, "step2");
    l->setText(i18n(
	"<QT>"
	"If no connection could be established on startup, "
	"<B>KPsion</B> will attempt to connect in regular "
	"intervals. Please specify the interval after which "
	"a connection attempt should happen. If you don't want "
	"automatic retry, set the interval to zero. Furthermore, "
	"<B>KPsion</B> can try to start ncpd if it is not already "
	"running. For that to work correctly, you need to"
	"<UL><LI>specify the serial port to use.</LI>"
	"<LI>specify the baud rate</LI>"
	"<LI>have permission to use the specified port</LI></UL>"
	"</QT>"
	));
    grid->addMultiCellWidget(l, 1, 1, 1, 2, Qt::AlignTop);

    l = new QLabel(
	i18n("&Connection retry interval (sec.)"), page4, "rconLabel");
    grid->addWidget(l, 3, 1);
    rconSpin = new KIntSpinBox(0, 600, 1,
			       pcfg.getIntDefault(KPsionConfig::DEF_CONNRETRY),
			       10, page4, "rconSpin");
    grid->addWidget(rconSpin, 3, 2);
    l->setBuddy(rconSpin);

    l = new QLabel(i18n("Serial &device"), page4, "devLabel");
    grid->addWidget(l, 5, 1);
    devCombo = new KComboBox(false, page4, "devCombo");
    sl = pcfg.getConfigDevices();
    devCombo->insertStringList(sl);
    devCombo->setCurrentItem(pcfg.getIntDefault(KPsionConfig::DEF_SERIALDEV));
    grid->addWidget(devCombo, 5, 2);
    l->setBuddy(devCombo);

    l = new QLabel(i18n("Serial &speed"), page4, "speedLabel");
    grid->addWidget(l, 7, 1);
    speedCombo = new KComboBox(false, page4, "speedCombo");
    sl = pcfg.getConfigSpeeds();
    speedCombo->insertStringList(sl);
    speedCombo->setCurrentItem(pcfg.getIntDefault(KPsionConfig::DEF_SERIALSPEED));
    grid->addWidget(speedCombo, 7, 2);
    l->setBuddy(speedCombo);

    grid->setRowStretch(1, 1);
    grid->setColStretch(1, 1);

    grid->addRowSpacing(2, KDialog::spacingHint());
    grid->addRowSpacing(4, KDialog::spacingHint());
    grid->addRowSpacing(6, KDialog::spacingHint());

    grid->addRowSpacing(0, KDialog::marginHint());
    grid->addRowSpacing(8, KDialog::marginHint());
    grid->addColSpacing(0, KDialog::marginHint());
    grid->addColSpacing(3, KDialog::marginHint());

    addPage(page4,
	    i18n("<QT><BIG><B>Step 3</B></BIG> - Connection parameters</QT>"));

    // Page 5
    page5 = new QWidget(this, "step3");
    grid = new QGridLayout(page5);

    l = new QLabel(page5, "step2");
    l->setText(i18n(
	"<QT>"
	"That's it!<BR/>"
	"Now I will start <B>KPsion</B> and if your Psion is already "
	"connected and its communication link turned on (use"
	"<B>Ctrl-L</B> on the System screen), then <B>KPsion</B> will "
	"bring up a dialog similar to this which lets you assign it a "
	"name. After that, I suggest you perform a full backup.<BR/>"
	"Please click <B>Finish</B> now.</QT>"
	));
    grid->addWidget(l, 1, 1, Qt::AlignTop);

    grid->setRowStretch(1, 1);
    grid->setColStretch(1, 1);

    grid->addRowSpacing(0, KDialog::marginHint());
    grid->addRowSpacing(2, KDialog::marginHint());
    grid->addColSpacing(0, KDialog::marginHint());
    grid->addColSpacing(2, KDialog::marginHint());

    addPage(page5, i18n("<QT><BIG><B>Finished</B></BIG></QT>"));

    setFinishEnabled(page5, true);
}

void FirstTimeWizard::
slotBdirBrowse() {
    QString dir = KFileDialog::getExistingDirectory(bdirLabel->text(), this,
						    i18n("Backup folder"));
    checkBackupDir(dir);
}

void FirstTimeWizard::
reject() {
    // kapp->quit() and [QK]Application::exit(0) don't work here?!
    // probably because we didn't call kapp->exec() yet?
    // -> brute force
    if (KMessageBox::questionYesNo(this,
				   i18n("<QT>You are about to abort the initial setup of <B>KPsion</B>. No configuration will be stored and you will have to repeat this procedure when you start <B>KPsion</B> next time.<BR/>Do you really want to exit now?</QT>")) == KMessageBox::Yes) {
	if (!bdirCreated.isEmpty())
	    ::rmdir(bdirCreated.data());
	::exit(0);
    }
}

void FirstTimeWizard::
accept() {
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
    config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_SERIALDEV),
		       devCombo->currentText());

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_SERIALSPEED));
    config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_SERIALSPEED),
		       speedCombo->currentText());

    hide();
    setResult(Accepted);
}

void FirstTimeWizard::
next() {
    for (int i = 0; i < pageCount(); i++)
	if (currentPage() == page(i)) {
	    switch (i) {
		case 1:
		    QString dir(bdirLabel->text());
		    if (!checkBackupDir(dir))
			return;
	    }
	    break;
	}
    KWizard::next();
}

void FirstTimeWizard::
closeEvent(QCloseEvent *e) {
    reject();
}

bool FirstTimeWizard::
checkBackupDir(QString &dir) {
    if (!bdirCreated.isEmpty()) {
	if (bdirCreated != dir) {
	    ::rmdir(bdirCreated.data());
	    bdirCreated = "";
	}
    }
    if (!dir.isEmpty()) {
	QDir d(dir);
	if (!d.exists()) {
	    if (KMessageBox::questionYesNo(this,
					   i18n("<QT>The folder <B>%1</B> does <B>not</B> exist.<BR/>Shall it be created?</QT>").arg(dir)) == KMessageBox::No) {
		bdirLabel->setText(bdirDefault);
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
		bdirLabel->setText(bdirDefault);
		msg += i18n("<BR/>Please select another folder.</QT>");
		KMessageBox::error(this, msg.arg(dir));
		return false;
	    }
	    bdirCreated = dir;
	}
	bdirLabel->setText(dir);
	return true;
    }
    bdirLabel->setText(bdirDefault);
    return false;
}

NewPsionWizard::NewPsionWizard(QWidget *parent, const char *name)
    : KWizard(parent, name, true) {

	setCaption(i18n("New Psion detected"));
	psion = (KPsionMainWindow *)parent;

	QWhatsThis::add(nextButton(),
			i18n("Click this button to continue with the next page."));
	QWhatsThis::add(backButton(),
			i18n("Click this button, to go to a previous page."));
	QWhatsThis::add(cancelButton(),
			i18n("<QT>If you click this button, the setup for the new connected Psion will be aborted and next time you connect this Psion again, <B>KPsion</B> will run this setup again.</QT>"));

	// Page 1
	page1 = new QWidget(this, "newmachine");
	QGridLayout *grid = new QGridLayout(page1);

	QLabel *l = new QLabel(page1, "newmachmessage");
	uid = psion->getMachineUID();
	l->setText(i18n(
	    "<QT>"
	    "The Psion with the unique ID <B>%1</B> "
	    "is connected the first time. Please assign a name to it."
	    "</QT>").arg(uid));
	grid->addMultiCellWidget(l, 1, 1, 1, 2, Qt::AlignTop);

	l = new QLabel(page1, "nameLabel");
	l->setText(i18n("&Name of new Psion"));
	nameEdit = new KLineEdit(page1, "nameEdit");
	nameEdit->setText(i18n("My new Psion"));
	nameEdit->selectAll();
	nameEdit->setFocus();
	l->setBuddy(nameEdit);
	grid->addWidget(l, 3, 1);
	grid->addWidget(nameEdit, 3, 2);

	grid->setColStretch(1, 1);
	grid->setRowStretch(1, 1);

	grid->addRowSpacing(2, KDialog::spacingHint());

	grid->addRowSpacing(0, KDialog::marginHint());
	grid->addRowSpacing(4, KDialog::marginHint());
	grid->addColSpacing(0, KDialog::marginHint());
	grid->addColSpacing(2, KDialog::marginHint());

	addPage(page1, i18n("<QT><BIG><B>New Psion detected<B></BIG></QT>"));

	// Page 2
	page2 = new QWidget(this, "bdrives");
	grid = new QGridLayout(page2);

	l = new QLabel(page2, "bdrivemessage");
	l->setText(i18n(
	    "<QT>"
	    "Please select the Drive(s), you want to be backed up when "
	    "running in unattended backup mode."
	    "</QT>"
	    ));
	grid->addMultiCellWidget(l, 1, 1, 1, 3, Qt::AlignTop);

	backupListView = new KListView(page2, "bdriveListView");
	backupListView->addColumn(i18n("Available drives"));
	driveMap dlist = psion->getDrives();
	driveMap::Iterator it;
	int height = backupListView->header()->height();
	for (it = dlist.begin(); it != dlist.end(); it++) {
	    QCheckListItem *i = new QCheckListItem(backupListView, it.data(),
						   QCheckListItem::CheckBox);
	    height += i->height();
	    i->setSelectable(false);
	}
	backupListView->setMaximumSize(backupListView->columnWidth(0) + 5, height + 5);
	grid->addWidget(backupListView, 3, 2);

	grid->setColStretch(1, 1);
	grid->setRowStretch(1, 1);
	grid->setColStretch(3, 1);

	grid->addRowSpacing(2, KDialog::spacingHint());

	grid->addRowSpacing(0, KDialog::marginHint());
	grid->addRowSpacing(4, KDialog::marginHint());
	grid->addColSpacing(0, KDialog::marginHint());
	grid->addColSpacing(4, KDialog::marginHint());

	addPage(page2, i18n("<QT><BIG><B>Specify drives to backup<B></BIG></QT>"));

	setFinishEnabled(page2, true);
}

void NewPsionWizard::
next() {
    for (int i = 0; i < pageCount(); i++)
	if (currentPage() == page(i)) {
	    switch (i) {
		case 0:
		    QString tmp(nameEdit->text());
		    if (!checkPsionName(tmp))
			return;
	    }
	    break;
	}
    KWizard::next();
}

bool NewPsionWizard::
checkPsionName(QString &name) {
    if (name.isEmpty()) {
	KMessageBox::sorry(this, i18n("The name cannot be empty."));
	return false;
    }
    psionMap l = psion->getMachines();
    psionMap::Iterator it;
    for (it = l.begin(); it != l.end(); it++) {
	if (name == it.data()) {
	    KMessageBox::sorry(this, i18n("<QT>The name <B>%1</B> is already assigned to another machine.<BR/>Please choose a different name.</QT>"));
	    return false;
	}
    }
    return true;
}

void NewPsionWizard::
accept() {
    KConfig *config = kapp->config();
    KPsionConfig pcfg;

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_UIDS));
    QStringList machines = config->readListEntry(
	pcfg.getOptionName(KPsionConfig::OPT_UIDS));
    machines += uid;
    config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_UIDS), machines);
    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_MACHNAME));
    QString tmp = pcfg.getOptionName(KPsionConfig::OPT_MACHNAME).arg(uid);
    config->writeEntry(tmp, nameEdit->text());
    tmp = nameEdit->text();
    psion->setMachineName(tmp);
    driveMap dlist = psion->getDrives();
    driveMap::Iterator di;
    QStringList drives;
    for (di = dlist.begin(); di != dlist.end(); di++) {
	QString drv = "";
	drv += di.key();
	drives += drv;
    }
    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_DRIVES));
    config->writeEntry(
	pcfg.getOptionName(KPsionConfig::OPT_DRIVES).arg(uid), drives);

    QListViewItemIterator li(backupListView);
    QStringList bdrives;
    for (; li.current(); li++) {
	QCheckListItem *qcli = (QCheckListItem *)(li.current());
	if (qcli->isOn()) {
	    tmp = qcli->text();
	    for (di = dlist.begin(); di != dlist.end(); di++)
		if (di.data() == tmp) {
		    QString drv = "";
		    drv += di.key();
		    bdrives += drv;
		}
	}
    }
    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPDRIVES));
    config->writeEntry(
	pcfg.getOptionName(KPsionConfig::OPT_BACKUPDRIVES).arg(uid), bdrives);
    hide();
    setResult(Accepted);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
