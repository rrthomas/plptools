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

#include "kpsion.h"
#include "kpsionbackuplistview.h"
#include "kpsionconfig.h"

#include <kapplication.h>
#include <klocale.h>
#include <kconfig.h>
#include <kiconloader.h>

#include <qwhatsthis.h>
#include <qiodevice.h>
#include <qheader.h>
#include <qdir.h>

class KPsionCheckListItem::KPsionCheckListItemMetaData {
    friend class KPsionCheckListItem;

private:
    KPsionCheckListItemMetaData();
    ~KPsionCheckListItemMetaData() { };

    bool parentIsKPsionCheckListItem;
    bool dontPropagate;
    bool gray;
    int backupType;
    int size;
    time_t when;
    u_int32_t timeHi;
    u_int32_t timeLo;
    u_int32_t attr;
    QString name;
};

KPsionCheckListItem::KPsionCheckListItemMetaData::KPsionCheckListItemMetaData() {
    when = 0;
    size = 0;
    timeHi = 0;
    timeLo = 0;
    attr = 0;
    gray = false;
    name = QString::null;
    backupType = KPsionBackupListView::UNKNOWN;
}

KPsionCheckListItem::~KPsionCheckListItem() {
    delete meta;
}

/**
 * Must re-implement this since QT3 does not leave the expansion square
 * enabled anymore if the item is disabled.
 */
void KPsionCheckListItem::
paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int align)
{
    QColorGroup tmp(cg);
    if (meta->gray)
	tmp.setColor(QColorGroup::Text,
		     listView()->palette().color(QPalette::Disabled,
						 QColorGroup::Text));
    else
	tmp.setColor(QColorGroup::Text,
		     listView()->palette().color(QPalette::Normal,
						 QColorGroup::Text));
    QCheckListItem::paintCell(p, tmp, column, width, align);
}

KPsionCheckListItem *KPsionCheckListItem::
firstChild() const {
    return (KPsionCheckListItem *)QListViewItem::firstChild();
}

KPsionCheckListItem *KPsionCheckListItem::
nextSibling() const {
    return (KPsionCheckListItem *)QListViewItem::nextSibling();
}

void KPsionCheckListItem::
init(bool myparent) {
    setSelectable(false);
    meta = new KPsionCheckListItemMetaData();
    meta->dontPropagate = false;
    meta->parentIsKPsionCheckListItem = myparent;
}

void KPsionCheckListItem::
setMetaData(int type, time_t when, QString name, int size,
	    u_int32_t tHi, u_int32_t tLo, u_int32_t attr) {
	meta->backupType = type;
	meta->when = when;
	meta->name = name;
	meta->size = size;
	meta->timeHi = tHi;
	meta->timeLo = tLo;
	meta->attr = attr;
}

void KPsionCheckListItem::
stateChange(bool state) {
    if (!state)
	meta->gray = false;
    QCheckListItem::stateChange(state);
    if (meta->dontPropagate)
	return;
    if (meta->parentIsKPsionCheckListItem)
	((KPsionCheckListItem *)QListViewItem::parent())->propagateUp(state);
    else
	emit rootToggled();
    propagateDown(state);
}

void KPsionCheckListItem::
propagateDown(bool state) {
    setOn(state);
    KPsionCheckListItem *child = firstChild();
    while (child) {
	child->meta->dontPropagate = true;
	child->propagateDown(state);
	child->meta->dontPropagate = false;
	child = child->nextSibling();
    }
}

void KPsionCheckListItem::
propagateUp(bool state) {
    bool makeGray = false;

    KPsionCheckListItem *child = firstChild();
    while (child) {
	if (child->isOn() != state) {
	    makeGray = true;
	    break;
	}
	child = child->nextSibling();
    }
    meta->dontPropagate = true;
    if (makeGray) {
	meta->gray = true;
	setOn(true);
    } else {
	meta->gray = false;
	setOn(state);
    }
    // Bug in QListView? It doesn't update, when
    // enabled/disabled without activating.
    // -> force it.
    listView()->repaintItem(this);
    meta->dontPropagate = false;
    if (meta->parentIsKPsionCheckListItem)
	((KPsionCheckListItem *)QListViewItem::parent())->propagateUp(state);
    else
	emit rootToggled();
}

QString KPsionCheckListItem::
key(int column, bool ascending) const {
    if (meta->when) {
	QString tmp;
	tmp.sprintf("%08d", meta->when);
	return tmp;
    }
    return text();
}

QString KPsionCheckListItem::
psionname() {
    if (meta->parentIsKPsionCheckListItem)
	return meta->name;
    else
	return QString::null;
}

QString KPsionCheckListItem::
unixname() {
    if (meta->parentIsKPsionCheckListItem)
	return KPsionMainWindow::psion2unix(meta->name);
    else
	return QString::null;
}

