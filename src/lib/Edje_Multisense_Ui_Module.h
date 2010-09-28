#ifndef _EDJE_MULTISENSE_UI_MODULE_H_
#define _EDJE_MULTISENSE_UI_MODULE_H_

#include <Edje.h>

/**
 * APIs to be implemented by Edje Multisense Modules.
 */

EAPI Eina_Bool ems_ui_sound_play( Edje_Sound_Info* sinfo, Edje_Sound_Data* sdata, unsigned int iterations );

EAPI Eina_Bool ems_ui_haptic_play( Edje_Haptic_Info *haptic, unsigned int iterations );

#endif	/* _EDJE_MULTISENSE_UI_MODULE_H_ */
