/**
 * @since 1.8.0
 * Simple Edje example illustrating the edje_codegen usage.
 *
 * edje_codegen is a tool that generates code to acessing the parts and programs
 * with the keyword "api" of a specified group. This tool make easier working
 * with edje avoiding common errors caused by typos when acessing the parts
 * and/or programs.
 *
 * To use the edje_codegen:
 * edje_codegen --prefix <myapp_myobj> <input.edj> <a_group> <source.c> <header.h>
 *
 * In case of this example:
 * edje_codegen --prefix=codegen_example codegen.edj group_example \
	codegen_example_generated.c codegen_example_generated.h
 *
 * @verbatim
 * edje_cc codegen.edc && edje_codegen --prefix=codegen_example codegen.edj \
 * example_group codegen_example_generated.c codegen_example_generated.h
 * gcc -c codegen_example_generated.c `pkg-config --libs --cflags ecore-evas edje`
 * gcc -o edje-codegen-example codegen_example_generated.o \
 * edje-codegen-example.c `pkg-config --libs --cflags ecore-evas edje`
 * @endverbatim
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#else
# define __UNUSED__
#endif

#include "codegen_example_generated.h"

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Edje.h>

#define WIDTH  (800)
#define HEIGHT (800)

static void
_on_delete(Ecore_Evas *ee)
{
   ecore_main_loop_quit();
}

static void
_columns_rows_print(Evas_Object *edje_obj)
{
   int cols, rows;

   if (codegen_example_part_four_col_row_size_get(edje_obj, &cols, &rows))
     fprintf(stdout, "Number of columns: %d\nNumber of rows: %d\n", cols, rows);
   else
     fprintf(stderr, "Cannot get the number of columns and rows\n");
}

static void
_on_mouse_over(void *data, Evas_Object *obj, const char *emission,
	       const char *source)
{
   Evas_Object *rect;
   Eina_Bool disabled;
   static int i = 0;

   printf("Mouse over, source: %s - emission: %s\n",
	  source, emission);
   if (i == 2)
     {
	rect = codegen_example_part_three_remove_at(obj, 0);
	codegen_example_part_three_append(obj, rect);
     }
   if (i++ == 5)
     {
	Evas_Object *rect = codegen_example_part_two_get(obj);
	evas_object_color_set(rect, 0, 255, 0, 255);
	codegen_example_part_below_over_callback_del_full(obj,_on_mouse_over,
							  NULL);
	codegen_example_part_four_clear(obj, EINA_TRUE);
	_columns_rows_print(obj);
	codegen_example_part_three_remove_all(obj, EINA_TRUE);
	part_five_part_five_disabled_set(obj, EINA_TRUE);

	if (part_five_part_five_disabled_get(obj, &disabled))
	  if (disabled)
	    fprintf(stdout, "Button is disabled\n");
	  else
	    fprintf(stdout, "Button is enabled\n");
	else
	  fprintf(stderr, "Couldn't check if the button is disabled\n");
     }
}

static Evas_Object *
_rect_create(Evas *e, unsigned char r, unsigned char g, unsigned char b)
{
   Evas_Object *o;

   o = evas_object_rectangle_add(e);
   evas_object_color_set(o, r, g, b, 255);
   evas_object_size_hint_weight_set(o, 1.0, 1.0);
   evas_object_size_hint_min_set(o, 100, 100);
   evas_object_show(o);

   return o;
}

int
main(int argc, char *argv[])
{
   char         edje_file_path[PATH_MAX];
   const char  *edje_file = "codegen.edj", *aux;
   Ecore_Evas  *ee;
   Evas        *evas;
   Evas_Object *bg;
   Evas_Object *edje_obj;
   Evas_Object *red_rect, *yellow_rect, *blue_rect, *rects[4];
   Eina_Prefix *pfx;
   Eina_Bool    disabled;

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

   snprintf(edje_file_path, sizeof(edje_file_path),
            "%s/examples/%s", eina_prefix_data_get(pfx), edje_file);

   /* this will give you a window with an Evas canvas under the first
    * engine available */
   ee = ecore_evas_new(NULL, 0, 0, WIDTH, HEIGHT, NULL);
   if (!ee)
     goto free_prefix;

   ecore_evas_callback_delete_request_set(ee, _on_delete);
   ecore_evas_title_set(ee, "Edje codegen Example");

   evas = ecore_evas_get(ee);

   bg = _rect_create(evas, 255, 255, 255);
   evas_object_move(bg, 0, 0); /* at canvas' origin */
   evas_object_resize(bg, WIDTH, HEIGHT); /* covers full canvas */
   ecore_evas_object_associate(ee, bg, ECORE_EVAS_OBJECT_ASSOCIATE_BASE);

   edje_obj = codegen_example_object_add(evas, edje_file_path);
   evas_object_resize(edje_obj, WIDTH, HEIGHT);
   evas_object_show(edje_obj);

   codegen_example_part_one_set(edje_obj, "CODEGEN_EXAMPLE");
   codegen_example_part_below_over_callback_add(edje_obj, _on_mouse_over, NULL);

   red_rect = _rect_create(evas, 255, 0, 0);
   codegen_example_part_two_set(edje_obj, red_rect);

   blue_rect = _rect_create(evas, 0, 0, 255);
   codegen_example_part_three_append(edje_obj, blue_rect);

   yellow_rect = _rect_create(evas, 255, 255, 0);
   codegen_example_part_three_prepend(edje_obj, yellow_rect);


   rects[0] = _rect_create(evas, 0, 0, 255);
   rects[1] = _rect_create(evas, 0, 255, 0);
   rects[2] = _rect_create(evas, 255, 0, 0);
   rects[3] = _rect_create(evas, 125, 140, 80);

   if (!codegen_example_part_four_pack(edje_obj, rects[0], 0, 0, 1, 2))
     fprintf(stderr, "Cannot add the rectangle 1 to table\n");

   if (!codegen_example_part_four_pack(edje_obj, rects[1], 0, 1, 1, 1))
     fprintf(stderr, "Cannot add the rectangle 2 to table\n");

   if (!codegen_example_part_four_pack(edje_obj, rects[2], 1, 0, 1, 1))
     fprintf(stderr, "Cannot add the rectangle 3 to table\n");

   if (!codegen_example_part_four_pack(edje_obj, rects[3], 1, 1, 1, 1))
     fprintf(stderr, "Cannot add the rectangle 4 to table\n");

   if (!part_five_part_five_label_set(edje_obj, "new label"))
     fprintf(stderr, "Cannot set the label of the button\n");

   if (part_five_part_five_label_get(edje_obj, &aux))
     fprintf(stdout, "Label contents: %s\n", aux);
   else
     fprintf(stderr, "Couldn't get the label contents\n");

   if (part_five_part_five_disabled_get(edje_obj, &disabled))
     if (disabled)
       fprintf(stdout, "Button is disabled\n");
     else
       fprintf(stdout, "Button is enabled\n");
   else
     fprintf(stderr, "Couldn't check if the button is disabled\n");

   _columns_rows_print(edje_obj);
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