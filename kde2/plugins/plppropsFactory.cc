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
	//KGlobal::locale()->insertCatalogue("plpprops");
}

plppropsFactory::~plppropsFactory() {
	delete s_global;
}

QObject* plppropsFactory::createObject(QObject* parent, const char *name, const char *classname, const QStringList & ) {

	QObject *obj = 0L;

	cout << "plppropsFactory: name=" << name << " class=" << classname << endl;
	if ((strcmp(classname, "KPropsDlgPlugin") == 0) &&
	    parent &&
	    parent->inherits("KPropertiesDialog"))
		obj = new PlpPropsPlugin(static_cast<KPropertiesDialog *>(parent));
	return obj;
}

#include <plppropsFactory.moc>
