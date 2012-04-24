/*
 * Remix Pulse Audio Player: Pulse Audio output
 *
 * Prince Kumar Dubey <prince.dubey@samsung.com>, April 2012
 */

#include "config.h"
#include <stdio.h>
#include <remix/remix.h>
#include <Eina.h>
#include <pulse/simple.h>
#include <pulse/error.h>
#include <pulse/gccmacro.h>
#include <pulse/proplist.h>

#define PA_PLAYER_BUFFERLEN 4096

typedef struct _PA_Player_Data PA_Player_Data;
typedef short PLAYER_PCM;

struct _PA_Player_Data
{
   RemixPCM databuffer[PA_PLAYER_BUFFERLEN];
   PLAYER_PCM *playbuffer;
   pa_simple *server;
   int error;
   unsigned int stereo;
   unsigned channels;
   unsigned int frequency;
   RemixPCM max_value;
};

static int _log_dom = -1;
static int init_count = 0;

#ifdef WRN
# undef WRN
#endif
#define WRN(...) EINA_LOG_DOM_WARN(_log_dom, __VA_ARGS__)

/* Optimisation dependencies: none */
static RemixBase *pa_player_optimise(RemixEnv *env, RemixBase *base);

static RemixBase *
pa_player_reset_device(RemixEnv *env, RemixBase *base)
{
   pa_sample_spec sample_spec;

   PA_Player_Data *player_data = remix_base_get_instance_data(env, base);
   sample_spec.rate = player_data->frequency;
   sample_spec.channels = player_data->channels;
   sample_spec.format = PA_SAMPLE_S16NE;

   if (player_data->server)
     {
        /* Make sure that every single sample was played */
        if (pa_simple_drain(player_data->server, &player_data->error) < 0)
          {
             WRN("pa_simple_drain() failed: (%s)", pa_strerror(player_data->error));
             goto error;
          }
        /* Make sure that all data currently in buffers are thrown away. */
        if (pa_simple_flush (player_data->server, &player_data->error) < 0)
          {
             WRN("pa_simple_flush() failed: (%s)", pa_strerror(player_data->error));
             goto error;
          }
     }

   /* Create a new playback stream */
   if (!(player_data->server = pa_simple_new(NULL, "EFL Edje", PA_STREAM_PLAYBACK,
                                             NULL, "Edje Multisense audio",
                                             &sample_spec, NULL,
                                             NULL, &player_data->error)))
     {
        WRN("pa_simple_new() failed: (%s)", pa_strerror(player_data->error));
        goto error;
     }

   if (player_data->playbuffer) free(player_data->playbuffer);
   player_data->playbuffer = calloc(sizeof(PLAYER_PCM), PA_PLAYER_BUFFERLEN);

   if(!player_data->playbuffer) goto error;

   if (sample_spec.rate != player_data->frequency)
     {
        player_data->frequency = sample_spec.rate;
        remix_set_samplerate(env, player_data->frequency);
     }

   return base;

error:
   if (player_data->server)
     pa_simple_free(player_data->server);
   remix_set_error(env, REMIX_ERROR_SYSTEM);
   return RemixNone;
}

static RemixBase *
pa_player_init(RemixEnv *env, RemixBase *base, CDSet *parameters __UNUSED__)
{
   CDSet *channels;

   PA_Player_Data *player_data = calloc(1, sizeof(PA_Player_Data));

   if (!player_data)
     {
        remix_set_error(env, REMIX_ERROR_SYSTEM);
        return RemixNone;
     }

   init_count++;
   if (init_count == 1)
     {
        eina_init();
        _log_dom = eina_log_domain_register("remix-pulseaudio", EINA_COLOR_CYAN);
     }

   remix_base_set_instance_data(env, base, player_data);
   channels = remix_get_channels(env);

   player_data->channels = cd_set_size(env, channels);
   if (player_data->channels == 1) player_data->stereo = 0;
   else if (player_data->channels == 2) player_data->stereo = 1;

   player_data->frequency = remix_get_samplerate(env);
   player_data->max_value = (RemixPCM) SHRT_MAX / 2;

   pa_player_reset_device(env, base);
   base = pa_player_optimise(env, base);

   return base;
}

static RemixBase *
pa_player_clone(RemixEnv *env, RemixBase *base __UNUSED__)
{
   RemixBase *new_player = remix_base_new(env);
   pa_player_init(env, new_player, NULL);
   return new_player;
}

static int
pa_player_destroy(RemixEnv *env, RemixBase *base)
{
   PA_Player_Data *player_data = remix_base_get_instance_data(env, base);

   if (player_data->server)
     {
        /* Make sure that every single sample was played */
        if (pa_simple_drain(player_data->server, &player_data->error) < 0)
          WRN("pa_simple_drain() failed: (%s)", pa_strerror(player_data->error));

       /* Make sure that all data currently in buffers are thrown away. */
       if (pa_simple_flush (player_data->server, &player_data->error) < 0)
         WRN("pa_simple_flush() failed: (%s)", pa_strerror(player_data->error));

        pa_simple_free(player_data->server);
     }
   if (player_data->playbuffer) free(player_data->playbuffer);

   free(player_data);
   init_count--;
   if (init_count == 0)
     {
        eina_log_domain_unregister(_log_dom);
        _log_dom = -1;
        eina_shutdown();
     }
   return 0;
}

