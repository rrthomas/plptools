/*-*-c++-*-
 * $Id$
 *
 * This file is part of plptools.
 *
 *  Copyright (C) 2001-2002 Fritz Elfert <felfert@to.com>
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

#include "plpprops.h"
#include "pie3dwidget.h"

#include <kdebug.h>
#include <klocale.h>
#include <kio/slaveinterface.h>
#include <krun.h>

#include <qlayout.h>
#include <qlabel.h>
#include <qobjectlist.h>
#include <qtabwidget.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

#include <strstream>
#include <iomanip>

#ifdef ENABLE_NLS
#define _PLPINTL_H_ // Override NLS stuff in headers from libplp
static inline QString X_(const char *t) {
    return KGlobal::locale()->translate(t);
}
static inline QString _(const char *t) {
    return KGlobal::locale()->translate(t);
}
static inline const char *N_(const char *t) { return t; }
#endif

#include <rfsv.h>
#include <rpcs.h>

#define PLP_CMD_DRIVEINFO 1
#define PLP_CMD_OWNERINFO 2
#define PLP_CMD_GETATTR   3
#define PLP_CMD_SETATTR   4
#define PLP_CMD_MACHINFO  5

#define KIO_ARGS QByteArray packedArgs; \
QDataStream stream( packedArgs, IO_WriteOnly ); stream

class PlpPropsPlugin::PlpPropsPluginPrivate {
public:
    PlpPropsPluginPrivate() { }
    ~PlpPropsPluginPrivate() { }

    QFrame *frame;
};

/*
 * A VERY UGLY HACK for removing the Permissions-Page from
 * the Properties dialog.
 */
static void
removeDialogPage(QWidget *theDialog, QString theLabel) {
    QObject *qtabwidget = 0L;
    QFrame *permframe = 0L;

    //
    // First, find the QTabWidget in the dialog.
    // Easy, cause there's only one.
    //
    QObjectList *l = theDialog->queryList("QTabWidget");
    QObjectListIt it(*l);
    QObject * obj;
    while ((obj=it.current()) != 0) {
	++it;
	qtabwidget = obj;
    }
    delete l;

    // Now, walk thru all Childs of the QTabWidget which are
    // inherited from class QFrame. For every found frame,
    // compare the tabLabel against the required label (&Permissions).
    //
    if (qtabwidget != 0L) {
	l = qtabwidget->queryList("QFrame");
	QObjectListIt it(*l);

	while ((obj = it.current()) != 0) {
	    if (theLabel ==
		((QTabWidget *)qtabwidget)->tabLabel((QWidget*)obj)) {
		permframe = (QFrame *)obj;
		break;
	    }
	    ++it;
	}
	delete l;

	// If we found it, remove it.
	if (permframe != 0)
	    ((QTabWidget *)qtabwidget)->removePage(permframe);
    }
}

PlpPropsPlugin::PlpPropsPlugin(KPropertiesDialog *_props)
    : KPropsDlgPlugin( _props )
{
    d = new PlpPropsPluginPrivate;
    bool removePerms = false;
    bool removeGeneral = false;

    if (!supports(properties->items()))
	return;

    if (PlpFileAttrPage::supports(properties->items())) {
	PlpFileAttrPage *p = new PlpFileAttrPage(_props);
	connect(p, SIGNAL(changed()), SLOT(doChange()));
	connect(this, SIGNAL(save()), p, SLOT(applyChanges()));
	removePerms = true;
    }
    if (PlpDriveAttrPage::supports(properties->items())) {
	PlpDriveAttrPage *p = new PlpDriveAttrPage(_props);
	removePerms = true;
    }
    if (PlpMachinePage::supports(properties->items())) {
	PlpMachinePage *p = new PlpMachinePage(_props);
	removePerms = true;
	removeGeneral = true;
    }
    if (PlpOwnerPage::supports(properties->items())) {
	PlpOwnerPage *p = new PlpOwnerPage(_props);
	removePerms = true;
	removeGeneral = true;
    }
    if (removePerms)
	removeDialogPage(properties->dialog(), i18n("&Permissions"));
    if (removeGeneral)
	removeDialogPage(properties->dialog(), i18n("&General"));
}

PlpPropsPlugin::~PlpPropsPlugin() {
    delete d;
}

bool PlpPropsPlugin::supports(KFileItemList _items) {
    for (KFileItemListIterator it(_items); it.current(); ++it) {
	KFileItem *fi = it.current();

	if (fi->url().protocol() != "psion")
	    return false;
    }
    return true;
}

void PlpPropsPlugin::applyChanges() {
    emit save();
}

void PlpPropsPlugin::doChange() {
    emit changed();
}

class PlpFileAttrPage::PlpFileAttrPagePrivate {

    typedef struct {
	const char * const lbl;
	const char * const tip;
	const u_int32_t mask;
	bool inverted;
	bool direnabled;
	bool s5enabled;
    } UIelem;

public:

    PlpFileAttrPagePrivate();
    ~PlpFileAttrPagePrivate() {}

    KPropertiesDialog *props;

    bool  jobReturned;
    u_int32_t flags;
    u_int32_t attr;

    const UIelem *generic;
    const UIelem *s3;
    const UIelem *s5;

    QFrame *frame;
    QLabel *psiPath;
    QCheckBox *genCb[5];  // MUST match initializers below!!!
    QCheckBox *specCb[3]; // MUST match initializers below!!!
};

