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
#ifndef _SETUPDIALOGS_H_
#define _SETUPDIALOGS_H_

#include <rfsv.h>
#include <rpcs.h>

#include <kdialogbase.h>
#include <knuminput.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <klistview.h>

#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlabel.h>

class SetupDialog : public KDialogBase {
    Q_OBJECT

public:
    SetupDialog(QWidget *parent, rfsv *plpRfsv, rpcs *plpRpcs);
    bool showPage(int index);

protected:
    virtual void closeEvent(QCloseEvent *e);

private slots:
    void slotDefaultClicked();
    void slotSaveSettings();
    void slotBdirBrowse();
    void slotDeleteMachine();
    void slotMachineChanged(int);

private:
    bool checkBackupDir(QString &dir);

    QFrame *page1;
    QFrame *page2;
    QFrame *page3;
    QFrame *page4;

    QLabel  *bdirLabel;
    KIntSpinBox *genSpin;
    KIntSpinBox *rconSpin;
    QPushButton *bdirButton;
    QPushButton *mdelButton;
    QCheckBox *remCheck;
    KComboBox *iIntCombo;
    KComboBox *fIntCombo;
    KComboBox *devCombo;
    KComboBox *speedCombo;
    KComboBox *machCombo;
    KLineEdit *nameEdit;
    KListView *backupListView;

    QString bdirDefault;
    QString bdirCreated;
    QString oldBDir;
};

#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
