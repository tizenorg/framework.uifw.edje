#ifndef _EDJE_SOUND_H
#define _EDJE_SOUND_H

#if 0
#include<glib.h>
#include<mmf/mm_sound.h>
#include<vconf.h>
#include<vconf-keys.h>
#include<mmf/mm_sound_private.h>
#include<stdio.h> 
#include<unistd.h>
#include <mm_types.h>
#include <mm_error.h>
#include <iniparser.h>
#include <pthread.h>
#endif

#include <Edje.h>
#include "edje_private.h"


EAPI  bool ems_ui_sound_play(Edje_Sound_Info* s, void * sound_data, unsigned int iteration)  ;

#endif	