PlpFileAttrPage::PlpFileAttrPagePrivate::PlpFileAttrPagePrivate() {
    int i;

    static const UIelem _generic[] = {
	{
	    I18N_NOOP("Readable"),
	    I18N_NOOP("If this is checked, read permission is granted. On series 5, this cannot swiched off."),
	    rfsv::PSI_A_READ,
	    false, false, false
	}, // Fake for S5
	{
	    I18N_NOOP("Writeable"),
	    I18N_NOOP("If this is checked, write permission is granted."),
	    rfsv::PSI_A_RDONLY,
	    true,  true,  true
	},
	{
	    I18N_NOOP("Hidden"),
	    I18N_NOOP("If this is checked, the file is not shown when displaying the directory on the Psion."),
	    rfsv::PSI_A_HIDDEN,
	    false, true,  true
	},
	{
	    I18N_NOOP("System"),
	    I18N_NOOP("If this is checked, the file is not shown when displaying the directory on the Psion."),
	    rfsv::PSI_A_SYSTEM,
	    false, false, true
	},
	{
	    I18N_NOOP("Archive"),
	    I18N_NOOP("If this is checked, the file will be included in the next incremental backup."),
	    rfsv::PSI_A_ARCHIVE,
	    false, true,  true
	},
	{ 0L, 0L, 0L, false, false, true  },
    };
    static const UIelem _s3[] = {
	{
	    I18N_NOOP("Executable"),
	    I18N_NOOP("If this is checked, the file can be executed on the Psion. This Attribute does not apply to directories."),
	    rfsv::PSI_A_EXEC,
	    false, false, true
	},
	{
	    I18N_NOOP("Stream"),
	    I18N_NOOP("If this is checked, the file is a stream. This Attribute does not apply to directories."),
	    rfsv::PSI_A_STREAM,
	    false, false, true
	},
	{
	    I18N_NOOP("Text"),
	    I18N_NOOP("If this is checked, the file is opened in text mode. This Attribute does not apply to directories."),
	    rfsv::PSI_A_TEXT,
	    false, false, true
	},
	{ 0L, 0L, 0L, false, false, true },
    };
    static const UIelem _s5[] = {
	{
	    I18N_NOOP("Normal"),
	    I18N_NOOP("If this is checked, the file is considered regular. This Attribute does not apply to directories."),
	    rfsv::PSI_A_NORMAL,
	    false, false, true
	},
	{
	    I18N_NOOP("Temporary"),
	    I18N_NOOP("If this is checked, the file considered temporary. This Attribute does not apply to directories."),
	    rfsv::PSI_A_TEMP,
	    false, false, true
	},
	{
	    I18N_NOOP("Compressed"),
	    I18N_NOOP("If this is checked, the file is stored in compressed mode. This Attribute does not apply to directories."),
	    rfsv::PSI_A_COMPRESSED,
	    false, false, true
	},
	{ 0L, 0L, 0L, false, false, true },
    };
    generic = _generic;
    s3 = _s3;
    s5 = _s5;
}

PlpFileAttrPage::PlpFileAttrPage(KPropertiesDialog *_props) {
    QGridLayout *mgl;
    QGridLayout *gl;
    QGroupBox *gb;
    QLabel *l;
    int i;

    d = new PlpFileAttrPagePrivate;
    d->props = _props;
    d->frame = _props->dialog()->addPage(i18n("Psion &Attributes"));

    mgl = new QGridLayout(d->frame, 1, 1, KDialog::marginHint(),
			  KDialog::spacingHint(), "mainLayout");

    l = new QLabel(i18n("Path on Psion:"), d->frame, "psiPathLabel");
    mgl->addWidget(l, 0, 0);

    d->psiPath = new QLabel(QString("?"), d->frame, "psiPath");
    mgl->addWidget(d->psiPath, 0, 1);
    mgl->setColStretch(1, 1);

    gb = new QGroupBox(i18n("Generic attributes"), d->frame, "genattrBox");
    mgl->addMultiCellWidget(gb, 1, 1, 0, 1);
    gl = new QGridLayout (gb, 1, 1, KDialog::marginHint(),
			  KDialog::spacingHint(), "genattrLayout");
    for (i = 0; d->generic[i].lbl; i++) {
	QString lbl = KGlobal::locale()->translate(d->generic[i].lbl);
	d->genCb[i] = new QCheckBox(lbl, gb, d->generic[i].lbl);
	QWhatsThis::add(d->genCb[i],
			KGlobal::locale()->translate(d->generic[i].tip));
	d->genCb[i]->setEnabled(false);
	connect(d->genCb[i], SIGNAL(toggled(bool)), SLOT(slotCbToggled(bool)));
	gl->addWidget(d->genCb[i], 0, i);
    }

    gb = new QGroupBox(i18n("Machine specific attributes"), d->frame,
		       "specattrBox");
    mgl->addMultiCellWidget(gb, 2, 2, 0, 1);
    gl = new QGridLayout (gb, 1, 1, KDialog::marginHint(),
			  KDialog::spacingHint(), "specattrLayout");
    for (i = 0; d->s5[i].lbl; i++) {
	QString lbl = KGlobal::locale()->translate(d->s5[i].lbl);
	d->specCb[i] = new QCheckBox(lbl, gb, d->s5[i].lbl);
	d->specCb[i]->setEnabled(false);
	connect(d->specCb[i], SIGNAL(toggled(bool)), SLOT(slotCbToggled(bool)));
	gl->addWidget(d->specCb[i], 0, i);
    }
    mgl->addRowSpacing(3, KDialog::marginHint());

    d->jobReturned = false;
    KIO_ARGS << int(PLP_CMD_GETATTR) << _props->item()->url().path();
    KIO::StatJob *job = new KIO::StatJob(KURL("psion:/"), KIO::CMD_SPECIAL,
					 packedArgs, false);
    connect(job, SIGNAL(result(KIO::Job *)),
	    SLOT(slotGetSpecialFinished(KIO::Job *)));
}

