ACLOCAL_AMFLAGS = -I m4

AUTOMAKE_OPTIONS = foreign
EXTRA_DIST = INSTALL LICENSE

SUBDIRS = src

realclean: distclean
	@rm -fr *~ autom4te.cache/ m4/ config.h.in configure config.sub ltmain.sh
	@rm -f aclocal.m4 install-sh missing depcomp config.guess
	@find $(top_srcdir) -name Makefile.in -exec rm -f {} \;