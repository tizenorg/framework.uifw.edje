## Process this file with automake to produce Makefile.in

MAINTAINERCLEANFILES = Makefile.in

pkglibdir = $(REMIX_PLUGIN_DIR)

AM_CPPFLAGS = \
-I. \
@EDJE_CFLAGS@ \
@REMIX_CFLAGS@ \
@PA_CFLAGS@

pkgdir = $(REMIX_PLUGIN_DIR)
pkg_LTLIBRARIES = libpa_snd_player.la

libpa_snd_player_la_SOURCES = pa_simple_snd_player.c
libpa_snd_player_la_LDFLAGS =  -no-undefined @lt_enable_auto_import@ -module -avoid-version @EDJE_LIBS@ @REMIX_LIBS@ @PA_LIBS@
libpa_snd_player_la_LIBTOOLFLAGS = --tag=disable-static