PlpFileAttrPage::~PlpFileAttrPage() {
    delete d;
}

bool PlpFileAttrPage::supports(KFileItemList _items) {
    for (KFileItemListIterator it(_items); it.current(); ++it) {
	KFileItem *fi = it.current();

	QString path = fi->url().path(-1);
	if (path.contains('/') == 1)
	    return false;
    }
    return true;
}

void PlpFileAttrPage::applyChanges() {
    u_int32_t attr = 0;
    bool isS5 = ((d->flags & 1) != 0);
    bool val;
    int i;

    for (i = 0; d->generic[i].lbl; i++) {
	val = d->genCb[i]->isChecked();
	if (d->generic[i].inverted)
	    val = !val;
	if (val)
	    attr |= d->generic[i].mask;
    }
    if (isS5)
	for (i = 0; d->s5[i].lbl; i++) {
	    val = d->specCb[i]->isChecked();
	    if (d->s5[i].inverted)
		val = !val;
	    if (val)
		attr |= d->s5[i].mask;
	}
    else
	for (i = 0; d->s3[i].lbl; i++) {
	    val = d->specCb[i]->isChecked();
	    if (d->s3[i].inverted)
		val = !val;
	    if (val)
		attr |= d->s3[i].mask;
	}
    if (d->attr != attr) {
	u_int32_t mask = d->attr ^ attr;
	u_int32_t sattr = attr & mask;
	u_int32_t dattr = ~sattr & mask;

	KIO_ARGS << int(PLP_CMD_SETATTR) << sattr << dattr
		 << d->props->item()->url().path();
	KIO::SimpleJob *sjob = new KIO::SimpleJob(KURL("psion:/"),
						  KIO::CMD_SPECIAL, packedArgs,
						  false);
	connect(sjob, SIGNAL(result(KIO::Job *)),
		SLOT(slotSetSpecialFinished(KIO::Job *)));
    }
}

void PlpFileAttrPage::slotCbToggled(bool) {
    if (d->jobReturned)
	emit changed();
}

void PlpFileAttrPage::slotSetSpecialFinished(KIO::Job *job) {
    KIO::SimpleJob *sJob = static_cast<KIO::SimpleJob *>(job);

    if (sJob->error())
	job->showErrorDialog(d->props->dialog());
}

void PlpFileAttrPage::slotGetSpecialFinished(KIO::Job *job) {
    KIO::StatJob *sJob = static_cast<KIO::StatJob *>(job);

    if (sJob->error())
	job->showErrorDialog(d->props->dialog());
    else {
	KIO::UDSEntry e = sJob->statResult();
	bool attr_found = false;
	bool flags_found = false;
	u_int32_t flags;
	u_int32_t attr;

	for (KIO::UDSEntry::ConstIterator it = e.begin(); it != e.end(); ++it) {
	    if ((*it).m_uds == KIO::UDS_SIZE) {
		attr_found = true;
		attr = (unsigned long)((*it).m_long);
	    }
	    if ((*it).m_uds == KIO::UDS_CREATION_TIME) {
		flags_found = true;
		flags = (unsigned long)((*it).m_long);
	    }
	    if ((*it).m_uds == KIO::UDS_NAME)
		d->psiPath->setText((*it).m_str);
	}
	if (attr_found && flags_found) {
	    bool isS5 = ((flags & 1) != 0);
	    bool isRom = ((flags & 2) != 0);
	    bool noDir = ((attr & rfsv::PSI_A_DIR) == 0);
	    int i;

	    d->attr = attr;
	    d->flags = flags;
	    for (i = 0; d->generic[i].lbl; i++) {
		bool val = ((attr & d->generic[i].mask) != 0);
		if (d->generic[i].inverted)
		    val = !val;
		d->genCb[i]->setChecked(val);
		if ((!isRom) && (d->generic[i].s5enabled) &&
		    (noDir || d->generic[i].direnabled)) {
		    d->genCb[i]->setEnabled(true);
		}
	    }
	    if (isS5)
		for (i = 0; d->s5[i].lbl; i++) {
		    bool val = ((attr & d->s5[i].mask) != 0);
		    QWhatsThis::add(d->specCb[i],
				    KGlobal::locale()->translate(d->s5[i].tip));
		    d->specCb[i]->setChecked(val);
		    if ((!isRom) && (noDir || d->s5[i].direnabled))
			d->specCb[i]->setEnabled(true);
		}
	    else
		for (i = 0; d->s3[i].lbl; i++) {
		    bool val = ((attr & d->s3[i].mask) != 0);
		    QString lbl = KGlobal::locale()->translate(d->s3[i].lbl);
		    d->specCb[i]->setText(lbl);
		    QWhatsThis::add(d->specCb[i],
				    KGlobal::locale()->translate(d->s3[i].tip));
		    d->specCb[i]->setChecked(val);
		    if ((!isRom) && (noDir || d->s3[i].direnabled))
			d->specCb[i]->setEnabled(true);
		}
	}
    }
    d->jobReturned = true;
}

