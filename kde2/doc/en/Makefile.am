
KDE_LANG = en
KDE_DOCS = kpsion

index.docbook: index.docbook.in
	cat index.docbook.in | perl -e 'while (<>) { if (/\$$Date:\s+(\S*)/) { $$d=$$1; } s/__DATE__/$$d/; s/__VERSION__/$(VERSION)/; print; }' > index.docbook
