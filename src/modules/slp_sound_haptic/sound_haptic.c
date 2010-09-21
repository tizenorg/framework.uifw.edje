#include"sound.h"
#include"haptic.h"
#include "tgmath.h"
#include<dlfcn.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>



#include <sys/types.h>
#include <limits.h>

EAPI bool
ems_ui_sound_play(Edje_Sound_Info *s, void* sound_data, unsigned int iteration )
{
	/*Implementation Problem Due to dependency*/
	printf("ems_ui_sound_play");
	return 0 ;
}


EAPI bool
ems_ui_haptic_play(Edje_Haptic_Info *haptic, unsigned int iterations) 
{
 /*Implementation Problem Due to dependency*/	
printf("ems_ui_haptic_play");
	return 0 ;

}