class PlpDriveAttrPage::PlpDriveAttrPagePrivate {
public:
    PlpDriveAttrPagePrivate() { }
    ~PlpDriveAttrPagePrivate() { }

    QColor  usedColor;
    QColor  freeColor;
    QString driveLetter;

    KPropertiesDialog *props;
    QFrame *frame;
    QGroupBox *gb;
    Pie3DWidget *pie;
    QLabel *typeLabel;
    QLabel *totalLabel;
    QLabel *freeLabel;
    QLabel *uidLabel;
    QPushButton *restoreButton;
    QPushButton *formatButton;
};

PlpDriveAttrPage::PlpDriveAttrPage(KPropertiesDialog *_props) {
    d = new PlpDriveAttrPagePrivate;
    d->props = _props;
    d->frame = _props->dialog()->addPage(i18n("Psion &Drive"));

    QBoxLayout *box = new QVBoxLayout(d->frame, KDialog::spacingHint());
    QLabel *l;
    QGridLayout *gl;
    QPushButton *b;
    d->usedColor = QColor(219, 58, 197);
    d->freeColor = QColor(39, 56, 167);
    d->driveLetter = "";

    d->gb = new QGroupBox(i18n("Information"), d->frame, "driveinfo");
    box->addWidget(d->gb);

    gl = new QGridLayout(d->gb, 7, 4, 15);
    gl->addRowSpacing(0, 10);

    l = new QLabel(i18n("Type"), d->gb, "typeLabel");
    gl->addWidget(l, 1, 0);

    d->typeLabel = new QLabel(d->gb);
    gl->addWidget(d->typeLabel, 2, 0);
    QWhatsThis::add(d->typeLabel,
		    i18n("The type of the drive is shown here."));

    l = new QLabel(i18n("Total capacity"), d->gb, "capacityLabel");
    gl->addWidget (l, 1, 1);

    d->totalLabel = new QLabel(i18n(" "), d->gb, "capacityValue");
    gl->addWidget(d->totalLabel, 2, 1);
    QWhatsThis::add(d->totalLabel,
		    i18n("This shows the total capacity of the drive."));

    l = new QLabel(i18n("Free space"), d->gb, "spaceLabel");
    gl->addWidget (l, 1, 2);

    d->freeLabel = new QLabel(i18n(" "), d->gb, "spaceValue");
    gl->addWidget(d->freeLabel, 2, 2);
    QWhatsThis::add(d->freeLabel,
		    i18n("This shows the available space of the drive."));

    l = new QLabel(i18n("Unique ID"), d->gb, "uidLabel");
    gl->addWidget (l, 1, 3);

    d->uidLabel = new QLabel(i18n(" "), d->gb, "uidValue");
    gl->addWidget(d->uidLabel, 2, 3);
    QWhatsThis::add(d->uidLabel,
		    i18n("This shows unique ID of the drive. For ROM drives, this is always 0."));

    d->pie = new Pie3DWidget(d->gb, "pie");
    QWhatsThis::add(d->pie,
		    i18n("Here, the usage of the drive is shown in a pie diagram. The purple area shows used space, the blue area shows free space."));

    gl->addMultiCellWidget(d->pie, 3, 4, 1, 2);

    QHBoxLayout *blay = new QHBoxLayout(KDialog::spacingHint(), "buttons");
    gl->addMultiCellLayout(blay, 5, 6, 1, 3);
    blay->setAlignment(AlignRight | AlignVCenter);

    b  = new QPushButton(i18n("Backup"), d->gb, "backupButton");
    connect(b, SIGNAL(clicked()), SLOT(slotBackupClicked()));
    blay->addWidget(b, 0);
    QWhatsThis::add(b,
		    i18n("Click here, to do a backup of this drive. This launches KPsion to perform that task."));

    b = new QPushButton(i18n("Restore"), d->gb, "restoreButton");
    connect(b, SIGNAL(clicked()), SLOT(slotRestoreClicked()));
    blay->addWidget(b, 0);
    d->restoreButton = b;
    QWhatsThis::add(b,
		    i18n("Click here, to do a restore of this drive. This launches KPsion to perform that task."));

    b = new QPushButton(i18n("Format"), d->gb, "formatButton");
    connect(b, SIGNAL(clicked()), SLOT(slotFormatClicked()));
    blay->addWidget(b, 0);
    d->formatButton = b;
    QWhatsThis::add(b,
		    i18n("Click here, to format this drive. This launches KPsion to perform that task."));

    blay->addStretch(10);

    box->addStretch(10);

    KIO_ARGS << int(PLP_CMD_DRIVEINFO) << _props->item()->url().path();
    KIO::StatJob *job = new KIO::StatJob(KURL("psion:/"), KIO::CMD_SPECIAL,
					 packedArgs, false);
    connect(job, SIGNAL(result(KIO::Job *)),
	    SLOT(slotSpecialFinished(KIO::Job *)));

}

PlpDriveAttrPage::~PlpDriveAttrPage() {
    delete d;
}

