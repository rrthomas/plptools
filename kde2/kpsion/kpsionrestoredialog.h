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
#ifndef _KPSIONRESTOREDIALOG_H_
#define _KPSIONRESTOREDIALOG_H_

#include "kpsionbackuplistview.h"

#include <kdialogbase.h>

#include <qstringlist.h>
#include <qcheckbox.h>

#include <rfsv.h>

typedef QMap<char,QString> driveMap;
typedef QMap<QString,QString> psionMap;

class KPsionRestoreDialog : public KDialogBase {
    Q_OBJECT

public:
    KPsionRestoreDialog(QWidget *parent, QString uid);

    PlpDir &getRestoreList(QString tarname);
    QStringList getSelectedTars();
    QStringList getFormatDrives();
    bool autoSelect(QString drive);

private slots:
    void slotBackupsSelected(bool);

private:
    KPsionBackupListView *backupView;
    QCheckBox *fmtCheck;
};
#endif
