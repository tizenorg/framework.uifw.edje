/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include <time.h>

#include "edje_private.h"

static int _edje_init_count = 0;
int _edje_default_log_dom = -1;
Eina_Mempool *_edje_real_part_mp = NULL;
Eina_Mempool *_edje_real_part_state_mp = NULL;



/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @addtogroup Edje_main_Group Main
 *
 * @brief These functions provide an abstraction layer between the
 * application code and the interface, while allowing extremely
 * flexible dynamic layouts and animations.
 *
 * @{
 */

/**
 * @brief Initialize the edje library.
 *
 * @return The new init count. The initial value is zero.
 *
 * This function initializes the ejde library, making the propers
 * calls to initialization functions. It makes calls to functions
 * eina_init(), ecore_init(), embryo_init() and eet_init() so
 * there is no need to call those functions again in your code. To
 * shutdown edje there is a function edje_shutdown().
 *
 * @see edje_shutdown()
 * @see eina_init()
 * @see ecore_init()
 * @see embryo_init()
 * @see eet_init()
 *
 */

EAPI int
edje_init(void)
{
   if (++_edje_init_count != 1)
     return _edje_init_count;

   srand(time(NULL));

   if (!eina_init())
     {
	fprintf(stderr, "Edje: Eina init failed");
	return --_edje_init_count;
     }

   _edje_default_log_dom = eina_log_domain_register("Edje", EDJE_DEFAULT_LOG_COLOR);
   if (_edje_default_log_dom < 0)
     {
	EINA_LOG_ERR("Edje Can not create a general log domain.");
	goto shutdown_eina;
     }

   if (!ecore_init())
     {
	ERR("Edje: Ecore init failed");
	goto unregister_log_domain;
     }

   if (!embryo_init())
     {
	ERR("Edje: Embryo init failed");
	goto shutdown_ecore;
     }

   if (!eet_init())
     {
	ERR("Edje: Eet init failed");
	goto shutdown_embryo;
     }

   _edje_scale = FROM_DOUBLE(1.0);

   _edje_edd_init();
   _edje_text_init();
   _edje_box_init();
   _edje_external_init();
   _edje_module_init();
   _edje_lua_init();
   _edje_message_init();

   _edje_real_part_mp = eina_mempool_add("chained_mempool",
					 "Edje_Real_Part", NULL,
					 sizeof (Edje_Real_Part), 128);
   if (!_edje_real_part_mp)
     {
	ERR("ERROR: Mempool for Edje_Real_Part cannot be allocated.\n");
	goto shutdown_eet;
     }

   _edje_real_part_state_mp = eina_mempool_add("chained_mempool",
					       "Edje_Real_Part_State", NULL,
					       sizeof (Edje_Real_Part_State), 256);
   if (!_edje_real_part_state_mp)
     {
	ERR("ERROR: Mempool for Edje_Real_Part_State cannot be allocated.\n");
	goto shutdown_eet;
     }

   return _edje_init_count;

 shutdown_eet:
   eina_mempool_del(_edje_real_part_state_mp);
   eina_mempool_del(_edje_real_part_mp);
   _edje_real_part_state_mp = NULL;
   _edje_real_part_mp = NULL;
   _edje_message_shutdown();
   _edje_lua_shutdown();
   _edje_module_shutdown();
   _edje_external_shutdown();
   _edje_box_shutdown();
   _edje_text_class_members_free();
   _edje_text_class_hash_free();
   _edje_edd_shutdown();
   eet_shutdown();
 shutdown_embryo:
   embryo_shutdown();
 shutdown_ecore:
   ecore_shutdown();
 unregister_log_domain:
   eina_log_domain_unregister(_edje_default_log_dom);
   _edje_default_log_dom = -1;
 shutdown_eina:
   eina_shutdown();
   return --_edje_init_count;
}

/**
 * @brief Shutdown the edje library.
 *
 * @return The number of times the library has been initialised without being
 *         shutdown.
 *
 * This function shuts down the edje library. It calls the functions
 * eina_shutdown(), ecore_shutdown(), embryo_shutdown() and
 * eet_shutdown(), so there is no need to call these functions again
 * in your code.
 *
 * @see edje_init()
 * @see eina_shutdown()
 * @see ecore_shutdown()
 * @see embryo_shutdown()
 * @see eet_shutdown()
 *
 */

EAPI int
edje_shutdown(void)
{
   if (--_edje_init_count != 0)
     return _edje_init_count;

   if (_edje_timer)
     ecore_animator_del(_edje_timer);
   _edje_timer = NULL;

   _edje_file_cache_shutdown();
   _edje_color_class_members_free();
   _edje_color_class_hash_free();

   eina_mempool_del(_edje_real_part_state_mp);
   eina_mempool_del(_edje_real_part_mp);
   _edje_real_part_state_mp = NULL;
   _edje_real_part_mp = NULL;

   _edje_message_shutdown();
   _edje_lua_shutdown();
   _edje_module_shutdown();
   _edje_external_shutdown();
   _edje_box_shutdown();
   _edje_text_class_members_free();
   _edje_text_class_hash_free();
   _edje_edd_shutdown();

   eet_shutdown();
   embryo_shutdown();
   ecore_shutdown();
   eina_log_domain_unregister(_edje_default_log_dom);
   _edje_default_log_dom = -1;
   eina_shutdown();

   return _edje_init_count;
}

/* Private Routines */

Edje *
_edje_add(Evas_Object *obj)
{
   Edje *ed;

   ed = calloc(1, sizeof(Edje));
   if (!ed) return NULL;
   ed->evas = evas_object_evas_get(obj);
   ed->clipper = evas_object_rectangle_add(ed->evas);
   evas_object_smart_member_add(ed->clipper, obj);
   evas_object_color_set(ed->clipper, 255, 255, 255, 255);
   evas_object_move(ed->clipper, -10000, -10000);
   evas_object_resize(ed->clipper, 20000, 20000);
   evas_object_pass_events_set(ed->clipper, 1);
   ed->have_objects = 1;
   ed->references = 1;
   return ed;
}

void
_edje_del(Edje *ed)
{
   if (ed->processing_messages)
     {
	ed->delete_me = 1;
	return;
     }
   _edje_message_del(ed);
   _edje_callbacks_patterns_clean(ed);
   _edje_file_del(ed);
   if (ed->path) eina_stringshare_del(ed->path);
   if (ed->group) eina_stringshare_del(ed->group);
   if (ed->parent) eina_stringshare_del(ed->parent);
   ed->path = NULL;
   ed->group = NULL;
   if ((ed->actions) || (ed->pending_actions))
     {
	_edje_animators = eina_list_remove(_edje_animators, ed);
     }
   while (ed->actions)
     {
	Edje_Running_Program *runp;

	runp = eina_list_data_get(ed->actions);
	ed->actions = eina_list_remove(ed->actions, runp);
	free(runp);
     }
   while (ed->pending_actions)
     {
	Edje_Pending_Program *pp;

	pp = eina_list_data_get(ed->pending_actions);
	ed->pending_actions = eina_list_remove(ed->pending_actions, pp);
	free(pp);
     }
   while (ed->callbacks)
     {
	Edje_Signal_Callback *escb;

	escb = eina_list_data_get(ed->callbacks);
	ed->callbacks = eina_list_remove(ed->callbacks, escb);
	if (escb->signal) eina_stringshare_del(escb->signal);
	if (escb->source) eina_stringshare_del(escb->source);
	free(escb);
     }
   while (ed->color_classes)
     {
	Edje_Color_Class *cc;

	cc = eina_list_data_get(ed->color_classes);
	ed->color_classes = eina_list_remove(ed->color_classes, cc);
	if (cc->name) eina_stringshare_del(cc->name);
	free(cc);
     }
   while (ed->text_classes)
     {
	Edje_Text_Class *tc;

	tc = eina_list_data_get(ed->text_classes);
	ed->text_classes = eina_list_remove(ed->text_classes, tc);
	if (tc->name) eina_stringshare_del(tc->name);
	if (tc->font) eina_stringshare_del(tc->font);
	free(tc);
     }
   free(ed);
}

void
_edje_clean_objects(Edje *ed)
{
   evas_object_del(ed->clipper);
   ed->evas = NULL;
   ed->obj = NULL;
   ed->clipper = NULL;
}

void
_edje_ref(Edje *ed)
{
   if (ed->references <= 0) return;
   ed->references++;
}

void
_edje_unref(Edje *ed)
{
   ed->references--;
   if (ed->references == 0) _edje_del(ed);
}

/**
 *
 * @}
 */