bool PlpDriveAttrPage::supports(KFileItemList _items) {
    for (KFileItemListIterator it(_items); it.current(); ++it) {
	KFileItem *fi = it.current();

	QString path = fi->url().path(-1);
	if (path.contains('/') != 1)
	    return false;
	if (fi->url().path() == "/")
	    return false;
    }
    return true;
}

void PlpDriveAttrPage::slotBackupClicked() {
    if (!d->driveLetter.isEmpty())
	KRun::runCommand(
	    QString("kpsion --backup %1").arg(d->driveLetter));
}

void PlpDriveAttrPage::slotRestoreClicked() {
    if (!d->driveLetter.isEmpty())
	KRun::runCommand(
	    QString("kpsion --restore %1").arg(d->driveLetter));
}

void PlpDriveAttrPage::slotFormatClicked() {
    if (!d->driveLetter.isEmpty())
	KRun::runCommand(
	    QString("kpsion --format %1").arg(d->driveLetter));
}

void PlpDriveAttrPage::slotSpecialFinished(KIO::Job *job) {
    KIO::StatJob *sJob = static_cast<KIO::StatJob *>(job);

    if (sJob->error())
	job->showErrorDialog(d->props->dialog());
    else {
	KIO::UDSEntry e = sJob->statResult();
	bool total_found = false;
	bool free_found = false;
	u_int32_t total;
	u_int32_t unused;

	for (KIO::UDSEntry::ConstIterator it = e.begin(); it != e.end(); ++it) {
	    if ((*it).m_uds == KIO::UDS_SIZE) {
		total_found = true;
		total = (unsigned long)((*it).m_long);
	    }
	    if ((*it).m_uds == KIO::UDS_MODIFICATION_TIME) {
		free_found = true;
		unused = (unsigned long)((*it).m_long);
	    }
	    if ((*it).m_uds == KIO::UDS_CREATION_TIME) {
		unsigned long uid = (unsigned long)((*it).m_long);
		d->uidLabel->setText(QString("%1").arg(uid, 8, 16));
	    }
	    if ((*it).m_uds == KIO::UDS_NAME) {
		QString name = ((*it).m_str);
		d->typeLabel->setText(name);
		if (name == "ROM") {
		    d->restoreButton->setEnabled(false);
		    d->formatButton->setEnabled(false);
		    // Can't change anything
		    removeDialogPage(d->props->dialog(), i18n("&General"));
		}
	    }
	    if ((*it).m_uds == KIO::UDS_USER) {
		d->driveLetter = ((*it).m_str);
		d->gb->setTitle(QString(i18n("Information for Psion drive %1: (%2)")).arg(d->driveLetter).arg(d->props->item()->name()));
	    }
	}
	if (total_found && free_found) {
	    d->totalLabel->setText(QString("%1 (%2)").arg(KIO::convertSize(total)).arg(KGlobal::locale()->formatNumber(total, 0)));
	    d->freeLabel->setText(QString("%1 (%2)").arg(KIO::convertSize(unused)).arg(KGlobal::locale()->formatNumber(unused, 0)));
	    d->pie->addPiece(total - unused, d->usedColor);
	    d->pie->addPiece(unused, d->freeColor);
	}
    }
}

class PlpMachinePage::PlpMachinePagePrivate {
public:
    PlpMachinePagePrivate() { }
    ~PlpMachinePagePrivate() { }

    KPropertiesDialog *props;

    QFrame *f;
    QGridLayout *g;

    QLabel *machType;
    QLabel *machName;
    QLabel *machUID;
    QLabel *machLang;

    QLabel *romVersion;
    QLabel *romSize;
    QLabel *romProg;

    QLabel *ramSize;
    QLabel *ramFree;
    QLabel *ramMaxFree;
    QLabel *ramDiskSz;

    QLabel *regSize;
    QLabel *dispGeo;
    QLabel *machTime;
    QLabel *machUTCo;
    QLabel *machDST;

    QLabel *mbattChanged;
    QLabel *mbattUsage;
    QLabel *mbattStatus;
    QLabel *mbattPower;
    QLabel *mbattCurrent;
    QLabel *mbattVoltage;
    QLabel *mbattMaxVoltage;

    QLabel *bbattStatus;
    QLabel *bbattVoltage;
    QLabel *bbattMaxVoltage;

    QLabel *epowerSupplied;
    QLabel *epowerUsage;

    rpcs::machineInfo mi;
};

QLabel *PlpMachinePage::
makeEntry(QString text, QWidget *w, int y) {
    QLabel *l = new QLabel(w);
    d->g->addWidget(new QLabel(text, w), y, 0);
    d->g->addWidget(l, y, 1);
    return l;
}

