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
#ifndef _KPSION_H_
#define _KPSION_H_

#include "setupdialog.h"
#include "statusbarprogress.h"

#include <kmainwindow.h>
#include <kcmdlineargs.h>
#include <kiconview.h>
#include <klistview.h>
#include <kdialogbase.h>
#include <ktar.h>

#include <qtextstream.h>
#include <rfsv.h>
#include <rpcs.h>
#include <ppsocket.h>

typedef QMap<char,QString> driveMap;
typedef QMap<QString,QString> psionMap;

class KPsionCheckListItem : public QObject, public QCheckListItem {
    Q_OBJECT

public:
    KPsionCheckListItem(KPsionCheckListItem *parent, const QString &text,
			Type tt)
	: QCheckListItem(parent, text, tt) { init(true); }
    KPsionCheckListItem(QCheckListItem *parent, const QString &text, Type tt)
	: QCheckListItem(parent, text, tt) { init(false); }
    KPsionCheckListItem(QListViewItem *parent, const QString &text, Type tt)
	: QCheckListItem(parent, text, tt) { init(false); }
    KPsionCheckListItem(QListView *parent, const QString &text, Type tt)
	: QCheckListItem(parent, text, tt) { init(false); }
    KPsionCheckListItem(QListViewItem *parent, const QString &text,
			const QPixmap &p)
	: QCheckListItem(parent, text, p) { init(false); }
    KPsionCheckListItem(QListView *parent, const QString &text,
			const QPixmap &p)
	: QCheckListItem(parent, text, p) { init(false); }

    KPsionCheckListItem *firstChild() const;
    KPsionCheckListItem *nextSibling() const;

    ~KPsionCheckListItem();

    virtual QString key(int column, bool ascending) const;
    void setMetaData(int bType, time_t bTime, QString tarName, int size,
		     u_int32_t tHi, u_int32_t tLo, u_int32_t attr);

    QString unixname();
    QString psionname();
    QString tarname();
    PlpDirent plpdirent();
    int backupType();
    int size();
    time_t when();
    bool isPathChecked(QString path);

signals:
    void rootToggled(void);

protected:
    virtual void stateChange(bool);
    void propagateUp(bool);
    void propagateDown(bool);

private:
    void init(bool);
    class KPsionCheckListItemMetaData;
    KPsionCheckListItemMetaData *meta;
};

class KPsionBackupListView : public KListView {
    Q_OBJECT

public:
    enum backupTypes {
	UNKNOWN = 0,
	FULL = 1,
	INCREMENTAL = 2,
    };

    KPsionBackupListView(QWidget *parent = 0, const char *name = 0);
    KPsionCheckListItem *firstChild() const;

    void readBackups(QString uid);
    PlpDir &getRestoreList(QString tarname);
    QStringList getSelectedTars();
    bool autoSelect(QString drive);

signals:
    void itemsEnabled(bool);

private slots:
    void slotRootToggled(void);

private:
    void collectEntries(KPsionCheckListItem *i);
    void listTree(KPsionCheckListItem *cli, const KTarEntry *te,
		  QTextIStream &idx, int level);

    QString uid;
    QString backupDir;
    PlpDir toRestore;
};

class KPsionRestoreDialog : public KDialogBase {
    Q_OBJECT

public:
    KPsionRestoreDialog(QWidget *parent, QString uid);

    PlpDir &getRestoreList(QString tarname);
    QStringList getSelectedTars();
    bool autoSelect(QString drive);

private slots:
    void slotBackupsSelected(bool);

private:
    KPsionBackupListView *backupView;
};

class KPsionMainWindow : public KMainWindow {
    Q_OBJECT

public:
    KPsionMainWindow();
    ~KPsionMainWindow();
    void setMachineName(QString &_name) { machineName = _name; }
    QString getMachineUID();
    driveMap &getDrives() { return drives; }
    psionMap &getMachines() { return machines; }
    QString &getMachineName() { return machineName; }
    QString &getBackupDir() { return backupDir; }
    bool isConnected() { return connected; }
    const KTarEntry *findTarEntry(const KTarEntry *te, QString path,
				  QString rpath = "");

    static QString unix2psion(const char * const);
    static QString psion2unix(const char * const);

signals:
    void setProgress(int);
    void setProgress(int, int);
    void setProgressText(const QString &);
    void enableProgressText(bool);

public slots:
    void slotStartRestore();
    void slotStartFullBackup();
    void slotStartIncBackup();
    void slotStartFormat();
    void slotToggleToolbar();
    void slotToggleStatusbar();
    void slotSaveOptions();
    void slotPreferences();
    void slotProgressBarPressed();

protected:
    virtual bool queryClose();
    void setupActions();
    void switchActions();
    void queryPsion();
    void insertDrive(char letter, const char * const name);

private slots:
    void iconClicked(QIconViewItem *i);
    void iconOver(QIconViewItem *i);
    void slotUpdateTimer();

private:
    void doBackup();
    void tryConnect();
    void updateProgress(unsigned long);
    void collectFiles(QString dir);
    void killSave();
    void runRestore();
    void createIndex();
    bool askOverwrite(PlpDirent e);

    rfsv *plpRfsv;
    rpcs *plpRpcs;
    ppsocket *rfsvSocket;
    ppsocket *rpcsSocket;
    SetupDialog *setupDialog;
    KIconView *view;
    KPsionStatusBarProgress *progress;
    KTarGz *backupTgz;
    KCmdLineArgs *args;

    driveMap drives;
    psionMap machines;
    QStringList overWriteList;
    QStringList backupDrives;
    QStringList savedCommands;
    QString backupDir;
    QString machineName;
    QString statusMsg;
    QString ncpdDevice;
    QString progressTotalText;
    bool S5mx;
    bool backupRunning;
    bool restoreRunning;
    bool formatRunning;
    bool lastSelected;
    bool connected;
    bool firstTry;
    bool shuttingDown;
    bool fullBackup;
    bool overWriteAll;
    int reconnectTime;
    int nextTry;
    int ncpdSpeed;
    unsigned long long machineUID;
    PlpDir toBackup;
    unsigned long backupSize;
    unsigned long backupCount;
    unsigned long progressTotal;
    unsigned long progressLocal;
    unsigned long progressCount;
    unsigned long progressLocalCount;
    int progressPercent;
    int progressLocalPercent;
};

#endif
/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
