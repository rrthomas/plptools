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

#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kmessagebox.h>

#include <qlayout.h>
#include <qwhatsthis.h>

#include <errno.h>

SetupDialog::SetupDialog(QWidget *parent, rfsv *plpRfsv, rpcs *plpRpcs)
    : KDialogBase(Tabbed, "Settings", Ok|Apply|Default|Cancel, Ok, parent,
		  "settingsDialog", true, true)
{
    int i;
    QString tmp;
    QStringList sl;
    QStringList::Iterator sli;
    KPsionConfig pcfg;

    enableLinkedHelp(false);

    KConfig *config = kapp->config();

    // Page 1
    page1 = addPage(i18n("Backup &folder"));
    QGridLayout *grid = new QGridLayout(page1, 1, 1,
					marginHint() * 2, spacingHint() * 2);

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPDIR));
    tmp = config->readEntry(pcfg.getOptionName(KPsionConfig::OPT_BACKUPDIR));
    bdirLabel = new QLabel(page1, "bdirLabel");
    bdirLabel->setText(tmp);
    bdirButton = new QPushButton(i18n("Browse"), page1);

    QWhatsThis::add(bdirLabel,
		    i18n("This is the name of the backup folder."));
    QWhatsThis::add(bdirButton,
		    i18n("Click here, for opening a dialog which lets you easily select the backup folder."));
    grid->addWidget(bdirLabel, 0, 1);
    grid->addWidget(bdirButton, 0, 2);

    grid->setColStretch(1, 1);

    connect(bdirButton, SIGNAL(clicked()), SLOT(slotBdirBrowse()));


    // Page 2
    page2 = addPage(i18n("Backup &policy"));
    grid = new QGridLayout(page2);

    QLabel *l = new QLabel(i18n("&Incremental backup reminder"), page2,
		   "iBackupIntLabel");
    grid->addWidget(l, 3, 1);

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_INCINTERVAL));
    i = config->readNumEntry(pcfg.getOptionName(KPsionConfig::OPT_INCINTERVAL));
    iIntCombo = new KComboBox(false, page2, "iIntCombo");
    iIntCombo->insertItem(i18n("none"));
    iIntCombo->insertItem(i18n("daily"));
    iIntCombo->insertItem(i18n("every 2 days"));
    iIntCombo->insertItem(i18n("every 3 days"));
    iIntCombo->insertItem(i18n("every 4 days"));
    iIntCombo->insertItem(i18n("every 5 days"));
    iIntCombo->insertItem(i18n("every 6 days"));
    iIntCombo->insertItem(i18n("weekly"));
    iIntCombo->insertItem(i18n("every 2 weeks"));
    iIntCombo->insertItem(i18n("every 3 weeks"));
    iIntCombo->insertItem(i18n("monthly"));
    iIntCombo->setCurrentItem(i);
    grid->addWidget(iIntCombo, 3, 2);
    l->setBuddy(iIntCombo);

    l = new QLabel(i18n("&Full backup reminder"), page2, "fBackupIntLabel");
    grid->addWidget(l, 5, 1);

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_FULLINTERVAL));
    i = config->readNumEntry(pcfg.getOptionName(KPsionConfig::OPT_FULLINTERVAL));
    fIntCombo = new KComboBox(false, page2, "fIntCombo");
    fIntCombo->insertItem(i18n("none"));
    fIntCombo->insertItem(i18n("daily"));
    fIntCombo->insertItem(i18n("every 2 days"));
    fIntCombo->insertItem(i18n("every 3 days"));
    fIntCombo->insertItem(i18n("every 4 days"));
    fIntCombo->insertItem(i18n("every 5 days"));
    fIntCombo->insertItem(i18n("every 6 days"));
    fIntCombo->insertItem(i18n("weekly"));
    fIntCombo->insertItem(i18n("every 2 weeks"));
    fIntCombo->insertItem(i18n("every 3 weeks"));
    fIntCombo->insertItem(i18n("monthly"));
    fIntCombo->setCurrentItem(i);
    grid->addWidget(fIntCombo, 5, 2);
    l->setBuddy(fIntCombo);

    l = new QLabel(i18n("Backup &generations"), page2, "backupGenLabel");
    grid->addWidget(l, 7, 1);

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPGEN));
    i = config->readNumEntry(pcfg.getOptionName(KPsionConfig::OPT_BACKUPGEN));
    genSpin = new KIntSpinBox(0, 10, 1, i, 10, page2, "backupGenSpin");
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

    // Page 3
    page3 = addPage(i18n("&Connection"));
    grid = new QGridLayout(page3);

    l = new QLabel(i18n("&Connection retry interval (sec.)"), page3,
		   "rconLabel");
    grid->addWidget(l, 3, 1);

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_CONNRETRY));
    i = config->readNumEntry(pcfg.getOptionName(KPsionConfig::OPT_CONNRETRY));
    rconSpin = new KIntSpinBox(0, 600, 1, i, 10, page3, "rconSpin");
    grid->addWidget(rconSpin, 3, 2);
    l->setBuddy(rconSpin);

    l = new QLabel(i18n("Serial &device"), page3, "devLabel");
    grid->addWidget(l, 5, 1);

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_SERIALDEV));
    tmp = config->readEntry(pcfg.getOptionName(KPsionConfig::OPT_SERIALDEV));

    devCombo = new KComboBox(false, page3, "devCombo");
    sl = pcfg.getConfigDevices();
    devCombo->insertStringList(sl);
    for (i = 0, sli = sl.begin(); sli != sl.end(); ++i, ++sli) {
	if (*sli == tmp)
	    devCombo->setCurrentItem(i);
    }
    grid->addWidget(devCombo, 5, 2);
    l->setBuddy(devCombo);

    l = new QLabel(i18n("Serial &speed"), page3, "speedLabel");
    grid->addWidget(l, 7, 1);

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_SERIALSPEED));
    tmp = config->readEntry(pcfg.getOptionName(KPsionConfig::OPT_SERIALSPEED));

    speedCombo = new KComboBox(false, page3, "speedCombo");
    sl = pcfg.getConfigSpeeds();
    speedCombo->insertStringList(sl);
    for (i = 0, sli = sl.begin(); sli != sl.end(); ++i, ++sli) {
	if (*sli == tmp)
	    speedCombo->setCurrentItem(i);
    }
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

    connect(this, SIGNAL(okClicked()), SLOT(slotSaveSettings()));
    connect(this, SIGNAL(applyClicked()), SLOT(slotSaveSettings()));
}

void SetupDialog::
slotDefaultClicked() {
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
    config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_SERIALDEV),
		       devCombo->currentText());

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_SERIALSPEED));
    config->writeEntry(pcfg.getOptionName(KPsionConfig::OPT_SERIALSPEED),
		       speedCombo->currentText());
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
	bdirLabel->setText(dir);
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
