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
#ifndef _KPSIONBACKUPLISTVIEW_H_
#define _KPSIONBACKUPLISTVIEW_H_

#include <rfsv.h>

#include <klistview.h>
#include <ktar.h>

#include <qdatetime.h>
#include <qcheckbox.h>
#include <qstringlist.h>
#include <qtextstream.h>

#include <vector>

typedef struct {
    u_int32_t timeHi;
    u_int32_t timeLo;
    u_int32_t attr;
    u_int32_t size;
    QString name;
} indexData_t;

typedef vector<indexData_t> indexDataList_t;

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
    QString psionpath();
    virtual void paintCell(QPainter *, const QColorGroup &, int, int, int);

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
    QStringList getFormatDrives();
    bool autoSelect(QString drive);

signals:
    void itemsEnabled(bool);

private slots:
    void slotRootToggled(void);

private:
    void collectEntries(KPsionCheckListItem *i);
    void listTree(KPsionCheckListItem *cli, const KTarEntry *te,
		  indexDataList_t &idx, int level);
    QString uid;
    QString backupDir;
    PlpDir toRestore;
};
#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
