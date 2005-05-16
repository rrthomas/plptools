AC_DEFUN([KDE_PRINT_QT_PROGRAM],[
	AC_REQUIRE([KDE_USE_QT])
	cat > conftest.$ac_ext <<-EOF
		#include "confdefs.h"
		#include <qglobal.h>
		#include <qapplication.h>
	EOF
	if test "$kde_qtver" = "2"; then
		cat >> conftest.$ac_ext <<-EOF
			#include <qstyle.h>
			#include <qiconview.h>
		EOF
	fi

	echo "#if ! ($kde_qt_verstring)" >> conftest.$ac_ext
	cat >> conftest.$ac_ext <<-EOF
		#error 1
		#endif

		int main() {
	EOF
	if test "$kde_qtver" = "2"; then
		cat >> conftest.$ac_ext <<-EOF
			QStringList *t = new QStringList();
			QIconView iv(0);
			iv.setWordWrapIconText(false);
		EOF
	fi
	cat >> conftest.$ac_ext <<-EOF
		return 0;
		}
	EOF
])
