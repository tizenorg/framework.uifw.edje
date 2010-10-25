#include <string.h>
#include <Edje.h>
#include "edje_private.h"

#define EMS_SOUND_PLAY_PLUGIN_API  "ems_ui_sound_play"
#define EMS_HAPTIC_PLAY_PLUGIN_API "ems_ui_haptic_play"

static Eina_Bool ems_ui_module_loaded = EINA_FALSE;

static Eina_Bool (*ems_ui_sound_play) (Edje_Sound_Info* , Edje_Sound_Data*, unsigned int ) = NULL;
static Eina_Bool (*ems_ui_haptic_play) (Edje_Haptic_Info* , unsigned int ) = NULL;

Eina_Bool _edje_multisense_ui_init()
{
   if (!ems_ui_module_loaded )
   {
    char  *ems_ui_module_name = NULL ;
	Eina_Module *m = NULL;
	ems_ui_module_name = getenv("EDJE_MULTISENSE_UI_MODULE");
	if(!ems_ui_module_name) return EINA_FALSE;
	m = edje_module_load(ems_ui_module_name);
	if(!m) return EINA_FALSE ;
	ems_ui_sound_play = eina_module_symbol_get(m, "ems_ui_sound_play");
	ems_ui_haptic_play = eina_module_symbol_get(m, "ems_ui_haptic_play" );
	ems_ui_module_loaded =  EINA_TRUE;
    }

   return ems_ui_module_loaded;
}

EAPI Eina_Bool
edje_multisense_ui_sound_play(Evas_Object *obj, const char* sound_name, unsigned int iterations )
{ 
   Edje *ed;
   Edje_Sound_Info *snd_info ;
   char sound_eet_path[30];
   Eina_List *l;
   Eet_File *ef ;
   Edje_Sound_Data *sdata = NULL;
   Eina_Bool result =  EINA_FALSE;
   ed = _edje_fetch(obj);
   if(!ems_ui_sound_play || !sound_name || !ed || !ed->file->sound_dir )
     return EINA_FALSE; 
   if (ed->file)
     {
	//Locate the Sound Info
        EINA_LIST_FOREACH(ed->file->sound_dir->entries, l, snd_info)
          {   
             if (!strcmp(snd_info->name, sound_name))
     	       {
	       	   sdata = (Edje_Sound_Data*)malloc(sizeof(Edje_Sound_Data));     
       		   snprintf(sound_eet_path, sizeof(sound_eet_path), "edje/sounds/%i", snd_info->id);
 		   if (ed->file->path )
 		     {  
	    	       ef = eet_open(ed->file->path, EET_FILE_MODE_READ);
	       	       sdata->sound_data = eet_read(ef, sound_eet_path, &(sdata->sound_size));
		       if (sdata->sound_data!=NULL) 
			 result =  EINA_TRUE;
		       break;
	 	    }
     	       }
	    
	  }
      }

    if (result)
      {
         result = ((*(ems_ui_sound_play)) (snd_info, sdata, iterations ));

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

    if (result)
      {
         result = ((*(ems_ui_haptic_play)) (hinfo, iterations));
      }
    return result;
}

