/* $Id$
 *
 * This file holds the definitions for all classes used to
 * display a Psion related properties dialog.
 */

#ifndef _PLPPROPS_H_
#define _PLPPROPS_H_

#include <qstring.h>
#include <qlist.h>
#include <qgroupbox.h>

#include <kurl.h>
#include <kfileitem.h>
#include <kdialogbase.h>
#include <kpropsdlg.h>

#include "pie3dwidget.h"

namespace KIO { class Job; }

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

	/**
	 * Called after all plugins applied their changes
	 */
	void postApplyChanges();

 private:
	class PlpPropsPluginPrivate;
	PlpPropsPluginPrivate *d;
};

class PlpFileAttrPage : public KPropsDlgPlugin {
	Q_OBJECT
 public:
	/**
	 * Constructor
	 */
	PlpFileAttrPage(KPropertiesDialog *_props);
	virtual ~PlpFileAttrPage();

	virtual void applyChanges();

	static bool supports(KFileItemList _items);

 private:
	class PlpFileAttrPagePrivate;
	PlpFileAttrPagePrivate *d;
};

class PlpDriveAttrPage : public KPropsDlgPlugin {
	Q_OBJECT
 public:
	/**
	 * Constructor
	 */
	PlpDriveAttrPage(KPropertiesDialog *_props);
	virtual ~PlpDriveAttrPage();

	virtual void applyChanges();

	static bool supports(KFileItemList _items);

 private slots:
	void slotSpecialFinished(KIO::Job *job);

 private:
	class PlpDriveAttrPagePrivate;
	PlpDriveAttrPagePrivate *d;

	unsigned long total;
	unsigned long unused;

	QGroupBox   *gb;
	QLabel      *uidLabel;
	QLabel      *typeLabel;
	QLabel      *totalLabel;
	QLabel      *freeLabel;
	QColor      usedColor;
	QColor      freeColor;
	Pie3DWidget *pie;
};


/**
 * Used to view/edit machine info.
 */
class PlpMachinePage : public KPropsDlgPlugin {
	Q_OBJECT
 public:
	/**
	 * Constructor
	 */
	PlpMachinePage(KPropertiesDialog *_props);
	virtual ~PlpMachinePage();

	virtual void applyChanges();

	static bool supports(KFileItemList _items);

 private:
	class PlpMachinePagePrivate;
	PlpMachinePagePrivate *d;
};

/**
 * Used to view/edit owner info
 */
class PlpOwnerPage : public KPropsDlgPlugin {
	Q_OBJECT
 public:
	/**
	 * Constructor
	 */
	PlpOwnerPage(KPropertiesDialog *_props);
	virtual ~PlpOwnerPage();

	virtual void applyChanges();

	static bool supports(KFileItemList _items);

 private:
	class PlpOwnerPagePrivate;
	PlpOwnerPagePrivate *d;
};

#endif
