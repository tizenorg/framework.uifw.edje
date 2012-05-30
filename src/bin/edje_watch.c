#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <Eina.h>
#include <Ecore.h>
#include <Eio.h>
#ifdef HAVE_EVIL
# include <Evil.h>
#endif

char watchfile[PATH_MAX];
char *edje_cc_command = NULL;
Eina_List *watching = NULL;
Ecore_Timer *timeout = NULL;

static void
read_watch_file(const char *file)
{
   Eina_File *f;
   Eina_Iterator *it;
   Eina_File_Lines *ln;
   Eio_Monitor *mon;
   Eina_List *r = NULL;

   f = eina_file_open(file, EINA_FALSE);
   if (!f) return ;

   it = eina_file_map_lines(f);
   if (!it) goto err;

   EINA_ITERATOR_FOREACH(it, ln)
     {
        const char *path;

        path = eina_stringshare_add_length(ln->line.start, ln->length);
        r = eina_list_append(r, eio_monitor_add(path));
        eina_stringshare_del(path);
     }
   eina_iterator_free(it);

   EINA_LIST_FREE(watching, mon)
     eio_monitor_del(mon);
   watching = r;

 err:
   eina_file_close(f);
}

Eina_Bool
rebuild(void *data __UNUSED__)
{
   double start, end;

   start = ecore_time_get();
   fprintf(stderr, "SYSTEM('%s')\n", edje_cc_command);
   if (system(edje_cc_command) == 0)
     read_watch_file(watchfile);
   end = ecore_time_get();
   fprintf(stderr, "DONE IN %f\n", end - start);

   timeout = NULL;
   return EINA_FALSE;
}

Eina_Bool
some_change(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   Eio_Monitor_Event *ev = event;

   fprintf(stderr, "EVENT %i on [%s]\n", type, ev->filename);
   if (timeout) ecore_timer_del(timeout);
   timeout = ecore_timer_add(0.5, rebuild, NULL);

   return ECORE_CALLBACK_PASS_ON;
}

int
main(int argc, char **argv)
{
   char *watchout;
   Eina_Strbuf *buf;
   double start, end;
   int tfd;
   int i;

   eina_init();
   ecore_init();
   eio_init();

   if (argc < 2) return -1;

   ecore_event_handler_add(EIO_MONITOR_FILE_MODIFIED, some_change, NULL);
   ecore_event_handler_add(EIO_MONITOR_FILE_CREATED, some_change, NULL);
   ecore_event_handler_add(EIO_MONITOR_FILE_DELETED, some_change, NULL);
   ecore_event_handler_add(EIO_MONITOR_FILE_CLOSED, some_change, NULL);

#ifdef HAVE_EVIL
   watchout = (char *)evil_tmpdir_get();
#else
   watchout = "/tmp";
#endif

   snprintf(watchfile, PATH_MAX, "%s/edje_watch-tmp-XXXXXX", watchout);

   tfd = mkstemp(watchfile);
   if (tfd < 0) return -1;
   close(tfd);

   buf = eina_strbuf_new();
   if (!buf) return -1;

   eina_strbuf_append_printf(buf, "%s/edje_cc -threads -fastcomp -w %s ", PACKAGE_BIN_DIR, watchfile);
   for (i = 1; i < argc; ++i)
     eina_strbuf_append_printf(buf, "%s ", argv[i]);

   edje_cc_command = eina_strbuf_string_steal(buf);

   eina_strbuf_free(buf);

   start = ecore_time_get();
   fprintf(stderr, "SYSTEM('%s')\n", edje_cc_command);
   system(edje_cc_command);
   read_watch_file(watchfile);
   end = ecore_time_get();
   fprintf(stderr, "DONE %f\n", end - start);

   ecore_main_loop_begin();

   unlink(watchfile);

   eio_shutdown();
   ecore_shutdown();
   eina_shutdown();

   return 1;
}
