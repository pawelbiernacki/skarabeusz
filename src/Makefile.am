

bin_PROGRAMS=skarabeusz

skarabeusz_SOURCES=skarabeusz.cc


AM_CXXFLAGS= -fconcepts -std=c++17 -I.. -I../inc -DLOCALEDIR='"$(localedir)"' @CXXFLAGS@ @GD_CFLAGS@ -DDATADIR=\"$(datadir)\"

skarabeusz_LDADD = @GD_LIBS@

skarabeuszdir = $(datadir)/skarabeusz

skarabeusz_DATA = \
	../image/*.png
	
