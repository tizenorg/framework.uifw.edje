FACTORY_MODULE =  multisense_factory

if HAVE_LIBREMIX
SND_READER_MODULE = eet_snd_reader
if HAVE_LIBALSA
ALSA_MODULE =  alsa_snd_player
endif
if HAVE_LIBPA
PA_MODULE =  pa_snd_player
endif
endif

SUBDIRS = $(FACTORY_MODULE) $(SND_READER_MODULE) $(ALSA_MODULE) $(PA_MODULE)
