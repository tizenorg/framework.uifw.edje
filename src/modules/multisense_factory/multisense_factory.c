#include "config.h"
#include "edje_private.h"

int _multisense_factory_log_dom = -1;
int init_count = 0;

#ifdef INF
# undef INF
#endif
#define INF(...) EINA_LOG_DOM_INFO(_multisense_factory_log_dom, __VA_ARGS__)

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
     return NULL;//remix_monitor_new(env);

   player = remix_new(env, player_plugin, NULL);
   return player;
}
#endif

EAPI Eina_Bool
multisense_factory_init(Edje_Multisense_Env *env)
{
   init_count++;
   if (init_count == 1)
     {
        eina_init();
        eina_log_level_set(EINA_LOG_LEVEL_INFO);
        _multisense_factory_log_dom = eina_log_domain_register("multisense_factory", EINA_COLOR_CYAN);
     }
#ifdef HAVE_LIBREMIX
   remix_set_samplerate(env->remixenv, DEFAULT_SAMPLERATE);
#endif
   return EINA_TRUE;
}
EAPI Eina_Bool
multisense_factory_shutdown(Edje_Multisense_Env *env __UNUSED__)
{
   init_count--;
   if (init_count == 0)
     {
        eina_log_domain_unregister(_multisense_factory_log_dom);
        _multisense_factory_log_dom = -1;
     }
   return EINA_TRUE;
}
