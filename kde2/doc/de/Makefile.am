
KDE_LANG = de
KDE_DOCS = kpsion

EXTRA_DIST = index2.docbook.in index3.docbook.in

index.docbook: index.docbook.in
	cat index.docbook.in | perl -e 'while (<>) { if (/\$$Date:\s+(.*)\s+\$$/) { $$d=$$1; } s/__DATE__/$$d/; s/__VERSION__/$(VERSION)/; print; }' > index.docbook
