#ifndef _EDJE_SOUND_H
#define _EDJE_SOUND_H

#include<mmf/mm_sound.h>
#include<vconf.h>
#include<mmf/mm_sound_private.h>
#include <Edje.h>
#include "edje_private.h"
#include <unistd.h>
#include <sys/stat.h>



EAPI Eina_Bool ems_ui_sound_play(Edje_Sound_Info* sinfo, Edje_Sound_Data* sdata, unsigned int it);

#endif	
