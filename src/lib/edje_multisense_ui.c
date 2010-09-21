#include <Edje.h>
#include "edje_private.h"


static Eina_Bool ems_ui_module_loaded = EINA_FALSE;

static Eina_Bool (*ems_ui_sound_play) (Edje_Sound_Info *, void *, unsigned int ) = NULL;
static Eina_Bool (*ems_ui_haptic_play) (Edje_Haptic_Info *, unsigned int) = NULL;

Eina_Bool edje_multisense_ui_init()
{
	if(!ems_ui_module_loaded )
	{
		char  *ems_ui_module_name = NULL ;
		Eina_List *_available_modules ;
		Eina_Module *m = NULL;

		ems_ui_module_name = getenv("EDJE_MULTISENSE_UI_MODULE");

		printf( "ems_ui_module_name:%s\n", ems_ui_module_name);

		if(!ems_ui_module_name)
			return EINA_FALSE;

		edje_module_load(ems_ui_module_name);
		_available_modules = edje_available_modules_get() ;
		m = eina_module_find(_available_modules, ems_ui_module_name);

		printf( "ems_ui_module:%d\n", m);
		if(!m)
			return EINA_FALSE;

		ems_ui_sound_play = eina_module_symbol_get(m, "ems_ui_sound_play");
		ems_ui_haptic_play = eina_module_symbol_get(m, "ems_ui_haptic_play");

		printf( " %d, %d\n", ems_ui_sound_play, ems_ui_haptic_play);

		ems_ui_module_loaded =  EINA_TRUE;
	}

	return ems_ui_module_loaded;
}




/**
 *
 */
EAPI Eina_Bool
edje_multisense_ui_sound_play(Evas_Object *obj, const char* sound_name, unsigned int iterations )
{ 
   Edje *ed;
   Edje_Sound_Info *snd_info;
   void *sound_data = NULL ;
   long sound_data_size = 0;
   char sound_eet_path[15];
   Eina_List *l;
   FILE *F ;
   Eina_Bool result =  EINA_FALSE;

   ed = _edje_fetch(obj);

printf( " before play sound\n");

   if(!ems_ui_sound_play || !sound_name || !ed || !ed->file->sound_dir )
        return EINA_FALSE;

printf( " before play sound - 1\n");
    //Locate the Sound Info
   EINA_LIST_FOREACH(ed->file->sound_dir->entries, l, snd_info)
   {
      if (!strcmp(snd_info->name, sound_name))
      {
		snprintf(sound_eet_path, sizeof(sound_eet_path), "sounds/%i", snd_info->id);
		sound_data = eet_read(ed->file->ef, sound_eet_path, &sound_data_size);
		result =  EINA_TRUE;
		/*FILE *F = fopen(snd_info->name,"rb");
		if(F!=NULL)
		   {
		       if (fwrite(sound_byte, sound_data_size, 1, F) != 1)
  	              fclose(F) ;
		       sound_error = mm_sound_play_keysound(snd_info-name, s->volume);*/
	  	      break;
      }
   }

	if(result)
	{
	printf( " before play sound - 2\n");
	  result = ((*(ems_ui_sound_play)) (snd_info, sound_data, iterations ));
	  printf( " after play sound\n");
	}
    return result;
}

/**
 *
 */
EAPI Eina_Bool
edje_multisense_ui_haptic_play(Evas_Object *obj, const char* haptic_name, unsigned int iterations)
{
   Edje *ed;
   Edje_Haptic_Info *hinfo;
   Eina_List *l ;
   Eina_Bool result =  EINA_FALSE;

   ed = _edje_fetch(obj);

   if(!ems_ui_haptic_play || !haptic_name || !ed )
         return EINA_FALSE;

	EINA_LIST_FOREACH(ed->file->haptics, l, hinfo)
	{
        if (!strcmp(hinfo->name, haptic_name))
         {
        	result =  EINA_TRUE;
        	break;
         }
     }

	if(result)
	{
		result = ((*(ems_ui_haptic_play)) (hinfo, iterations));
	}

	return result;
}

