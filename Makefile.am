AUTOMAKE_OPTIONS = foreign
ACLOCAL_AMFLAGS = -I m4
SOURCEDIR=@top_srcdir@/src
EXTRA_DIST = README.md LICENSE $(SOURCEDIR)/headerbar.ui

pkglib_LTLIBRARIES = ddb_misc_headerbar_GTK3.la

built_sources = resources.c resources.h
DISTCLEANFILES = $(built_sources)

resource_files = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(SOURCEDIR) --generate-dependencies $(SOURCEDIR)/headerbarui.gresource.xml)

resources.h: src/headerbarui.gresource.xml
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(SOURCEDIR) --generate-header $(SOURCEDIR)/headerbarui.gresource.xml

resources.c: src/headerbarui.gresource.xml $(resource_files)
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(SOURCEDIR) --generate-source $(SOURCEDIR)/headerbarui.gresource.xml

ddb_misc_headerbar_GTK3_la_SOURCES=src/resources.c src/resources.h src/headerbarui.c src/headerbarui.h src/headerbarui.gresource.xml
ddb_misc_headerbar_GTK3_la_CFLAGS=${GTK_DEPS_CFLAGS}
ddb_misc_headerbar_GTK3_la_LDFLAGS=${GTK_DEPS_LIBS} -module -avoid-version -shared

localinstall:
	install .libs/ddb_misc_headerbar_GTK3.so ~/.local/lib64/deadbeef

realclean: distclean
	@rm -fr *~ autom4te.cache/ m4/ config.h.in configure config.sub ltmain.sh
	@rm -f aclocal.m4 install-sh missing depcomp config.guess
	@find $(top_srcdir) -name Makefile.in -exec rm -f {} \;