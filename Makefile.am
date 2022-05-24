

SUBDIRS = po example inc src

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = m4/ChangeLog

translations:
	xgettext --keyword=_ --language=C --add-comments --sort-output -o po/skarabeusz.pot src/skarabeusz.cc

polish-po:
	msginit --input=po/skarabeusz.pot --locale=pl --output=po/pl/skarabeusz.po

polish-update:
	msgmerge --update po/pl/skarabeusz.po po/skarabeusz.pot	

polish-mo:
	msgfmt --output-file=po/pl/skarabeusz.mo po/pl/skarabeusz.po
	
