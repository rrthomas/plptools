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

#include "kpsionrestoredialog.h"

#include <klocale.h>

#include <qlayout.h>

KPsionRestoreDialog::KPsionRestoreDialog(QWidget *parent, QString uid)
    : KDialogBase(parent, "restoreDialog", true, i18n("Restore"),
		  KDialogBase::Cancel | KDialogBase::Ok,
		  KDialogBase::Ok, true) {

    setButtonOKText(i18n("Start"));
    enableButtonOK(false);
    setButtonWhatsThis(KDialogBase::Ok,
		       i18n("Select items in the list of"
			    " available backups, then click"
			    " here to start restore of these"
			    " items."));

    QWidget *w = new QWidget(this);
    setMainWidget(w);
    QGridLayout *gl = new QGridLayout(w, 1, 1, KDialog::marginHint(),
				      KDialog::marginHint());
    backupView = new KPsionBackupListView(w, "restoreSelector");
    gl->addWidget(backupView, 0, 0);
    fmtCheck = new QCheckBox(i18n("Format drive before restore"), w, "fmtCheck");
    gl->addWidget(fmtCheck, 1, 0);
    backupView->readBackups(uid);
    connect(backupView, SIGNAL(itemsEnabled(bool)), this,
	    SLOT(slotBackupsSelected(bool)));
}

void KPsionRestoreDialog::
slotBackupsSelected(bool any) {
    enableButtonOK(any);
}

QStringList KPsionRestoreDialog::
getSelectedTars() {
    return backupView->getSelectedTars();
}

bool KPsionRestoreDialog::
autoSelect(QString drive) {
    return backupView->autoSelect(drive);
}

QStringList KPsionRestoreDialog::
getFormatDrives() {
    if (fmtCheck->isChecked())
	return backupView->getFormatDrives();
    return QStringList();
}

PlpDir &KPsionRestoreDialog::
getRestoreList(QString tarname) {
    return backupView->getRestoreList(tarname);
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
