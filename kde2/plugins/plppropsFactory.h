#ifndef _PLPPROPSFACTORY_H_
#define _PLPPROPSFACTORY_H_

#include <klibloader.h>

class plppropsFactory : public KLibFactory {
	Q_OBJECT
 public:
	plppropsFactory(QObject *parent = 0, const char *name = 0);
	virtual ~plppropsFactory();

	virtual QObject* createObject(QObject* parent = 0, const char* name = 0, const char* classname = "QObject", const QStringList &args = QStringList());

 private:
	KInstance *s_global;

};

#endif
