#include"sound.h"
#include"haptic.h"


EAPI Eina_Bool ems_ui_sound_play(Edje_Sound_Info* sinfo, Edje_Sound_Data* sdata, unsigned int it)
  {    
     int volume_level = 0 ;
     int sound_profile;
     FILE *f ;
     char buf[256] ;
     struct stat st;	  
     /*Checking for silent profile */
     if(vconf_get_int(VCONFKEY_SETAPPL_CUR_PROFILE_INT,&sound_profile ) ) ;
     if( sound_profile==1 )return EINA_FALSE; 
     if( vconf_get_int(VCONFKEY_SETAPPL_PROFILE_NORMAL_CALL_VOLUME_INT,&volume_level)==-1)return EINA_FALSE ;
     if( sinfo==NULL )return EINA_FALSE ;
     snprintf(buf,sizeof(buf),"/tmp/%s",sinfo->name) ;
     /* Sound File already present in the /tmp directory */
     if (stat(buf,&st) == 0)
       goto PLAY ;
     f = fopen(buf,"wb") ;
     if (f != NULL)
       {  
           if (fwrite(sdata->sound_data, sdata->sound_size, 1, f) != 1)
  	     return EINA_FALSE ;
	   fclose(f);
       }  
     else 
       return EINA_FALSE ;
    PLAY:
       mm_sound_play_keysound(buf,volume_level)  ;
     return EINA_TRUE ;
  }

EAPI Eina_Bool ems_ui_haptic_play(Edje_Haptic_Info *haptic, unsigned int iterations)
   {
	/*TO BE IMPLEMENTED*/	

    }
