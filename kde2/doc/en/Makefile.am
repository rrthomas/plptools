
KDE_LANG = en
KDE_DOCS = kpsion

EXTRA_DIST = index.docbook.in

index.docbook: index.docbook.in
	cat index.docbook.in | perl -e 'while (<>) { if (/\$$Date:\s+(.*)\s+\$$/) { $$d=$$1; } s/__DATE__/$$d/; s/__VERSION__/$(VERSION)/; print; }' > index.docbook

maintainer-clean-local:
	rm -f Makefile.in
