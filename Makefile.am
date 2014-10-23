pkglib_LTLIBRARIES = \
	watch-gettext.la

bin_SCRIPTS = \
	watch-gettext

$(bin_SCRIPTS): Makefile
	for SCRIPT in $(bin_SCRIPTS) ; do \
		echo -e "#!/bin/sh\nrm -f watch-gettext.po\nLD_PRELOAD=$(pkglibdir)/$$SCRIPT.so \"\$$@\"" >$$SCRIPT ; \
	done

DISTCLEANFILES = $(bin_SCRIPTS)

watch_gettext_la_LIBADD = -ldl $(GLIB_LIBS)
watch_gettext_la_CFLAGS = $(GLIB_CFLAGS)
watch_gettext_la_LDFLAGS = -avoid-version -module -no-undefined

ACLOCAL_AMFLAGS = -I m4
