

SUBDIRS = po image example inc src

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = m4/ChangeLog

translations:
	xgettext --keyword=_ --language=C --add-comments --sort-output -o po/skarabeusz.pot src/skarabeusz.cc

polish-po:
	msginit --input=po/skarabeusz.pot --locale=pl --output=po/pl/skarabeusz.po

polish-update:
	msgmerge --update po/pl/skarabeusz.po po/skarabeusz.pot	

polish-mo:
	msgfmt --output-file=po/pl/skarabeusz.mo po/pl/skarabeusz.po; \
	cp po/pl/skarabeusz.po po/pl.po

russian-po:
	msginit --input=po/skarabeusz.pot --locale=ru --output=po/ru/skarabeusz.po

russian-update:
	msgmerge --update po/ru/skarabeusz.po po/skarabeusz.pot	

russian-mo:
	msgfmt --output-file=po/ru/skarabeusz.mo po/ru/skarabeusz.po; \
	cp po/ru/skarabeusz.po po/ru.po
	
	
	
finnish-po:
	msginit --input=po/skarabeusz.pot --locale=fi --output=po/fi/skarabeusz.po

finnish-update:
	msgmerge --update po/fi/skarabeusz.po po/skarabeusz.pot	

finnish-mo:
	msgfmt --output-file=po/fi/skarabeusz.mo po/fi/skarabeusz.po; \
	cp po/fi/skarabeusz.po po/fi.po
	
	
	
locale_pldir = ${localedir}/pl/LC_MESSAGES
locale_rudir = ${localedir}/ru/LC_MESSAGES
locale_fidir = ${localedir}/fi/LC_MESSAGES

locale_pl_DATA = po/pl/skarabeusz.mo 
locale_ru_DATA = po/ru/skarabeusz.mo
locale_fi_DATA = po/fi/skarabeusz.mo
