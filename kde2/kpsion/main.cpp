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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "kpsion.h"
#include "kpsionconfig.h"
#include "wizards.h"
#include <kapp.h>
#include <klocale.h>
#include <kconfig.h>
#include <kcmdlineargs.h>
#include <kaboutdata.h>

namespace LIBPLP {
extern "C" {
#include <intl.h>
	void init_libplp_i18n() {
#ifdef HAVE_BINDTEXTDOMAIN_CODESET
		bind_textdomain_codeset(PACKAGE, "latin1");
#endif
		textdomain(PACKAGE);
	}
};
};


static KCmdLineOptions options[] = {
//    {"a", 0, 0},
    {"autobackup", I18N_NOOP("perform scheduled backup"), 0},
//    {"b <drv>", 0, 0},
    {"backup <drv>", I18N_NOOP("perform backup"), 0},
//    {"r <drv>", 0, 0},
    {"restore <drv>", I18N_NOOP("perform restore"), 0},
//    {"f <drv>", 0, 0},
    {"format <drv>", I18N_NOOP("format drive"), 0},
    { 0, 0, 0},
};

int main(int argc, char **argv) {
    KAboutData *about = new KAboutData("kpsion", I18N_NOOP("KPsion"),
				       VERSION,
				       I18N_NOOP("Psion connectivity utility"),
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

    // Install additional translations
    LIBPLP::init_libplp_i18n();
    KGlobal::locale()->insertCatalogue(QString::fromLatin1("plptools"));

    KConfig *config = kapp->config();
    KPsionConfig pcfg;

    config->setGroup(pcfg.getSectionName(KPsionConfig::OPT_BACKUPDIR));
    QString backupDir = config->readEntry(
	pcfg.getOptionName(KPsionConfig::OPT_BACKUPDIR));

    if (backupDir.isEmpty()) {
	FirstTimeWizard *wiz = new FirstTimeWizard(0L, "firsttimewizard");
	wiz->exec();
    }

    KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

    int acnt = 0;
    if (args->isSet("backup"))
	acnt++;
    if (args->isSet("restore"))
	acnt++;
    if (args->isSet("format"))
	acnt++;
    if (args->isSet("autobackup"))
	acnt++;

    if (acnt > 1)
	KCmdLineArgs::usage(i18n(
	    "The options are mutually exclusive. "
	    "I.e. You cannot specify more than one action at once."));

    KPsionMainWindow *w = new KPsionMainWindow();

    if (args->isSet("autobackup") && (!w->isConnected()))
	return 0;
    w->resize(300, 170);
    a.setMainWidget(w);
    w->show();
    return a.exec();
}

/*
 * Local variables:
 * c-basic-offset: 4
 * End:
 */
