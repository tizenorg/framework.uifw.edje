#include<Edje.h>
#include <edje_private.h>

extern Eina_Hash *_registered_modules;
Eina_List *_available_modules ;
extern char* sound_module_loaded ;

Eina_Bool (*msui_sound_play) (Edje_Sound_Info *, unsigned int, unsigned int) = NULL ;
Eina_Bool (*msui_haptic_play) (Edje_Haptic_Info *, unsigned int) = NULL ;

Eina_Bool edje_multisense_ui_init()
{
   if(sound_module_loaded!=NULL)
   {
	  Eina_Module *m = NULL;

	  //Already Loaded?
      if(!msui_sound_play && !msui_haptic_play)
         return EINA_TRUE ;

      edje_module_load(sound_module_loaded);
      _available_modules = edje_available_modules_get() ;
      m = eina_module_find(_available_modules, sound_module_loaded);

      if (!m)
      {
    	  msui_sound_play = eina_module_symbol_get(m, "msui_sound_play");
    	  msui_haptic_play = eina_module_symbol_get(m, "msui_haptic_play");
         return EINA_TRUE;
      }
   }

   return EINA_FALSE ;
}


/**
 *
 */
EAPI Eina_Bool
edje_multisense_ui_sound_play(const char* sound_name, unsigned int iterations, unsigned int volume)
{
   Edje_Sound_Info *sinfo;

   if (!msui_sound_play || !sound_name)
     {
	   return EINA_FALSE;
     }

   //TODO: Fetch Sound Info from Table using sound_name:  Sending NULL now.
   return ((*(msui_sound_play)) (sinfo, iterations, volume));
}

/**
 *
 */
EAPI Eina_Bool
edje_multisense_ui_haptic_play(const char* haptic_name, unsigned int iterations)
{
   Edje_Haptic_Info *hinfo;
   if (!msui_haptic_play || !haptic_name)
     {
	   return EINA_FALSE;
     }
   //TODO: Fetch Haptic Info from Table using haptic_name:  Sending NULL now.
   return ((*(msui_haptic_play)) (hinfo, iterations));
}