QString KPsionCheckListItem::
tarname() {
    if (meta->parentIsKPsionCheckListItem)
	return ((KPsionCheckListItem *)QListViewItem::parent())->tarname();
    else
	return meta->name;
}

QString KPsionCheckListItem::
psionpath() {
    QString tmp = text();
    QCheckListItem *p = this;
    while (p->depth() > 1) {
	p = (QCheckListItem *)p->parent();
	tmp = p->text() + "/" + tmp;
    }
    return KPsionMainWindow::unix2psion(tmp);
}

int KPsionCheckListItem::
backupType() {
    if (meta->parentIsKPsionCheckListItem)
	return ((KPsionCheckListItem *)QListViewItem::parent())->backupType();
    else
	return meta->backupType;
}

time_t KPsionCheckListItem::
when() {
    if (meta->parentIsKPsionCheckListItem)
	return ((KPsionCheckListItem *)QListViewItem::parent())->when();
    else
	return meta->when;
}

PlpDirent KPsionCheckListItem::
plpdirent() {
    assert(meta->parentIsKPsionCheckListItem);
    return PlpDirent(meta->size, meta->attr, meta->timeHi, meta->timeLo,
		     meta->name);
}

KPsionBackupListView::KPsionBackupListView(QWidget *parent, const char *name)
    : KListView(parent, name) {

    toRestore.clear();
    uid = QString::null;
    KConfig *config = kapp->config();
    KPsionConfig pcfg;

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPDIR));
    backupDir = config->readEntry(
	pcfg.getOptionName(KPsionConfig::OPT_BACKUPDIR));
    addColumn(i18n("Available backups"));
    setRootIsDecorated(true);
    setSorting(-1);
}

KPsionCheckListItem *KPsionBackupListView::
firstChild() const {
    return (KPsionCheckListItem *)QListView::firstChild();
}

void KPsionBackupListView::
readBackups(QString uid) {
    QString bdir(backupDir);
    bdir += "/";
    bdir += uid;
    QDir d(bdir);
    kapp->processEvents();
    const QFileInfoList *fil =
	d.entryInfoList("*.tar.gz", QDir::Files|QDir::Readable, QDir::Time);
    QFileInfoListIterator it(*fil);
    QFileInfo *fi;
    while ((fi = it.current())) {
	kapp->processEvents();

	bool isValid = false;
#if KDE_VERSION >= 300
	KTar tgz(fi->absFilePath());
#else
	KTarGz tgz(fi->absFilePath());
#endif
	const KTarEntry *te;
	QString bTypeName;
	int bType;
	QDateTime date;

	tgz.open(IO_ReadOnly);
	te = tgz.directory()->entry("KPsionFullIndex");
	if (te && (!te->isDirectory())) {
	    date.setTime_t(te->date());
	    bTypeName = i18n("Full");
	    bType = FULL;
	    isValid = true;
	} else {
	    te = tgz.directory()->entry("KPsionIncrementalIndex");
	    if (te && (!te->isDirectory())) {
		date.setTime_t(te->date());
		bTypeName = i18n("Incremental");
		bType = INCREMENTAL;
		isValid = true;
	    }
	}

	if (isValid) {
	    QString n =	i18n("%1 backup, created at %2").arg(bTypeName).arg(KGlobal::locale()->formatDateTime(date, false));
	    QByteArray a = ((KTarFile *)te)->data();
	    QTextIStream indexData(a);

	    indexData.readLine();
	    indexData.flags(QTextStream::hex);
	    indexData.fill('0');
	    indexData.width(8);

	    KPsionCheckListItem *i =
		new KPsionCheckListItem(this, n,
					KPsionCheckListItem::CheckBox);
	    i->setMetaData(bType, te->date(), tgz.fileName(), 0, 0, 0, 0);
	    i->setPixmap(0, KGlobal::iconLoader()->loadIcon("tgz",
							 KIcon::Small));
	    connect(i, SIGNAL(rootToggled()), this, SLOT(slotRootToggled()));

	    indexDataList_t idList;
	    while (!indexData.atEnd()) {
		indexData_t id;

		indexData >> id.timeHi >> id.timeLo >> id.size >> id.attr;
		id.name = indexData.readLine().mid(1);
		idList.push_back(id);
	    }

	    QStringList files = tgz.directory()->entries();
	    for (QStringList::Iterator f = files.begin();
		 f != files.end(); f++)
		if ((*f != "KPsionFullIndex") &&
		    (*f != "KPsionIncrementalIndex"))
		    listTree(i, tgz.directory()->entry(*f), idList, 0);
	}
	tgz.close();
	++it;
    }
    header()->setClickEnabled(false);
    header()->setResizeEnabled(false);
    header()->setMovingEnabled(false);
    setMinimumSize(columnWidth(0) + 4, height());
    QWhatsThis::add(this, i18n(
			"<qt>Here, you can select the available backups."
			" Select the items you want to restore, the click"
			" on <b>Start</b> to start restoring these items."
			"</qt>"));
}

