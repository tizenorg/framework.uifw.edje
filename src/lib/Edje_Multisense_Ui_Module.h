#ifndef _EDJE_MULTISENSE_UI_MODULE_H_
#define _EDJE_MULTISENSE_UI_MODULE_H_

#include <Edje.h>

/**
 *
 *  @file Edje_Multisense_Ui_Module.h
 *  @brief  The header file comprises of API's to be implemented by Edje Multisense Module for providing the sound/haptic play functionality .
 *  For a reference of what all parameter means
 *  look at the complete @ref edcref
 *  
 *  /
 
 /**
  * Play the sound data being send by edje.
  * 
  * @param sinfo address of the sound definition structure as defined in edc 
  * @param sdata comprises of address of buffer having the sound data and size of buffer
  * @param iterations number of iterations of the sound to be played
  *
  *  @return EINA_TRUE on success EINA_FALSE on failure 
  * */
EAPI Eina_Bool ems_ui_sound_play( Edje_Sound_Info* sinfo, Edje_Sound_Data* sdata, unsigned int iterations );

/**
 * Play the haptic definition as send by edje
 * 
 * @param haptic holds the address of haptic definition as defined in edc 
 * @param iterations number of iterations of the haptic to be played
 * 
 * @return EINA_TRUE on success EINA_FALSE on failure 
 * */
EAPI Eina_Bool ems_ui_haptic_play( Edje_Haptic_Info *haptic, unsigned int iterations );

#endif	/* _EDJE_MULTISENSE_UI_MODULE_H_ */
