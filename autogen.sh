# Set up for compilation

cp Makefile.am.in Makefile.am
mkdir -p intl
echo n | gettextize --force --intl --no-changelog
test -e po/Makevars || cp po/Makevars.template po/Makevars
rm -f *~
-test -f po/ChangeLog~ && mv -f po/ChangeLog~ po/ChangeLog
autoreconf -i
