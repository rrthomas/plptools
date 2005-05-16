/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 1999  Philip Proudman <philip.proudman@btinternet.com>
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
#ifndef _PLPPROPS_H_
#define _PLPPROPS_H_

#include <qstring.h>
#include <qptrlist.h>
#include <qgroupbox.h>
#include <qmultilineedit.h>

#include <kurl.h>
#include <kfileitem.h>
#include <kdialogbase.h>
#include <kpropsdlg.h>
#include <krun.h>
#include <kio/job.h>

#include "pie3dwidget.h"

class PlpPropsPlugin : public KPropsDlgPlugin {
    Q_OBJECT

public:
    /**
     * Constructor
     */
    PlpPropsPlugin( KPropertiesDialog *_props );
    virtual ~PlpPropsPlugin();

    /**
     * Applies all changes made.
     */
    virtual void applyChanges();

    /**
     * Tests whether the files specified by _items need a 'General' plugin.
     */
    static bool supports(KFileItemList _items);

signals:
    void save();

private slots:
    void doChange();

private:
    class PlpPropsPluginPrivate;
    PlpPropsPluginPrivate *d;
};

class PlpFileAttrPage : public QObject {
    Q_OBJECT

public:
    /**
     * Constructor
     */
    PlpFileAttrPage(KPropertiesDialog *_props);
    virtual ~PlpFileAttrPage();

    static bool supports(KFileItemList _items);

public slots:
    void applyChanges();

signals:
    void changed();

private slots:
    void slotGetSpecialFinished(KIO::Job *job);
    void slotSetSpecialFinished(KIO::Job *job);
    void slotCbToggled(bool);

private:
    class PlpFileAttrPagePrivate;
    PlpFileAttrPagePrivate *d;
};

class PlpDriveAttrPage : public QObject {
    Q_OBJECT

public:
    /**
     * Constructor
     */
    PlpDriveAttrPage(KPropertiesDialog *_props);
    virtual ~PlpDriveAttrPage();

    static bool supports(KFileItemList _items);

private slots:
    void slotSpecialFinished(KIO::Job *job);
    void slotBackupClicked();
    void slotRestoreClicked();
    void slotFormatClicked();

private:
    class PlpDriveAttrPagePrivate;
    PlpDriveAttrPagePrivate *d;

};


/**
 * Used to view machine info.
 */
class PlpMachinePage : public QObject {
    Q_OBJECT

public:
    /**
     * Constructor
     */
    PlpMachinePage(KPropertiesDialog *_props);
    virtual ~PlpMachinePage();

    static bool supports(KFileItemList _items);

private slots:
    void slotJobData(KIO::Job *job, const QByteArray &data);
    void slotJobFinished(KIO::Job *job);

private:
    class PlpMachinePagePrivate;
    PlpMachinePagePrivate *d;

    QLabel *makeEntry(QString text, QWidget *w, int y);
};

/**
 * Used to view owner info
 */
class PlpOwnerPage : public QObject {
    Q_OBJECT

public:
    /**
     * Constructor
     */
    PlpOwnerPage(KPropertiesDialog *_props);
    virtual ~PlpOwnerPage();

    static bool supports(KFileItemList _items);

private slots:
void slotSpecialFinished(KIO::Job *job);

private:
    class PlpOwnerPagePrivate;
    PlpOwnerPagePrivate *d;
};

#endif

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
