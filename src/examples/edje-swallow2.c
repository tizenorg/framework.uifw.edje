/**
 * Simple Edje example illustrating swallow functions.
 *
 * You'll need at least one Evas engine built for it (excluding the
 * buffer one). See stdout/stderr for output.
 *
 * @verbatim
 * edje_cc swallow.edc && gcc -o edje-swallow2 edje-swallow2.c `pkg-config --libs --cflags evas ecore ecore-evas edje`
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

static void
_on_delete(Ecore_Evas *ee __UNUSED__)
{
   ecore_main_loop_quit();
}

int
main(int argc __UNUSED__, char *argv[])
{
   char         edje_file_path[PATH_MAX];
   char         img_file_path[PATH_MAX];
   const char  *edje_file = "swallow.edj";
   const char  *img_file = "bubble.png";
   Ecore_Evas  *ee;
   Evas        *evas;
   Evas_Object *bg;
   Evas_Object *img;
   Evas_Object *obj;
   Evas_Object *edje_obj;
   Eina_Prefix *pfx;
   Evas_Load_Error err;

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

   ecore_evas_title_set(ee, "Edje Swallow Example");

   evas = ecore_evas_get(ee);

   bg = evas_object_rectangle_add(evas);
   evas_object_color_set(bg, 255, 255, 255, 255); /* white bg */
   evas_object_move(bg, 0, 0); /* at canvas' origin */
   evas_object_resize(bg, WIDTH, HEIGHT); /* covers full canvas */
   evas_object_show(bg);
   ecore_evas_data_set(ee, "background", bg);

   ecore_evas_object_associate(ee,bg, ECORE_EVAS_OBJECT_ASSOCIATE_BASE);

   edje_obj = edje_object_add(evas);

   snprintf(edje_file_path, sizeof(edje_file_path),
            "%s/examples/%s", eina_prefix_data_get(pfx), edje_file);
   edje_object_file_set(edje_obj, edje_file_path, "example_group");
   evas_object_move(edje_obj, 20, 20);
   evas_object_resize(edje_obj, WIDTH - 40, HEIGHT - 40);
   evas_object_show(edje_obj);

   snprintf(img_file_path, sizeof(edje_file_path),
            "%s/examples/%s", eina_prefix_data_get(pfx), img_file);

   img = evas_object_image_filled_add(evas);
   evas_object_image_file_set(img, img_file_path, NULL);

   err = evas_object_image_load_error_get(img);

   if (err != EVAS_LOAD_ERROR_NONE)
   {
      fprintf(stderr, "could not load image '%s'. error string is \"%s\"\n",
              img_file_path, evas_load_error_str(err));
     goto free_prefix;
   }

   edje_object_part_swallow(edje_obj, "part_one", img);

   obj = edje_object_part_swallow_get(edje_obj, "part_one");

   if(obj == img)
      printf("Swallowing worked!\n");

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
