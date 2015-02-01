#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <sys/wait.h>

#include <Eina.h>
#include <Ecore.h>
#include <Eio.h>
#ifdef HAVE_EVIL
# include <Evil.h>
#endif

static char watchfile[PATH_MAX];
static char *edje_cc_command = NULL;
static Eina_List *watching = NULL;
static Ecore_Timer *timeout = NULL;
static Eina_Bool anotate = EINA_FALSE;

static void
read_watch_file(const char *file)
{
   Eina_File *f;
   Eina_Iterator *it;
   Eina_File_Line *ln;
   Eio_Monitor *mon;
   Eina_List *r = NULL;

   f = eina_file_open(file, EINA_FALSE);
   if (!f) return ;

   it = eina_file_map_lines(f);
   if (!it) goto err;

   EINA_ITERATOR_FOREACH(it, ln)
     {
        const char *path;
        Eina_Bool do_append = EINA_TRUE;

	if (ln->length < 4) continue ;
        if (anotate)
          {
             path = eina_stringshare_add_length(ln->start + 3, ln->length - 3);
             fprintf(stdout, "%c: %s\n", *ln->start, path);
             if (*ln->start == 'O')
               do_append = EINA_FALSE;
          }
        else
          {
             path = eina_stringshare_add_length(ln->start, ln->length);
          }
        if (do_append)
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
   int ret;

   start = ecore_time_get();
   fprintf(stdout, "* SYSTEM('%s')\n", edje_cc_command);
   fflush(stdout);

   ret = system(edje_cc_command);
   if (WEXITSTATUS(ret) == 0)
     read_watch_file(watchfile);
   end = ecore_time_get();
   fprintf(stdout, "* DONE IN %f\n", end - start);
   fflush(stdout);

   timeout = NULL;
   return EINA_FALSE;
}

Eina_Bool
some_change(void *data __UNUSED__, int type __UNUSED__, void *event __UNUSED__)
{
   if (timeout) ecore_timer_del(timeout);
   timeout = ecore_timer_add(0.5, rebuild, NULL);

   return ECORE_CALLBACK_PASS_ON;
}

int
main(int argc, char **argv)
{
   char *watchout;
   Eina_Strbuf *buf;
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

   eina_strbuf_append_printf(buf, "%s/edje_cc -fastcomp -w %s ", PACKAGE_BIN_DIR, watchfile);
   for (i = 1; i < argc; ++i)
     {
        if (!strcmp(argv[i], "-anotate"))
          anotate = EINA_TRUE;
        eina_strbuf_append_printf(buf, "%s ", argv[i]);
     }
   eina_strbuf_append(buf, "> /dev/null 2>/dev/null");

   edje_cc_command = eina_strbuf_string_steal(buf);

   eina_strbuf_free(buf);

   rebuild(NULL);

   ecore_main_loop_begin();

   unlink(watchfile);

   eio_shutdown();
   ecore_shutdown();
   eina_shutdown();

   return 1;
}
