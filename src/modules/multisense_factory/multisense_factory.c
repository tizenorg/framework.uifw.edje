#include "config.h"
#include "edje_private.h"

#define DEFAULT_SAMPLERATE 44100

#ifdef HAVE_LIBREMIX
EAPI RemixBase *
multisense_sound_player_get(Edje_Multisense_Env *msenv)
{
   RemixEnv *env = msenv->remixenv;
   RemixPlugin *player_plugin = NULL;
   RemixBase *player;
   char *ms_player_name = NULL;

   ms_player_name = getenv("MULTISENSE_SND_PLAYER");
   if (ms_player_name)
     {
        player_plugin = remix_find_plugin(env, ms_player_name);
        INF("Custom player_plugin = %p \n", player_plugin);
     }
#ifdef HAVE_LIBPA
   if (!player_plugin)
     {
        player_plugin = remix_find_plugin(env, "pa_snd_player");
        INF("PA player_plugin = %p \n", player_plugin);
     }
#endif
#ifdef HAVE_LIBALSA
   if (!player_plugin)
     {
        player_plugin = remix_find_plugin(env, "alsa_snd_player");
        INF("ALSA player_plugin = %p \n", player_plugin);
     }
#endif
   if (!player_plugin)
     return remix_monitor_new(env);

   player = remix_new(env, player_plugin, NULL);
   return player;
}
#endif

EAPI Eina_Bool
multisense_factory_init(Edje_Multisense_Env *env __UNUSED__)
{
#ifdef HAVE_LIBREMIX
   remix_set_samplerate(env->remixenv, DEFAULT_SAMPLERATE);
#endif
   return EINA_TRUE;
}
