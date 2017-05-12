pkglib_LTLIBRARIES = ddb_misc_headerbar_GTK3.la

EXTRA_DIST = headerbar.ui

ACLOCAL_AMFLAGS= -I m4
AUTOMAKE_OPTIONS = foreign

BUILT_SOURCES=resources.c resources.h

resource_files = $(shell $(GLIB_COMPILE_RESOURCES) --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/headerbarui.gresource.xml)

resources.h:
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-header $(srcdir)/headerbarui.gresource.xml

resources.c: headerbarui.gresource.xml $(resource_files) resources.h
	$(AM_V_GEN) $(GLIB_COMPILE_RESOURCES) --target=$@ --sourcedir=$(srcdir) --generate-source $(srcdir)/headerbarui.gresource.xml

ddb_misc_headerbar_GTK3_la_SOURCES=resources.c headerbarui.c headerbarui.h resources.h headerbarui.gresource.xml
ddb_misc_headerbar_GTK3_la_CFLAGS=${GTK_DEPS_CFLAGS}
ddb_misc_headerbar_GTK3_la_LDFLAGS=-module -avoid-version -shared
ddb_misc_headerbar_GTK3_la_LIBADD=${GTK_DEPS_LIBS}

localinstall:
	install .libs/ddb_misc_headerbar_GTK3.so ~/.local/lib64/deadbeef