PlpMachinePage::PlpMachinePage( KPropertiesDialog *_props ) {
    d = new PlpMachinePagePrivate;
    d->props = _props;

    QGroupBox *gb;
    QBoxLayout *box;

    d->f = _props->dialog()->addPage(i18n("Psion &Machine"));

    box = new QVBoxLayout(d->f, KDialog::spacingHint());
    gb = new QGroupBox(i18n("General"), d->f, "genInfBox");
    box->addWidget(gb);
    d->g = new QGridLayout(gb, 8, 2, KDialog::marginHint(),
			   KDialog::spacingHint());

    d->machType = makeEntry(i18n("Machine type:"), gb, 1);
    QWhatsThis::add(d->machType,
		    i18n("Here, the type of the connected device is shown."));
    d->machName = makeEntry(i18n("Model name:"), gb, 2);
    QWhatsThis::add(d->machName,
		    i18n("Here, the model name of the connected device is shown."));
    d->machUID  = makeEntry(i18n("Machine UID:"), gb, 3);
    QWhatsThis::add(d->machUID,
		    i18n("Here, the unique ID of the connected device is shown."));
    d->machLang = makeEntry(i18n("UI language:"), gb, 4);
    QWhatsThis::add(d->machLang,
		    i18n("Here, the user interface language of the connected device is shown."));
    d->dispGeo  = makeEntry(i18n("Display geometry:"), gb, 5);
    QWhatsThis::add(d->dispGeo,
		    i18n("Here, the display geometry of the connected device is shown."));
    d->regSize  = makeEntry(i18n("Registry size:"), gb, 6);
    QWhatsThis::add(d->regSize,
		    i18n("Here, the size of the registry data is shown."));
    d->g->addRowSpacing(0, KDialog::marginHint());
    d->g->setColStretch(0, 1);
    d->g->setColStretch(1, 1);

    gb = new QGroupBox(i18n("Time"), d->f, "timeInfBox");
    box->addWidget(gb);
    d->g = new QGridLayout(gb, 5, 2, KDialog::marginHint(),
			   KDialog::spacingHint());
    d->machTime = makeEntry(i18n("Date/Time:"), gb, 1);
    QWhatsThis::add(d->machTime,
		    i18n("Here, the current time setting of the connected device is shown."));
    d->machUTCo = makeEntry(i18n("UTC offset"), gb, 2);
    QWhatsThis::add(d->machUTCo,
		    i18n("Here, the offset of the connected device's time zone relative to GMT is shown."));
    d->machDST  = makeEntry(i18n("Daylight saving"), gb, 3);
    QWhatsThis::add(d->machDST,
		    i18n("Here, you can see, if daylight saving time is currently active on the connected device."));
    d->g->addRowSpacing(0, KDialog::marginHint());
    d->g->setColStretch(0, 1);
    d->g->setColStretch(1, 1);
    box->addStretch(10);

    d->f = _props->dialog()->addPage(i18n("Psion &Battery"));
    box = new QVBoxLayout(d->f, KDialog::spacingHint());
    gb = new QGroupBox(i18n("Main battery"), d->f, "mbatInfBox");
    box->addWidget(gb);
    d->g = new QGridLayout(gb, 9, 2, KDialog::marginHint(),
			   KDialog::spacingHint());
    d->mbattChanged    = makeEntry(i18n("Changed at:"), gb, 1);
    QWhatsThis::add(d->mbattChanged,
		    i18n("This shows the time of last battery change."));
    d->mbattUsage      = makeEntry(i18n("Usage time:"), gb, 2);
    QWhatsThis::add(d->mbattUsage,
		    i18n("This shows the accumulated time of running on battery power."));
    d->mbattStatus     = makeEntry(i18n("Status:"), gb, 3);
    QWhatsThis::add(d->mbattStatus,
		    i18n("This shows current status of the battery."));
    d->mbattPower      = makeEntry(i18n("Total consumed power:"), gb, 4);
    QWhatsThis::add(d->mbattPower,
		    i18n("This shows accumulated power consumtion of the device."));
    d->mbattCurrent    = makeEntry(i18n("Current:"), gb, 5);
    QWhatsThis::add(d->mbattCurrent,
		    i18n("This shows the current, drawn from power supply (battery or mains)."));
    d->mbattVoltage    = makeEntry(i18n("Voltage:"), gb, 6);
    QWhatsThis::add(d->mbattVoltage,
		    i18n("This shows the current battery voltage."));
    d->mbattMaxVoltage = makeEntry(i18n("Max. voltage:"), gb, 7);
    QWhatsThis::add(d->mbattMaxVoltage,
		    i18n("This shows the maximum battery voltage."));
    d->g->addRowSpacing(0, KDialog::marginHint());
    d->g->setColStretch(0, 1);
    d->g->setColStretch(1, 1);

    gb = new QGroupBox(i18n("Backup battery"), d->f, "bbatInfBox");
    box->addWidget(gb);
    d->g = new QGridLayout(gb, 5, 2, KDialog::marginHint(),
			   KDialog::spacingHint());
    d->bbattStatus     = makeEntry(i18n("Status:"), gb, 1);
    QWhatsThis::add(d->bbattStatus,
		    i18n("This shows current status of the backup battery."));
    d->bbattVoltage    = makeEntry(i18n("Voltage:"), gb, 2);
    QWhatsThis::add(d->bbattVoltage,
		    i18n("This shows the current backup battery voltage."));
    d->bbattMaxVoltage = makeEntry(i18n("Max. voltage:"), gb, 3);
    QWhatsThis::add(d->bbattMaxVoltage,
		    i18n("This shows the maximum backup battery voltage."));
    d->g->addRowSpacing(0, KDialog::marginHint());
    d->g->setColStretch(0, 1);
    d->g->setColStretch(1, 1);

    gb = new QGroupBox(i18n("External power"), d->f, "epowerInfBox");
    box->addWidget(gb);
    d->g = new QGridLayout(gb, 4, 2, KDialog::marginHint(),
			   KDialog::spacingHint());
    d->epowerSupplied   = makeEntry(i18n("Supplied:"), gb, 1);
    QWhatsThis::add(d->epowerSupplied,
		    i18n("This shows whether external power is currently supplied."));
    d->epowerUsage      = makeEntry(i18n("Usage time:"), gb, 2);
    QWhatsThis::add(d->epowerUsage,
		    i18n("This shows the accumulated time of running on external power."));
    d->g->addRowSpacing(0, KDialog::marginHint());
    d->g->setColStretch(0, 1);
    d->g->setColStretch(1, 1);
    box->addStretch(10);

    d->f = _props->dialog()->addPage(i18n("Psion M&emory"));

    box = new QVBoxLayout(d->f, KDialog::spacingHint());
    gb = new QGroupBox(i18n("ROM"), d->f, "romInfBox");
    box->addWidget(gb);
    d->g = new QGridLayout(gb, 5, 2, KDialog::marginHint(),
			   KDialog::spacingHint());
    d->romVersion = makeEntry(i18n("Version:"), gb, 1);
    QWhatsThis::add(d->romVersion,
		    i18n("This shows the firmware version."));
    d->romSize    = makeEntry(i18n("Size:"), gb, 2);
    QWhatsThis::add(d->romSize,
		    i18n("This shows the size of the ROM."));
    d->romProg    = makeEntry(i18n("Programmable:"), gb, 3);
    QWhatsThis::add(d->romProg,
		    i18n("This shows, whether the ROM is flashable or not."));
    d->g->addRowSpacing(0, KDialog::marginHint());
    d->g->setColStretch(0, 1);
    d->g->setColStretch(1, 1);

    gb = new QGroupBox(i18n("RAM"), d->f, "ramInfBox");
    box->addWidget(gb);
    d->g = new QGridLayout(gb, 6, 2, KDialog::marginHint(),
			   KDialog::spacingHint());
    d->ramSize    = makeEntry(i18n("Size:"), gb, 1);
    QWhatsThis::add(d->ramSize,
		    i18n("This shows the total capacity of the RAM."));
    d->ramFree    = makeEntry(i18n("Free:"), gb, 2);
    QWhatsThis::add(d->ramFree,
		    i18n("This shows the free capacity of the RAM."));
    d->ramMaxFree = makeEntry(i18n("Max. free:"), gb, 3);
    QWhatsThis::add(d->ramMaxFree,
		    i18n("This shows the size of the largest free block of the RAM."));
    d->ramDiskSz  = makeEntry(i18n("RAMDisk size:"), gb, 4);
    QWhatsThis::add(d->ramDiskSz,
		    i18n("This shows, how much RAM is currently used for the RAMDisc."));
    d->g->addRowSpacing(0, KDialog::marginHint());
    d->g->setColStretch(0, 1);
    d->g->setColStretch(1, 1);
    box->addStretch(10);

    KURL u("psion:/0:_MachInfo");
    KIO::TransferJob *job = KIO::get(u, false, false);

    connect(job, SIGNAL(result(KIO::Job *)),
	    SLOT(slotJobFinished(KIO::Job *)));
    connect(job, SIGNAL(data(KIO::Job *, const QByteArray &)),
	    SLOT(slotJobData(KIO::Job *, const QByteArray &)));
}

