#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "kpsion.h"
#include "wizards.h"
#include <kapp.h>
#include <klocale.h>
#include <kconfig.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

static KCmdLineOptions options[] = {
	{"b", 0, 0},
	{"backup", I18N_NOOP("perform backup"), 0},
	{"r", 0, 0},
	{"restore", I18N_NOOP("perform restore"), 0},
	{"f", 0, 0},
	{"format", I18N_NOOP("format drive"), 0},
	{"+[DriveLetter]", I18N_NOOP("The drive letter to backup/restore or format."), 0},
	{ 0, 0, 0},
};

int main(int argc, char **argv) {
	KAboutData *about = new KAboutData("kpsion", I18N_NOOP("KPsion"),
					   VERSION, I18N_NOOP("Psion connectivity utility"),
					   KAboutData::License_GPL,
					   "(C) 2001, Fritz Elfert", 0L,
					   "http://plptools.sourceforge.net",
					   "plptools-developers@sourceforge.net");
	about->addAuthor("Fritz Elfert", I18N_NOOP("Original Developer/Maintainer"),
			 "felfert@users.sourceforge.net",
			 "http://plptools.sourceforge.net");
	KCmdLineArgs::init(argc, argv, about);
	KCmdLineArgs::addCmdLineOptions(options);

	KApplication a;

	KConfig *config = kapp->config();
	config->setGroup("Settings");
	QString backupDir = config->readEntry("BackupDir");
	
	if (backupDir.isEmpty()) {
		FirstTimeWizard *wiz = new FirstTimeWizard(0L, "firsttimewizard");
		wiz->exec();
	}

	KPsionMainWindow *w = new KPsionMainWindow();
	w->resize(300, 150);
	a.setMainWidget(w);
	w->show();
	return a.exec();
}
					   
					   
