#ifndef _KPSION_H_
#define _KPSION_H_

#include "setupdialog.h"

#include <kmainwindow.h>
#include <kiconview.h>
#include <kprogress.h>
#include <ksqueezedtextlabel.h>
#include <klistview.h>
#include <ktar.h>

#include <rfsv.h>
#include <rpcs.h>
#include <ppsocket.h>

typedef QMap<char,QString> driveMap;
typedef QMap<QString,QString> psionMap;

class KPsionCheckListItem : public QCheckListItem {
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
    void setMetaData(int, time_t);

 protected:
    virtual void stateChange(bool);
    void propagateUp(bool);
    void propagateDown(bool);

 private:
    void init(bool);

    bool parentIsKPsionCheckListItem;
    bool dontPropagate;
    int backupType;
    time_t when;
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
    void readBackups(QString uid);
    PlpDir &getRestoreList();

 private:
    void listTree(KPsionCheckListItem *cli, const KTarEntry *te, int level);

    QString uid;
    QString backupDir;
    PlpDir toRestore;
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

 public slots:
    void slotStartRestore();
    void slotStartFullBackup();
    void slotStartIncBackup();
    void slotStartFormat();
    void slotToggleToolbar();
    void slotToggleStatusbar();
    void slotSaveOptions();
    void slotPreferences();
 
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

    rfsv *plpRfsv;
    rpcs *plpRpcs;
    ppsocket *rfsvSocket;
    ppsocket *rpcsSocket;
    SetupDialog *setupDialog;
    KIconView *view;
    KProgress *progress;
    KSqueezedTextLabel *progressLabel;
    KTarGz *backupTgz;

    driveMap drives;
    psionMap machines;
    QStringList backupDrives;
    QStringList savedCommands;
    QString backupDir;
    QString machineName;
    QString statusMsg;
    QString ncpdDevice;
    bool S5mx;
    bool backupRunning;
    bool restoreRunning;
    bool formatRunning;
    bool lastSelected;
    bool connected;
    bool firstTry;
    bool shuttingDown;
    bool fullBackup;
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