PlpMachinePage::~PlpMachinePage() {
    delete d;
}

bool PlpMachinePage::supports(KFileItemList _items) {
    for (KFileItemListIterator it(_items); it.current(); ++it) {
	KFileItem *fi = it.current();

	if (fi->url().path() != "/")
	    return false;
    }
    return true;
}

void PlpMachinePage::slotJobData(KIO::Job *job, const QByteArray &data) {
    if (data.size() == sizeof(d->mi)) {
	memcpy((char *)&d->mi, data, sizeof(d->mi));

	d->machType->setText(KGlobal::locale()->translate(d->mi.machineType));
	d->machName->setText(QString(d->mi.machineName));
	// ??! None of QString's formatting methods knows about long long.
	ostrstream s;
	s << hex << setw(16) << d->mi.machineUID << '\0';
	d->machUID->setText(QString(s.str()));
	d->machLang->setText(KGlobal::locale()->translate(d->mi.uiLanguage));
	d->dispGeo->setText(QString("%1x%2").arg(
				d->mi.displayWidth).arg(d->mi.displayHeight));
	d->regSize->setText(
	    QString("%1 (%2)").arg(
		KIO::convertSize(d->mi.registrySize)).arg(
		    KGlobal::locale()->formatNumber(d->mi.registrySize, 0)));

	QString rev;
	rev.sprintf("%d.%02d(%d)", d->mi.romMajor, d->mi.romMinor,
		    d->mi.romBuild);
	d->romVersion->setText(rev);
	d->romSize->setText(
	    QString("%1 (%2)").arg(KIO::convertSize(d->mi.romSize)).arg(
		KGlobal::locale()->formatNumber(d->mi.romSize, 0)));
	d->romProg->setText(d->mi.romProgrammable ? i18n("yes") : i18n("no"));

	d->ramSize->setText(
	    QString("%1 (%2)").arg(
		KIO::convertSize(d->mi.ramSize)).arg(
		    KGlobal::locale()->formatNumber(d->mi.ramSize, 0)));
	d->ramFree->setText(
	    QString("%1 (%2)").arg(KIO::convertSize(d->mi.ramFree)).arg(
		KGlobal::locale()->formatNumber(d->mi.ramFree, 0)));
	d->ramMaxFree->setText(
	    QString("%1 (%2)").arg(KIO::convertSize(d->mi.ramMaxFree)).arg(
		KGlobal::locale()->formatNumber(d->mi.ramMaxFree, 0)));
	d->ramDiskSz->setText(
	    QString("%1 (%2)").arg(KIO::convertSize(d->mi.ramDiskSize)).arg(
		KGlobal::locale()->formatNumber(d->mi.ramDiskSize, 0)));


	PsiTime pt(&d->mi.time, &d->mi.tz);
	QDateTime dt;
	dt.setTime_t(pt.getTime());
	d->machTime->setText(KGlobal::locale()->formatDateTime(dt, false));
	d->machUTCo->setText(i18n("%1 seconds").arg(d->mi.tz.utc_offset));
	d->machDST->setText((d->mi.tz.dst_zones & PsiTime::PSI_TZ_HOME)
			    ? i18n("yes") : i18n("no"));

	ostrstream mbs;
	mbs << d->mi.mainBatteryUsedTime << '\0';
	d->mbattUsage->setText(QString(mbs.str()));
	pt.setPsiTime(&d->mi.mainBatteryInsertionTime);
	dt.setTime_t(pt.getTime());
	d->mbattChanged->setText(KGlobal::locale()->formatDateTime(dt, false));
	d->mbattStatus->setText(
	    KGlobal::locale()->translate(d->mi.mainBatteryStatus));
	d->mbattPower->setText(
	    QString("%1 mAs").arg(
		KGlobal::locale()->formatNumber(d->mi.mainBatteryUsedPower, 0)));
	d->mbattCurrent->setText(
	    QString("%1 mA").arg(
		KGlobal::locale()->formatNumber(d->mi.mainBatteryCurrent, 0)));
	d->mbattVoltage->setText(
	    QString("%1 mV").arg(
		KGlobal::locale()->formatNumber(d->mi.mainBatteryVoltage, 0)));
	d->mbattMaxVoltage->setText(
	    QString("%1 mV").arg(KGlobal::locale()->formatNumber(
				     d->mi.mainBatteryMaxVoltage, 0)));

	d->bbattStatus->setText(
	    KGlobal::locale()->translate(d->mi.backupBatteryStatus));
	d->bbattVoltage->setText(
	    QString("%1 mV").arg(
		KGlobal::locale()->formatNumber(d->mi.backupBatteryVoltage, 0)));
	d->bbattMaxVoltage->setText(
	    QString("%1 mV").arg(KGlobal::locale()->formatNumber(
				     d->mi.backupBatteryMaxVoltage, 0)));

	ostrstream bbs;
	bbs << d->mi.externalPowerUsedTime << '\0';
	d->epowerUsage->setText(QString(bbs.str()));
	d->epowerSupplied->setText(
	    d->mi.externalPower ? i18n("Yes") : i18n("No"));
    }
}

