/**
 * Simple Edje example illustrating text functions.
 *
 * You'll need at least one Evas engine built for it (excluding the
 * buffer one). See stdout/stderr for output.
 *
 * @verbatim
 * edje_cc text.edc && gcc -o edje-text edje-text.c `pkg-config --libs --cflags ecore-evas edje`
 * @endverbatim
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#else
# define __UNUSED__
#endif

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>

#define WIDTH  (300)
#define HEIGHT (300)

#define PATH_MAX  256
#define PACKAGE_BIN_DIR "."
#define PACKAGE_LIB_DIR "."
#define PACKAGE_DATA_DIR "."

static void
_on_delete(Ecore_Evas *ee __UNUSED__)
{
   ecore_main_loop_quit();
}

static void
_on_text_change(void *data __UNUSED__, Evas_Object *obj, const char *part)
{
   printf("text: %s\n", edje_object_part_text_unescaped_get(obj, part));
}

int
main(int argc __UNUSED__, char *argv[])
{
   char         edje_file_path[PATH_MAX];
   const char  *edje_file = "text.edj";
   Ecore_Evas  *ee;
   Evas        *evas;
   Evas_Object *bg;
   Evas_Object *edje_obj;
   Eina_Prefix *pfx;

   if (!ecore_evas_init())
     return EXIT_FAILURE;

   if (!edje_init())
     goto shutdown_ecore_evas;

   pfx = eina_prefix_new(argv[0], main,
                         "EDJE_EXAMPLES",
                         "edje/examples",
                         edje_file,
                         PACKAGE_BIN_DIR,
                         PACKAGE_LIB_DIR,
                         PACKAGE_DATA_DIR,
                         PACKAGE_DATA_DIR);
   if (!pfx)
     goto shutdown_edje;

   /* this will give you a window with an Evas canvas under the first
    * engine available */
   ee = ecore_evas_new(NULL, 0, 0, WIDTH, HEIGHT, NULL);
   if (!ee)
     goto free_prefix;

   ecore_evas_callback_delete_request_set(ee, _on_delete);
   ecore_evas_title_set(ee, "Edje text Example");

   evas = ecore_evas_get(ee);

   bg = evas_object_rectangle_add(evas);
   evas_object_color_set(bg, 255, 255, 255, 255); /* white bg */
   evas_object_move(bg, 0, 0); /* at canvas' origin */
   evas_object_resize(bg, WIDTH, HEIGHT); /* covers full canvas */
   evas_object_show(bg);
   ecore_evas_object_associate(ee, bg, ECORE_EVAS_OBJECT_ASSOCIATE_BASE);

   edje_obj = edje_object_add(evas);

   snprintf(edje_file_path, sizeof(edje_file_path),
            "%s/examples/%s", eina_prefix_data_get(pfx), edje_file);
   edje_object_file_set(edje_obj, edje_file_path, "example_group");
   evas_object_move(edje_obj, 20, 20);
   evas_object_resize(edje_obj, WIDTH - 40, HEIGHT - 40);
   evas_object_show(edje_obj);

   edje_object_text_change_cb_set(edje_obj, _on_text_change, NULL);
   edje_object_part_text_set(edje_obj, "part_one", "one");
   edje_object_part_text_set(edje_obj, "part_two", "<b>two");

   edje_object_part_text_select_allow_set(edje_obj, "part_two", EINA_TRUE);
   edje_object_part_text_select_all(edje_obj, "part_two");
   printf("selection: %s\n", edje_object_part_text_selection_get(edje_obj, "part_two"));
   edje_object_part_text_select_none(edje_obj, "part_two");
   printf("selection: %s\n", edje_object_part_text_selection_get(edje_obj, "part_two"));

   ecore_evas_show(ee);

   ecore_main_loop_begin();

   eina_prefix_free(pfx);
   ecore_evas_free(ee);
   ecore_evas_shutdown();
   edje_shutdown();

   return EXIT_SUCCESS;

 free_prefix:
   eina_prefix_free(pfx);
 shutdown_edje:
   edje_shutdown();
 shutdown_ecore_evas:
   ecore_evas_shutdown();

   return EXIT_FAILURE;
}
