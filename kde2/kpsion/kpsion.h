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
#include "kpsionrestoredialog.h"

#include <kmainwindow.h>
#include <kcmdlineargs.h>
#include <kiconview.h>
#include <kdialogbase.h>

#include <rfsv.h>
#include <rpcs.h>
#include <ppsocket.h>

typedef QMap<char,QString> driveMap;
typedef QMap<QString,QString> psionMap;

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
    bool shouldQuit();
    static QString unix2psion(const char * const);
    static QString psion2unix(const char * const);

signals:
    void setProgress(int);
    void setProgress(int, int);
    void setProgressText(const QString &);
    void enableProgressText(bool);
    void rearrangeIcons(bool);

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
    void slotAutoAction();

private:
    void doBackup();
    void tryConnect();
    void updateProgress(unsigned long);
    void collectFiles(QString dir);
    void killSave();
    void runRestore();
    void createIndex();
    bool askOverwrite(PlpDirent e);
    void setDriveName(const char dchar, QString dname);
    void doFormat(QString drive);
    void updateBackupStamps();
    void startupNcpd();
    void removeOldBackups(QStringList &drives);
    void syncTime(QString uid);

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
    QString ncpdSpeed;
    QString ncpdPath;
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
    bool doScheduledBackup;
    bool overWriteAll;
    bool quitImmediately;
    int reconnectTime;
    int nextTry;
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