void PlpMachinePage::slotJobFinished(KIO::Job *job) {
    KIO::TransferJob *mJob = static_cast<KIO::TransferJob *>(job);

    if (mJob->error())
	job->showErrorDialog(d->props->dialog());
}

class PlpOwnerPage::PlpOwnerPagePrivate
{
public:
    PlpOwnerPagePrivate()	{ }
    ~PlpOwnerPagePrivate() { }

    QFrame *frame;
    KPropertiesDialog *props;
    QMultiLineEdit *owneredit;
};

PlpOwnerPage::PlpOwnerPage( KPropertiesDialog *_props ) {
    d = new PlpOwnerPagePrivate;
    d->props = _props;
    d->frame = _props->dialog()->addPage(i18n("Psion &Owner"));
    QBoxLayout *box = new QVBoxLayout( d->frame, KDialog::spacingHint() );
    d->owneredit = new QMultiLineEdit(d->frame, "ownerinfo");
    d->owneredit->setReadOnly(true);
    box->addWidget(d->owneredit);
    QWhatsThis::add(d->owneredit,
		    i18n("This shows the owner's information of the connected device."));

    KIO_ARGS << int(PLP_CMD_OWNERINFO);
    KIO::StatJob *job = new KIO::StatJob(KURL("psion:/"),
					 KIO::CMD_SPECIAL, packedArgs, false);
    connect(job, SIGNAL(result(KIO::Job *)),
	    SLOT(slotSpecialFinished(KIO::Job *)));

    box->addStretch(10);
}

PlpOwnerPage::~PlpOwnerPage() {
    delete d;
}

bool PlpOwnerPage::supports(KFileItemList _items) {
    for (KFileItemListIterator it(_items); it.current(); ++it) {
	KFileItem *fi = it.current();

	if (fi->url().path() != "/")
	    return false;
    }
    return true;
}

void PlpOwnerPage::slotSpecialFinished(KIO::Job *job) {
    KIO::StatJob *sJob = static_cast<KIO::StatJob *>(job);

    if (sJob->error())
	job->showErrorDialog(d->props->dialog());
    else {
	KIO::UDSEntry e = sJob->statResult();
	for (KIO::UDSEntry::ConstIterator it = e.begin();
	     it != e.end(); ++it) {
	    if ((*it).m_uds == KIO::UDS_NAME)
		d->owneredit->setText((*it).m_str);
	}
    }
}

#include "plpprops.moc"
/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
