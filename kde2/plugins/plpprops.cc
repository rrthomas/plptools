#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stream.h>
#include <errno.h>
#include <assert.h>

#include <qfile.h>
#include <qapplication.h>
#include <qdir.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qstrlist.h>
#include <qstringlist.h>
#include <qtextstream.h>
#include <qpainter.h>
#include <qlayout.h>
#include <qcombobox.h>
#include <qgroupbox.h>

#include <kdialog.h>
#include <kdirsize.h>
#include <kdirwatch.h>
#include <kdebug.h>
#include <kdesktopfile.h>
#include <kicondialog.h>
#include <kurl.h>
#include <kurlrequester.h>
#include <klocale.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kstddirs.h>
#include <kio/job.h>
#include <kio/renamedlg.h>
#include <kfiledialog.h>
#include <kmimetype.h>
#include <kmessagebox.h>
#include <kservice.h>
#include <kglobal.h>
#include <kcompletion.h>
#include <klineedit.h>
//#include <klibloader.h>
//#include <ktrader.h>
#include <kio/slaveinterface.h>

#include "plpprops.h"
#include "pie3dwidget.h"

#include <qobjectlist.h>
#include <qtabwidget.h>

#define KIO_ARGS QByteArray packedArgs; \
QDataStream stream( packedArgs, IO_WriteOnly ); stream

class PlpPropsPlugin::PlpPropsPluginPrivate {
public:
	PlpPropsPluginPrivate() { }
	~PlpPropsPluginPrivate() { }

	QFrame *m_frame;
};

/*
 * A VERY UGLY HACK for removing the Permissions-Page from
 * the Properties dialog.
 */
static void
removePermsPage(QWidget *theDialog) {
	QObject *qtabwidget = 0L;
	QFrame *permframe = 0L;

	//
	// First, find the QTabWidget in the dialog.
	// This is easy, cause there's only one.
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
	// inherited from class QFrame.
	//
	if (qtabwidget != 0L) {
		l = qtabwidget->queryList("QFrame");
		QObjectListIt it(*l);

		while ((obj = it.current()) != 0) {
			QObjectList *l2 = obj->queryList();
			QObjectListIt it2(*l2);
			QObject *o2;
			int qvbl, qgb, qgl, ql, qcb;
			qvbl = qgb = qgl = ql = qcb = 0;

			// If we found a QFrame, count it's children
			// by className. We must rely on the numbers,
			// because not a single child has been given
			// a name.
			while ((o2 = it2.current()) != 0) {
				if (o2->isA("QVBoxLayout"))
					qvbl++;
				if (o2->isA("QGroupBox"))
					qgb++;
				if (o2->isA("QGridLayout"))
					qgl++;
				if (o2->isA("QLabel"))
					ql++;
				if (o2->isA("QCheckBox"))
					qcb++;
				++it2;
			}
			delete l2;

			// The PermissionsPage is build out of:
			//
			// 1 QVBoxLayout, 2 QGroupboxes, 2 QGridLayouts,
			// 15 QLabels and 12 QCheckBoxes
			//
			if ((qvbl == 1) &&
			    (qgb == 2) &&
			    (qgl == 2) &&
			    (ql == 15) &&
			    (qcb == 12)) {
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
	
	if (!supports(properties->items()))
		return;

	if (PlpFileAttrPage::supports(properties->items())) {
		PlpFileAttrPage *p = new PlpFileAttrPage(_props);
		removePerms = true;
	}
	if (PlpDriveAttrPage::supports(properties->items())) {
		PlpDriveAttrPage *p = new PlpDriveAttrPage(_props);
		removePerms = true;
	}
	if (PlpMachinePage::supports(properties->items())) {
		PlpMachinePage *p = new PlpMachinePage(_props);
	}
	if (PlpOwnerPage::supports(properties->items())) {
		PlpOwnerPage *p = new PlpOwnerPage(_props);
	}
	if (removePerms)
		removePermsPage(properties->dialog());
}

PlpPropsPlugin::~PlpPropsPlugin() {
	delete d;
}

bool PlpPropsPlugin::supports(KFileItemList _items) {
	for (KFileItemListIterator it(_items); it.current(); ++it) {
		KFileItem *fi = it.current();

		if (fi->url().protocol() != QString::fromLatin1("psion"))
			return false;
	}
	return true;
}

void PlpPropsPlugin::applyChanges() {
	kdDebug(250) << "PlpFileAttrPlugin::applyChanges" << endl;
}

void PlpPropsPlugin::postApplyChanges() {
}

class PlpFileAttrPage::PlpFileAttrPagePrivate {
public:
	PlpFileAttrPagePrivate() { }
	~PlpFileAttrPagePrivate() { }

	QFrame *m_frame;
};

PlpFileAttrPage::PlpFileAttrPage(KPropertiesDialog *_props)
	: KPropsDlgPlugin( _props ) {
	d = new PlpFileAttrPagePrivate;
	d->m_frame = properties->dialog()->addPage(i18n("Psion &Attributes"));
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
}

class PlpDriveAttrPage::PlpDriveAttrPagePrivate {
public:
	PlpDriveAttrPagePrivate() { }
	~PlpDriveAttrPagePrivate() { }

	QFrame *m_frame;
};

PlpDriveAttrPage::PlpDriveAttrPage(KPropertiesDialog *_props)
	: KPropsDlgPlugin( _props ) {

	d = new PlpDriveAttrPagePrivate;
	d->m_frame = properties->dialog()->addPage(i18n("Psion &Drive"));

	QBoxLayout *box = new QVBoxLayout( d->m_frame, KDialog::spacingHint() );
	QLabel *l;
	QGridLayout *gl;

	KIO_ARGS << int(1) << properties->item()->name();
	KIO::StatJob *job = new KIO::StatJob(KURL("psion:/"), KIO::CMD_SPECIAL, packedArgs, false);
	connect(job, SIGNAL(result(KIO::Job *)), SLOT(slotSpecialFinished(KIO::Job *)));


	long total = 33267;
	long free = 12345;

	gb = new QGroupBox(i18n("Information"), d->m_frame);
	box->addWidget(gb);

	gl = new QGridLayout(gb, 7, 4, 15);
	gl->addRowSpacing(0, 10);

	l = new QLabel(i18n("Type"), gb);
	gl->addWidget(l, 1, 0);

	typeLabel = new QLabel(gb);
	gl->addWidget(typeLabel, 2, 0);

	l = new QLabel(i18n("Total capacity"), gb);
	gl->addWidget (l, 1, 1);

	totalLabel = new QLabel(gb);
	gl->addWidget(totalLabel, 2, 1);

	l = new QLabel(i18n("Free space"), gb);
	gl->addWidget (l, 1, 2);

	freeLabel = new QLabel(gb);
	gl->addWidget(freeLabel, 2, 2);

	l = new QLabel(i18n("Unique ID"), gb);
	gl->addWidget (l, 1, 3);

	uidLabel = new QLabel(gb);
	gl->addWidget(uidLabel, 2, 3);

	pie = new Pie3DWidget(gb, "pie");

	gl->addMultiCellWidget(pie, 3, 4, 1, 2);

	l = new QLabel(i18n("Used space"), gb);
	gl->addWidget (l, 5, 2);
	
	l = new QLabel(i18n(" "), gb);
	l->setBackgroundColor(QColor(219, 58, 197));
	gl->addWidget (l, 5, 3);

	l = new QLabel(i18n("Free space"), gb);
	gl->addWidget (l, 6, 2);

	l = new QLabel(i18n(" "), gb);
	l->setBackgroundColor(QColor(39, 56, 167));

	gl->addWidget (l, 6, 3);

	box->addStretch(10);
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
	}
	return true;
}

void PlpDriveAttrPage::applyChanges() {
}

void PlpDriveAttrPage::slotSpecialFinished(KIO::Job *job) {
	KIO::StatJob *sJob = static_cast<KIO::StatJob *>(job);

	if (sJob->error())
		job->showErrorDialog(properties->dialog());
	else {
		KIO::UDSEntry e = sJob->statResult();
		bool total_found = false;
		bool free_found = false;

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
				uidLabel->setText(QString::fromLatin1("%1").arg(uid, 8, 16));
			}
			if ((*it).m_uds == KIO::UDS_NAME) {
				QString name = ((*it).m_str);
				typeLabel->setText(name);
			}
			if ((*it).m_uds == KIO::UDS_USER) {
				QString name = ((*it).m_str);
				gb->setTitle(QString(i18n("Information for Psion drive %1: (%2)")).arg(name).arg(properties->item()->name()));
			}
		}
		if (total_found && free_found) {
			totalLabel->setText(QString::fromLatin1("%1 (%2)").arg(KIO::convertSize(total)).arg(KGlobal::locale()->formatNumber(total, 0)));
			freeLabel->setText(QString::fromLatin1("%1 (%2)").arg(KIO::convertSize(unused)).arg(KGlobal::locale()->formatNumber(unused, 0)));
			pie->addPiece(total - unused, QColor(219, 58, 197));
			pie->addPiece(unused, QColor(39, 56, 167));
		}
	}
}