void KPsionBackupListView::
listTree(KPsionCheckListItem *cli, const KTarEntry *te, indexDataList_t &idx,
	 int level) {
    KPsionCheckListItem *i =
	new KPsionCheckListItem(cli, te->name(),
				KPsionCheckListItem::CheckBox);
    kapp->processEvents();
    if (te->isDirectory()) {
	if (level)
	    i->setPixmap(0, KGlobal::iconLoader()->loadIcon("folder",
							    KIcon::Small));
	else
	    i->setPixmap(0, KGlobal::iconLoader()->loadIcon("hdd_unmount",
							    KIcon::Small));
	i->setMetaData(0, 0, QString::null, 0, 0, 0, 0);
	KTarDirectory *td = (KTarDirectory *)te;
	QStringList files = td->entries();
	for (QStringList::Iterator f = files.begin(); f != files.end(); f++)
	    listTree(i, td->entry(*f), idx, level + 1);
    } else {
	indexDataList_t::iterator ii;

	QString name = i->psionpath();
	for (ii = idx.begin(); ii != idx.end(); ii++) {
	    if (ii->name == name) {
		i->setMetaData(0, 0, name, ii->size, ii->timeHi, ii->timeLo,
			       ii->attr);
		break;
	    }
	}

	i->setPixmap(0, KGlobal::iconLoader()->loadIcon("mime_empty",
							KIcon::Small));
    }
}

void KPsionBackupListView::
slotRootToggled() {
    bool anyOn = false;
    KPsionCheckListItem *i = firstChild();
    while (i != 0L) {
	if (i->isOn()) {
	    anyOn = true;
	    break;
	}
	i = i->nextSibling();
    }
    emit itemsEnabled(anyOn);
}

QStringList KPsionBackupListView::
getSelectedTars() {
    QStringList l;

    KPsionCheckListItem *i = firstChild();
    while (i != 0L) {
	if (i->isOn())
	    l += i->tarname();
	i = i->nextSibling();
    }
    return l;
}

QStringList KPsionBackupListView::
getFormatDrives() {
    QStringList l;

    KPsionCheckListItem *i = firstChild();
    while (i != 0L) {
	if (i->isOn()) {
	    KPsionCheckListItem *c = i->firstChild();
	    while (c != 0L) {
		if (c->isOn()) {
		    QString drv = c->text().left(1);
		    if (l.find(drv) == l.end())
			l += drv;
		}
		c = c->nextSibling();
	    }
	}
	i = i->nextSibling();
    }
    return l;
}

bool KPsionBackupListView::
autoSelect(QString drive) {
    KPsionCheckListItem *latest = NULL;
    time_t stamp = 0;

    drive += ":";
    // Find latest full backup for given drive
    KPsionCheckListItem *i = firstChild();
    while (i != 0L) {
	if ((i->backupType() == FULL) && (i->when() > stamp)) {
	    KPsionCheckListItem *c = i->firstChild();
	    while (c != 0L) {
		if (c->text() == drive) {
		    latest = c;
		    stamp = i->when();
		    break;
		}
		c = c->nextSibling();
	    }
	}
	i = i->nextSibling();
    }
    // Now find all incremental backups for given drive which are newer than
    // selected backup
    if (latest != 0) {
	latest->setOn(true);
	i = firstChild();
	while (i != 0L) {
	    if ((i->backupType() == INCREMENTAL) && (i->when() >= stamp)) {
		KPsionCheckListItem *c = i->firstChild();
		while (c != 0L) {
		    if (c->text() == drive)
			c->setOn(true);
		    c = c->nextSibling();
		}
	    }
	    i = i->nextSibling();
	}
    }
    return (latest != 0L);
}

void KPsionBackupListView::
collectEntries(KPsionCheckListItem *i) {
    while (i != 0L) {
	KPsionCheckListItem *c = i->firstChild();
	if (c == 0L) {
	    if (i->isOn())
		toRestore.push_back(i->plpdirent());
	} else
	    collectEntries(c);
	i = i->nextSibling();
    }
}

PlpDir &KPsionBackupListView::
getRestoreList(QString tarname) {
    toRestore.clear();
    KPsionCheckListItem *i = firstChild();
    while (i != 0L) {
	if ((i->tarname() == tarname) && (i->isOn())) {
	    collectEntries(i->firstChild());
	    break;
	}
	i = i->nextSibling();
    }
    return toRestore;
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
