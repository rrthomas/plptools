#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "plppropsFactory.h"
#include "plpprops.h"

#include <iostream>
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
    KGlobal::locale()->insertCatalogue(QString::fromLatin1("plptools"));
    KGlobal::locale()->insertCatalogue(QString::fromLatin1("libplpprops"));
}

plppropsFactory::~plppropsFactory() {
    delete s_global;
}

QObject* plppropsFactory::createObject(QObject* parent, const char *name, const char *classname, const QStringList & ) {

    QObject *obj = 0L;

    if ((strcmp(classname, "KPropsDlgPlugin") == 0) && parent &&
	parent->inherits("KPropertiesDialog"))
	obj = new PlpPropsPlugin(static_cast<KPropertiesDialog *>(parent));
    return obj;
}

#include <plppropsFactory.moc>
