
MAINTAINERCLEANFILES = Makefile.in

AM_CPPFLAGS = \
-I. \
-I$(top_builddir) \
-I$(top_srcdir) \
-I$(top_srcdir)/src/lib \
-I$(top_builddir)/src/lib \
-DPACKAGE_DATA_DIR=\"$(datadir)/$(PACKAGE)\" \
-DPACKAGE_LIB_DIR=\"$(libdir)\" \
`pkg-config --cflags mm-sound mm-player vconf devman devman_haptic` \
@EVIL_CFLAGS@ \
@EDJE_CFLAGS@ \
@EFL_EDJE_BUILD@


pkgdir = $(libdir)/edje/modules/slp_sound_haptic/$(MODULE_ARCH)
pkg_LTLIBRARIES = module.la

module_la_SOURCES = sound_haptic.c
module_la_LIBADD = $(top_builddir)/src/lib/libedje.la
module_la_LDFLAGS = -no-undefined @lt_enable_auto_import@ -module -avoid-version `pkg-config --libs mm-sound mm-player vconf devman devman_haptic` 
module_la_LIBTOOLFLAGS = --tag=disable-static
