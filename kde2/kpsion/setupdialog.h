#ifndef _SETUPDIALOGS_H_
#define _SETUPDIALOGS_H_

#include <rfsv.h>
#include <rpcs.h>

#include <kdialogbase.h>

class SetupDialog : public KDialogBase {
 public:
	SetupDialog(QWidget *parent, rfsv *plpRfsv, rpcs *plpRpcs);

 private slots:
	void slotDefaultClicked();
};

#endif
