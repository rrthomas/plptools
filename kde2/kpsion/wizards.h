#ifndef _WIZARDS_H_
#define _WIZARDS_H_

#include "kpsion.h"

#include <kwizard.h>
#include <knuminput.h>
#include <kcombobox.h>
#include <klineedit.h>
#include <klistview.h>

#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qlabel.h>

class FirstTimeWizard : public KWizard {
	Q_OBJECT
 public:
	FirstTimeWizard(QWidget *parent = 0, const char *name = 0);

 protected:
	virtual void closeEvent(QCloseEvent *e);
	virtual void reject();
	virtual void accept();

 protected slots:
	virtual void next();

 private slots:
	void slotBdirBrowse();

 private:
	bool checkBackupDir(QString &);

	QWidget *page1;
	QWidget *page2;
	QWidget *page3;
	QWidget *page4;
	QWidget *page5;
	QLabel  *bdirLabel;
	KIntSpinBox *genSpin;
	KIntSpinBox *rconSpin;
	QPushButton *bdirButton;
	QCheckBox *remCheck;
	KComboBox *iIntCombo;
	KComboBox *fIntCombo;
	KComboBox *devCombo;
	KComboBox *speedCombo;

	QString bdirDefault;
	QString bdirCreated;
};

class NewPsionWizard : public KWizard {
	Q_OBJECT

 public:
	NewPsionWizard(QWidget *parent = 0, const char *name = 0);

 protected:
	virtual void accept();

 protected slots:
	virtual void next();

 private:
	bool checkPsionName(QString &);

	QWidget *page1;
	QWidget *page2;
	KPsionMainWindow *psion;
	KLineEdit *nameEdit;
	KListView *backupListView;

	QString uid;
	QString machineName;
};
#endif