static int
pa_player_ready(RemixEnv *env, RemixBase *base)
{
   RemixCount nr_channels;
   CDSet *channels;
   int samplerate;

   PA_Player_Data *player_data = remix_base_get_instance_data(env, base);

   channels = remix_get_channels(env);
   samplerate = (int)remix_get_samplerate(env);
   nr_channels = cd_set_size(env, channels);
   return ((samplerate == (int)player_data->frequency) &&
           (((nr_channels == 1) && (player_data->stereo == 0)) ||
               ((nr_channels > 1) && (player_data->stereo == 1))));
}

static RemixBase *
pa_player_prepare(RemixEnv *env, RemixBase *base)
{
   pa_player_reset_device(env, base);
   return base;
}

static void
pa_player_playbuffer(RemixEnv *env __UNUSED__, PA_Player_Data *player, RemixPCM *data, RemixCount count)
{
   int ret;
   RemixCount i;
   RemixPCM value;
   size_t length;

   length = count * sizeof(RemixCount);

   for (i = 0; i < length; i++)
     {
        value = *data++ * (player->max_value);
        *(player->playbuffer + i) = (PLAYER_PCM) value;
     }

   ret = pa_simple_write(player->server, player->playbuffer, length, &player->error);

   if (ret < 0) WRN("pa_simple_write() failed: (%s)", pa_strerror(player->error));

   return;
}

static RemixCount
pa_player_chunk(RemixEnv *env, RemixChunk *chunk, RemixCount offset, RemixCount count, int channelname __UNUSED__, void *data)
{
   RemixCount remaining = count, written = 0, playcount;
   RemixPCM *d;

   PA_Player_Data *player = data;

   while (remaining > 0)
     {
        playcount = MIN(remaining, PA_PLAYER_BUFFERLEN);

        d = &chunk->data[offset];
        pa_player_playbuffer(env, player, d, playcount);

        offset += playcount;
        written += playcount;
        remaining -= playcount;
     }
   return written;
}

static RemixCount
pa_player_process(RemixEnv *env, RemixBase *base, RemixCount count, RemixStream *input, RemixStream *output __UNUSED__)
{
   PA_Player_Data *player_data = remix_base_get_instance_data(env, base);
   RemixCount nr_channels = remix_stream_nr_channels(env, input);
   RemixCount remaining = count, processed = 0, n;

   if ((nr_channels == 1) && (player_data->stereo == 0))
     { /*MONO*/
        return remix_stream_chunkfuncify(env, input, count,
                                         pa_player_chunk, player_data);
     }
   else if ((nr_channels == 2) && (player_data->stereo == 1))
     { /*STEREO*/
        while (remaining > 0)
          {
             n = MIN(remaining, PA_PLAYER_BUFFERLEN / 2);
             n = remix_stream_interleave_2(env, input,
                                           REMIX_CHANNEL_LEFT,
                                           REMIX_CHANNEL_RIGHT,
                                           player_data->databuffer, n);
             pa_player_playbuffer(env, player_data,
                                         player_data->databuffer, n);
             processed += n;
             remaining -= n;
          }
        return processed;
     }
   WRN("[pa_player_process] unsupported stream/output channel "
       "combination %ld / %d", nr_channels, player_data->stereo ? 2 : 1);
   return -1;
}

static RemixCount
pa_player_length(RemixEnv *env __UNUSED__, RemixBase *base __UNUSED__)
{
   return REMIX_COUNT_INFINITE;
}

static RemixCount
pa_player_seek(RemixEnv *env __UNUSED__, RemixBase *base __UNUSED__, RemixCount count __UNUSED__)
{
   return count;
}

static int
pa_player_flush(RemixEnv *env, RemixBase *base)
{
   pa_player_reset_device(env, base);
   return 0;
}

static struct _RemixMethods _pa_player_methods =
{
   pa_player_clone,
   pa_player_destroy,
   pa_player_ready,
   pa_player_prepare,
   pa_player_process,
   pa_player_length,
   pa_player_seek,
   pa_player_flush,
};

static RemixBase *
pa_player_optimise(RemixEnv *env, RemixBase *base)
{
   remix_base_set_methods(env, base, &_pa_player_methods);
   return base;
}

static struct _RemixMetaText pa_player_metatext =
{
   "pa_snd_player",
   "PA sound player for Remix",
   "Output the audio stream into Pulse Audio",
   "Copyright (C) 2012, Samsung Electronics Co., Ltd.",
   "http://www.samsung.com",
   REMIX_ONE_AUTHOR("Prince Kr Dubey", "prince.dubey@samsung.com"),
};

static struct _RemixPlugin pa_player_plugin =
{
   &pa_player_metatext,
   REMIX_FLAGS_NONE,
   CD_EMPTY_SET, /* init scheme */
   pa_player_init,
   CD_EMPTY_SET, /* process scheme */
   NULL, /* suggests */
   NULL, /* plugin data */
   NULL  /* destroy */
};

EAPI CDList *
remix_load(RemixEnv *env)
{
   CDList *plugins = cd_list_new(env);
   plugins = cd_list_prepend(env, plugins, CD_POINTER(&pa_player_plugin));
   return plugins;
}
