#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef ENABLE_NLS
namespace LIBPLP {
extern "C" {
#include <plpintl.h>
    void init_libplp_i18n() {
	setlocale(LC_ALL, "");
	textdomain(PACKAGE);
    }
};
};
#endif

#include "plppropsFactory.h"
#include "plpprops.h"

#include <stream.h>
#include <kdebug.h>
#include <klocale.h>

extern "C" {
	void *init_libplpprops() {
		return new plppropsFactory();
	}
};

plppropsFactory::plppropsFactory(QObject *parent, const char *name)
	: KLibFactory(parent, name) {
	s_global = new KInstance("plpprops");
	// Install the translations
#ifdef ENABLE_NLS
	LIBPLP::init_libplp_i18n();
#endif
	KGlobal::locale()->insertCatalogue(QString::fromLatin1("plptools"));
	KGlobal::locale()->insertCatalogue(QString::fromLatin1("libplpprops"));
}

plppropsFactory::~plppropsFactory() {
	delete s_global;
}

QObject* plppropsFactory::createObject(QObject* parent, const char *name, const char *classname, const QStringList & ) {

	QObject *obj = 0L;

	if ((strcmp(classname, "KPropsDlgPlugin") == 0) &&
	    parent &&
	    parent->inherits("KPropertiesDialog"))
		obj = new PlpPropsPlugin(static_cast<KPropertiesDialog *>(parent));
	return obj;
}

#include <plppropsFactory.moc>