class PlpMachinePage::PlpMachinePagePrivate {
public:
	PlpMachinePagePrivate() { }
	~PlpMachinePagePrivate() { }

	QFrame *m_frame;
};

PlpMachinePage::PlpMachinePage( KPropertiesDialog *_props )
	: KPropsDlgPlugin( _props ) {

	d = new PlpMachinePagePrivate;
	d->m_frame = properties->dialog()->addPage(i18n("Psion &Machine"));

	QVBoxLayout * mainlayout = new QVBoxLayout( d->m_frame, KDialog::spacingHint());

	// Now the widgets in the top layout

	QLabel* l;
	l = new QLabel(d->m_frame, "Label_1" );
	l->setText( i18n("Machine UID:") );
	mainlayout->addWidget(l, 1);

	mainlayout->addStretch(2);
}

PlpMachinePage::~PlpMachinePage() {
	delete d;
}

bool PlpMachinePage::supports(KFileItemList _items) {
	for (KFileItemListIterator it(_items); it.current(); ++it) {
		KFileItem *fi = it.current();

		QString path = fi->url().path(-1);
		if (path.contains('/') != 1)
			return false;
		if (fi->mimetype() != QString::fromLatin1("application/x-psion-machine"))
			return false;
	}
	return true;
}

void PlpMachinePage::applyChanges() {
}


class PlpOwnerPage::PlpOwnerPagePrivate
{
public:
	PlpOwnerPagePrivate()	{ }
	~PlpOwnerPagePrivate() { }

	QFrame *m_frame;
};

PlpOwnerPage::PlpOwnerPage( KPropertiesDialog *_props ) : KPropsDlgPlugin( _props ) {
	d = new PlpOwnerPagePrivate;
	d->m_frame = properties->dialog()->addPage(i18n("Psion &Owner"));
}

PlpOwnerPage::~PlpOwnerPage() {
	delete d;
}

bool PlpOwnerPage::supports(KFileItemList _items) {
	for (KFileItemListIterator it(_items); it.current(); ++it) {
		KFileItem *fi = it.current();

		QString path = fi->url().path(-1);
		if (path.contains('/') != 1)
			return false;
		if (fi->mimetype() != QString::fromLatin1("application/x-psion-owner"))
			return false;
	}
	return true;
}

void PlpOwnerPage::applyChanges() {
}

#include "plpprops.moc"
