#include "setupdialog.h"

#include <kapp.h>
#include <kconfig.h>
#include <klocale.h>
#include <knuminput.h>
#include <klineedit.h>
#include <kcombobox.h>

#include <qlayout.h>
#include <qlabel.h>

SetupDialog::SetupDialog(QWidget *parent, rfsv *plpRfsv, rpcs *plpRpcs)
	: KDialogBase(Tabbed, "Settings", Ok|Apply|Default|Cancel, Ok, parent,
		      "settingsDialog", true, true) {

	enableLinkedHelp(true);

	KConfig *config = kapp->config();
	QFrame *page = addPage(i18n("&General"));
	QGridLayout *gl = new QGridLayout(page, 4, 2, 15);
	gl->addRowSpacing(0, 10);
	QLabel *l;

	l = new QLabel(i18n("Backup &directory"), page, "backupDirLabel");
	gl->addWidget(l, 1, 0);
	KLineEdit *bdiredit = new KLineEdit(page, "backupDirEdit");
	gl->addWidget(bdiredit, 1 , 1);
	l->setBuddy(bdiredit);
	QPushButton *bdirbutton = new QPushButton(i18n("Browse"), page, "backupDirButton");
	gl->addWidget(bdirbutton, 1 , 2);

	l = new QLabel(i18n("Backup &generations"), page, "backupGenLabel");
	gl->addMultiCellWidget(l, 2, 2, 0, 1);
	KIntSpinBox *genspin = new KIntSpinBox(0, 10, 1, 3, 10, page, "backupGenSpin");
	gl->addWidget(genspin, 2, 2);
	l->setBuddy(genspin);

	page = addPage(i18n("&Machines"));
	gl = new QGridLayout(page, 4, 2, 15);
	gl->addRowSpacing(0, 10);

	l = new QLabel(i18n("Machine &Name"), page, "NameLabel");
	gl->addWidget(l, 1, 0);
	KLineEdit *nedit = new KLineEdit(page, "NameEdit");
	gl->addWidget(nedit, 1, 1);
	l->setBuddy(nedit);
	l = new QLabel(i18n("Machine &UID"), page, "UIDLabel");
	gl->addWidget(l, 2, 0);
	KComboBox *uidcombo = new KComboBox(true, page, "UIDCombo");
	config->setGroup("Psion");
	uidcombo->insertStringList(config->readListEntry("MachineUIDs"));
	gl->addWidget(uidcombo, 1, 1);
	l->setBuddy(uidcombo);

	connect(this, SIGNAL(defaultClicked()), SLOT(slotDefaultClicked()));
}

void SetupDialog::
slotDefaultClicked() {
	enableLinkedHelp(false);
}
