#include"sound.h"
#include"haptic.h"

int dev_handle = -1 ;
Eina_Bool aui_haptic_play(char* filename,int iteration)
{
   int ret_val=0 ;
   int mode=0;
   if(dev_handle!=-1)device_haptic_close(dev_handle) ;
   dev_handle= device_haptic_open(1,mode);
   if(dev_handle < 0)return EINA_FALSE ;
   ret_val = device_haptic_play_file(dev_handle,filename,iteration,3);
   if(ret_val<0)
     return EINA_FALSE ;
   return EINA_TRUE ;
}	
			 	
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
int HexToDec(const char* strHex)
{	
   int nFactor = 1;
   int nRetVal = 0;
   char* p;
   for(p = strHex + strlen(strHex); p >= strHex; --p)
     {
        if( p[0] == ' ' || p[0] == 0) continue ;		
       	if(p[0] < 'A') nRetVal += nFactor * (p[0] - '0');
	else if(p[0] < 'a') nRetVal += nFactor * (p[0] - 'A' + 10);											
	else nRetVal += nFactor * (p[0] - 'a' + 10);												
 	nFactor *= 16;			
     }
	   
   return nRetVal;
}

EAPI Eina_Bool ems_ui_haptic_play(Edje_Haptic_Info *haptic, unsigned int iterations)
   {
	char *pattern =NULL ;
	pattern=(char*)malloc(strlen(haptic->pattern)) ;
        char *t = NULL;
        strcpy(pattern, haptic->pattern);
	char *s =pattern ;
        int a = 0;
	char buf[256] ;
	char test[3] ;
	test[2]=0 ;
	int i=0 ;

	int counter = 0;
	char data ;
	snprintf(buf,sizeof(buf),"/tmp/%s.ivt",haptic->name) ;
	FILE *f=fopen(buf,"w") ;
	
	 while (*s != '\0')
	{
           if (*s == ',')
   	     counter++;
	   s++;
	}
	
	 while (a <= counter)
	      {
 		 test[0]= pattern[i] ;
	         test[1]=pattern[i+1] ;
      	         i = i + 3 ;
		 data=HexToDec(test) ;
		 fputc(data,f) ;
		 a++ ;
	      }
	     
	     fclose(f) ;
	     if(aui_haptic_play(buf,iterations)==EINA_TRUE) ;
	     return EINA_TRUE    ;
   }
