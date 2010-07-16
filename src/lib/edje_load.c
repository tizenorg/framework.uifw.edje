/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include <string.h>

#include "edje_private.h"

#ifdef EDJE_PROGRAM_CACHE
static Eina_Bool  _edje_collection_free_prog_cache_matches_free_cb(const Eina_Hash *hash, const void *key, void *data, void *fdata);
#endif
static void _edje_object_pack_item_hints_set(Evas_Object *obj, Edje_Pack_Element *it);
static void _cb_signal_repeat(void *data, Evas_Object *obj, const char *signal, const char *source);

static Eina_List *_edje_swallows_collect(Edje *ed);

/************************** API Routines **************************/

/* FIXDOC: Verify/expand doc */
/** Sets the EET file and group to load @a obj from
 * @param obj A valid Evas_Object handle
 * @param file The path to the EET file
 * @param group The group name in the Edje
 * @return 0 on Error\n
 * 1 on Success and sets EDJE_LOAD_ERROR_NONE
 *
 * Edje uses EET files, conventionally ending in .edj, to store object
 * descriptions. A single file contains multiple named groups. This function
 * specifies the file and group name to load @a obj from.
 */
EAPI Eina_Bool
edje_object_file_set(Evas_Object *obj, const char *file, const char *group)
{
   Edje *ed;

   ed = _edje_fetch(obj);
   if (!ed)
     return EINA_FALSE;
   return ed->api->file_set(obj, file, group);
}

/* FIXDOC: Verify/expand doc. */
/** Get the file and group name that @a obj was loaded from
 * @param obj A valid Evas_Object handle
 * @param file A pointer to store a pointer to the filename in
 * @param group A pointer to store a pointer to the group name in
 *
 * This gets the EET file location and group for the given Evas_Object.
 * If @a obj is either not an edje file, or has not had its file/group set
 * using edje_object_file_set(), then both @a file and @a group will be set
 * to NULL.
 *
 * It is valid to pass in NULL for either @a file or @a group if you are not
 * interested in one of the values.
 */
EAPI void
edje_object_file_get(const Evas_Object *obj, const char **file, const char **group)
{
   Edje *ed;

   ed = _edje_fetch(obj);
   if (!ed)
     {
	if (file) *file = NULL;
	if (group) *group = NULL;
	return;
     }
   if (file) *file = ed->path;
   if (group) *group = ed->group;
}

/* FIXDOC: Verify. return error? */
/** Gets the Edje load error
 * @param obj A valid Evas_Object handle
 *
 * @return The Edje load error:\n
 * EDJE_LOAD_ERROR_NONE: No Error\n
 * EDJE_LOAD_ERROR_GENERIC: Generic Error\n
 * EDJE_LOAD_ERROR_DOES_NOT_EXIST: Does not Exist\n
 * EDJE_LOAD_ERROR_PERMISSION_DENIED: Permission Denied\n
 * EDJE_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED: Resource Allocation Failed\n
 * EDJE_LOAD_ERROR_CORRUPT_FILE: Corrupt File\n
 * EDJE_LOAD_ERROR_UNKNOWN_FORMAT: Unknown Format\n
 * EDJE_LOAD_ERROR_INCOMPATIBLE_FILE: Incompatible File\n
 * EDJE_LOAD_ERROR_UNKNOWN_COLLECTION: Unknown Collection\n
 * EDJE_LOAD_ERROR_RECURSIVE_REFERENCE: Recursive Reference\n
 */
EAPI int
edje_object_load_error_get(const Evas_Object *obj)
{
   Edje *ed;

   ed = _edje_fetch(obj);
   if (!ed) return EDJE_LOAD_ERROR_NONE;
   return ed->load_error;
}

EAPI const char *
edje_load_error_str(int error)
{
   switch (error)
     {
      case EDJE_LOAD_ERROR_NONE:
	 return "No Error";
      case EDJE_LOAD_ERROR_GENERIC:
	 return "Generic Error";
      case EDJE_LOAD_ERROR_DOES_NOT_EXIST:
	 return "File Does Not Exist";
      case EDJE_LOAD_ERROR_PERMISSION_DENIED:
	 return "Permission Denied";
      case EDJE_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED:
	 return "Resource Allocation Failed";
      case EDJE_LOAD_ERROR_CORRUPT_FILE:
	 return "Corrupt File";
      case EDJE_LOAD_ERROR_UNKNOWN_FORMAT:
	 return "Unknown Format";
      case EDJE_LOAD_ERROR_INCOMPATIBLE_FILE:
	 return "Incompatible File";
      case EDJE_LOAD_ERROR_UNKNOWN_COLLECTION:
	 return "Unknown Collection";
      case EDJE_LOAD_ERROR_RECURSIVE_REFERENCE:
	 return "Recursive Reference";
      default:
	 return "Unknown Error";
     }
}


/** Get a list of groups in an edje file
 * @param file The path to the edje file
 *
 * @return The Eina_List of group names (char *)
 *
 * Note: the list must be freed using edje_file_collection_list_free()
 * when you are done with it.
 */
EAPI Eina_List *
edje_file_collection_list(const char *file)
{
   Eina_List *lst = NULL;
   Edje_File *edf;
   int error_ret = 0;

   if ((!file) || (!*file)) return NULL;
   edf = _edje_cache_file_coll_open(file, NULL, &error_ret, NULL);
   if (edf != NULL)
     {
	Eina_Iterator *i;
	const char *key;

	i = eina_hash_iterator_key_new(edf->collection);

	EINA_ITERATOR_FOREACH(i, key)
	  lst = eina_list_append(lst, eina_stringshare_add(key));

	eina_iterator_free(i);

	_edje_cache_file_unref(edf);
     }
   return lst;
}

/** Free file collection list
 * @param lst The Eina_List of groups
 *
 * Frees the list returned by edje_file_collection_list().
 */
EAPI void
edje_file_collection_list_free(Eina_List *lst)
{
   while (lst)
     {
        if (eina_list_data_get(lst)) eina_stringshare_del(eina_list_data_get(lst));
	lst = eina_list_remove(lst, eina_list_data_get(lst));
     }
}

/** Determine whether a group matching glob exists in an edje file.
 * @param file The file path
 * @param glob A glob to match on
 *
 * @return 1 if a match is found, 0 otherwise
 */
EAPI Eina_Bool
edje_file_group_exists(const char *file, const char *glob)
{
   Edje_File *edf;
   int error_ret = 0;
   Eina_Bool succeed = EINA_FALSE;

   if ((!file) || (!*file)) return EINA_FALSE;
   edf = _edje_cache_file_coll_open(file, NULL, &error_ret, NULL);
   if (edf != NULL)
     {
	Edje_Patterns *patterns;

	if (edf->collection_patterns)
	  {
	     patterns = edf->collection_patterns;
	  }
	else
	  {
	     Edje_Part_Collection_Directory_Entry *ce;
	     Eina_Iterator *i;
	     Eina_List *l = NULL;

	     i = eina_hash_iterator_data_new(edf->collection);

	     EINA_ITERATOR_FOREACH(i, ce)
	       l = eina_list_append(l, ce);

	     eina_iterator_free(i);

	     patterns = edje_match_collection_dir_init(l);
	     eina_list_free(l);
	  }

	succeed = edje_match_collection_dir_exec(patterns, glob);

	edf->collection_patterns = patterns;

	_edje_cache_file_unref(edf);
     }
   return succeed;
}


/** Get data from the file level data block of an edje file
 * @param file The path to the .edj file
 * @param key The data key
 * @return The string value of the data
 *
 * If an edje file is built from the following edc:
 *
 * data {
 *   item: "key1" "value1";
 *   item: "key2" "value2";
 * }
 * collections { ... }
 *
 * Then, edje_file_data_get("key1") will return "value1"
 */
EAPI char *
edje_file_data_get(const char *file, const char *key)
{
   Edje_File *edf;
   char *str = NULL;
   int error_ret = 0;

   if (key)
     {
	edf = _edje_cache_file_coll_open(file, NULL, &error_ret, NULL);
	if (edf != NULL)
	  {
	     str = eina_hash_find(edf->data, key);

	     if (str) str = strdup(str);

	     _edje_cache_file_unref(edf);
	  }
     }
   return str;
}

void
_edje_programs_patterns_clean(Edje *ed)
{
   _edje_signals_sources_patterns_clean(&ed->patterns.programs);

   eina_rbtree_delete(ed->patterns.programs.exact_match,
		      EINA_RBTREE_FREE_CB(edje_match_signal_source_free),
		      NULL);
   ed->patterns.programs.exact_match = NULL;

   ed->patterns.programs.globing = eina_list_free(ed->patterns.programs.globing);
}

void
_edje_programs_patterns_init(Edje *ed)
{
   Edje_Signals_Sources_Patterns *ssp = &ed->patterns.programs;

   if (ssp->signals_patterns)
     return;

   ssp->globing = edje_match_program_hash_build(ed->collection->programs,
							  &ssp->exact_match);

   ssp->signals_patterns = edje_match_programs_signal_init(ssp->globing);
   ssp->sources_patterns = edje_match_programs_source_init(ssp->globing);
}

int
_edje_object_file_set_internal(Evas_Object *obj, const char *file, const char *group, Eina_List *group_path)
{
   Edje *ed;
   int n;
   Eina_List *parts = NULL;
   Eina_List *old_swallows;
   int group_path_started = 0;

   ed = _edje_fetch(obj);
   if (!ed) return 0;
   if (!file) file = "";
   if (!group) group = "";
   if (((ed->path) && (!strcmp(file, ed->path))) &&
	(ed->group) && (!strcmp(group, ed->group)))
     return 1;

   old_swallows = _edje_swallows_collect(ed);

   if (_edje_script_only(ed)) _edje_script_only_shutdown(ed);
   if (_edje_lua_script_only(ed)) _edje_lua_script_only_shutdown(ed);
   _edje_file_del(ed);

   if (ed->path) eina_stringshare_del(ed->path);
   if (ed->group) eina_stringshare_del(ed->group);
   ed->path = eina_stringshare_add(file);
   ed->group = eina_stringshare_add(group);

   ed->load_error = EDJE_LOAD_ERROR_NONE;
   _edje_file_add(ed);

   if (ed->file && ed->file->external_dir)
     {
	unsigned int i;

	for (i = 0; i < ed->file->external_dir->entries_count; ++i)
	  edje_module_load(ed->file->external_dir->entries[i].entry);
     }

   _edje_textblock_styles_add(ed);
   _edje_textblock_style_all_update(ed);

   ed->has_entries = EINA_FALSE;

   if (ed->collection)
     {
	if (ed->collection->script_only)
	  {
	     ed->load_error = EDJE_LOAD_ERROR_NONE;
	     _edje_script_only_init(ed);
	  }
	else if (ed->collection->lua_script_only)
	  {
	     ed->load_error = EDJE_LOAD_ERROR_NONE;
	     _edje_lua_script_only_init(ed);
	  }
	else
	  {
	     Eina_List *l;
	     int i;
	     int errors = 0;
	     Edje_Part *ep;

	     /* colorclass stuff */
	     EINA_LIST_FOREACH(ed->collection->parts, l, ep)
	       {
		  Eina_List *hist = NULL;
		  Edje_Part_Description *desc;

		  if (errors)
		    break;
		  /* Register any color classes in this parts descriptions. */
		  if ((ep->default_desc) && (ep->default_desc->color_class))
		    _edje_color_class_member_add(ed, ep->default_desc->color_class);

		  EINA_LIST_FOREACH(ep->other_desc, hist, desc)
		    if (desc->color_class)
		      _edje_color_class_member_add(ed, desc->color_class);
	       }
	     /* build real parts */
	     for (n = 0, l = ed->collection->parts; l; l = eina_list_next(l), n++)
	       {
		  Edje_Part *ep;
		  Edje_Real_Part *rp;

		  ep = eina_list_data_get(l);
		  rp = eina_mempool_malloc(_edje_real_part_mp, sizeof(Edje_Real_Part));
		  if (!rp)
		    {
		       ed->load_error = EDJE_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
		       return 0;
		    }

		  memset(rp, 0, sizeof (Edje_Real_Part));

		  if ((ep->dragable.x != 0) || (ep->dragable.y != 0))
		    {
		       rp->drag = calloc(1, sizeof (Edje_Real_Part_Drag));
		       if (!rp->drag)
			 {
			    ed->load_error = EDJE_LOAD_ERROR_RESOURCE_ALLOCATION_FAILED;
			    free(rp);
			    return 0;
			 }

		       rp->drag->step.x = FROM_INT(ep->dragable.step_x);
		       rp->drag->step.y = FROM_INT(ep->dragable.step_y);
		    }

		  rp->edje = ed;
		  _edje_ref(rp->edje);
		  rp->part = ep;
		  parts = eina_list_append(parts, rp);
		  rp->param1.description = ep->default_desc;
		  rp->chosen_description = rp->param1.description;
		  if (!rp->param1.description)
		    ERR("no default part description!");

		  switch (ep->type)
		    {
		     case EDJE_PART_TYPE_RECTANGLE:
			rp->object = evas_object_rectangle_add(ed->evas);
			break;
		     case EDJE_PART_TYPE_IMAGE:
			rp->object = evas_object_image_add(ed->evas);
			break;
		     case EDJE_PART_TYPE_TEXT:
			_edje_text_part_on_add(ed, rp);
			rp->object = evas_object_text_add(ed->evas);
			evas_object_text_font_source_set(rp->object, ed->path);
			break;
		     case EDJE_PART_TYPE_SWALLOW:
		     case EDJE_PART_TYPE_GROUP:
		     case EDJE_PART_TYPE_EXTERNAL:
			rp->object = evas_object_rectangle_add(ed->evas);
			evas_object_color_set(rp->object, 0, 0, 0, 0);
			evas_object_pass_events_set(rp->object, 1);
			evas_object_pointer_mode_set(rp->object, EVAS_OBJECT_POINTER_MODE_NOGRAB);
			_edje_callbacks_focus_add(rp->object, ed, rp);
			break;
		     case EDJE_PART_TYPE_TEXTBLOCK:
			rp->object = evas_object_textblock_add(ed->evas);
			break;
		     case EDJE_PART_TYPE_BOX:
			rp->object = evas_object_box_add(ed->evas);
			break;
		     case EDJE_PART_TYPE_TABLE:
			rp->object = evas_object_table_add(ed->evas);
			break;
		     case EDJE_PART_TYPE_GRADIENT:
			ERR("SPANK ! SPANK ! SPANK !\nYOU ARE USING GRADIENT IN PART %s FROM GROUP %s INSIDE FILE %s !!\n THEY ARE NOW REMOVED !",
			    ep->name, group, file);
		     default:
			ERR("wrong part type %i!", ep->type);
			break;
		    }

		  if (rp->object)
		    {
		       evas_object_smart_member_add(rp->object, ed->obj);
//		       evas_object_layer_set(rp->object, evas_object_layer_get(ed->obj));
		       if (ep->type != EDJE_PART_TYPE_SWALLOW && ep->type != EDJE_PART_TYPE_GROUP && ep->type != EDJE_PART_TYPE_EXTERNAL)
			 {
			    if (ep->mouse_events)
			      {
				 _edje_callbacks_add(rp->object, ed, rp);
				 if (ep->repeat_events)
				   evas_object_repeat_events_set(rp->object, 1);
				 
				 if (ep->pointer_mode != EVAS_OBJECT_POINTER_MODE_AUTOGRAB)
				   evas_object_pointer_mode_set(rp->object, ep->pointer_mode);
			      }
			    else
			      {
				 evas_object_pass_events_set(rp->object, 1);
				 evas_object_pointer_mode_set(rp->object, EVAS_OBJECT_POINTER_MODE_NOGRAB);
			      }
			    if (ep->precise_is_inside)
			      evas_object_precise_is_inside_set(rp->object, 1);
			 }
		       if (rp->part->clip_to_id < 0)
			 evas_object_clip_set(rp->object, ed->clipper);
		    }
	       }
	     if (n > 0)
	       {
		  Edje_Real_Part *rp;
		  ed->table_parts = malloc(sizeof(Edje_Real_Part *) * n);
		  ed->table_parts_size = n;
		  /* FIXME: check malloc return */
		  n = 0;
		  EINA_LIST_FOREACH(parts, l, rp)
		    {
		       ed->table_parts[n] = rp;
		       n++;
		    }
		  eina_list_free(parts);
		  for (i = 0; i < ed->table_parts_size; i++)
		    {
		       rp = ed->table_parts[i];
		       if (rp->param1.description->common.rel1.id_x >= 0)
			 rp->param1.rel1_to_x = ed->table_parts[rp->param1.description->common.rel1.id_x % ed->table_parts_size];
		       if (rp->param1.description->common.rel1.id_y >= 0)
			 rp->param1.rel1_to_y = ed->table_parts[rp->param1.description->common.rel1.id_y % ed->table_parts_size];
		       if (rp->param1.description->common.rel2.id_x >= 0)
			 rp->param1.rel2_to_x = ed->table_parts[rp->param1.description->common.rel2.id_x % ed->table_parts_size];
		       if (rp->param1.description->common.rel2.id_y >= 0)
			 rp->param1.rel2_to_y = ed->table_parts[rp->param1.description->common.rel2.id_y % ed->table_parts_size];
		       if (rp->part->clip_to_id >= 0)
			 {
			    rp->clip_to = ed->table_parts[rp->part->clip_to_id % ed->table_parts_size];
			    if (rp->clip_to)
			      {
				 evas_object_pass_events_set(rp->clip_to->object, 1);
				 evas_object_pointer_mode_set(rp->clip_to->object, EVAS_OBJECT_POINTER_MODE_NOGRAB);
				 evas_object_clip_set(rp->object, rp->clip_to->object);
			      }
			 }
		       if (rp->drag)
			 {
			    if (rp->part->dragable.confine_id >= 0)
			      rp->drag->confine_to = ed->table_parts[rp->part->dragable.confine_id % ed->table_parts_size];
			 }

		       /* replay events for dragable */
		       if (rp->part->dragable.event_id >= 0)
			 {
			    rp->events_to =
			      ed->table_parts[rp->part->dragable.event_id % ed->table_parts_size];
			    /* events_to may be used only with dragable */
			    if (!rp->events_to->part->dragable.x &&
				!rp->events_to->part->dragable.y)
			      rp->events_to = NULL;
			 }

		       rp->swallow_params.min.w = 0;
		       rp->swallow_params.min.w = 0;
		       rp->swallow_params.max.w = -1;
		       rp->swallow_params.max.h = -1;
		       
		       if (ed->file->feature_ver < 1)
			 {
			    rp->param1.description->text.id_source = -1;
			    rp->param1.description->text.id_text_source = -1;
			 }
		       if (rp->param1.description->text.id_source >= 0)
			 rp->text.source = ed->table_parts[rp->param1.description->text.id_source % ed->table_parts_size];
		       if (rp->param1.description->text.id_text_source >= 0)
			 rp->text.text_source = ed->table_parts[rp->param1.description->text.id_text_source % ed->table_parts_size];
		       if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
                         {
                            _edje_entry_real_part_init(rp);
                            if (!ed->has_entries)
                                ed->has_entries = EINA_TRUE;
                         }
		    }
	       }
	     
	     _edje_programs_patterns_init(ed);
	     
	     n = eina_list_count(ed->collection->programs);
	     if (n > 0)
	       {
		  Edje_Program *pr;
		  /* FIXME: keeping a table AND a list is just bad - nuke list */
		  ed->table_programs = malloc(sizeof(Edje_Program *) * n);
		  ed->table_programs_size = n;
		  /* FIXME: check malloc return */
		  n = 0;
		  EINA_LIST_FOREACH(ed->collection->programs, l, pr)
		    {
		       ed->table_programs[n] = pr;
		       n++;
		    }
	       }
	     _edje_ref(ed);
	     _edje_block(ed);
	     _edje_freeze(ed);
//	     if (ed->collection->script) _edje_embryo_script_init(ed);
	     _edje_var_init(ed);
	     for (i = 0; i < ed->table_parts_size; i++)
	       {
		  Edje_Real_Part *rp;
		  
		  rp = ed->table_parts[i];
		  evas_object_show(rp->object);
		  if (_edje_block_break(ed)) break;
		  if (rp->drag)
		    {
		       if (rp->part->dragable.x < 0) rp->drag->val.x = FROM_DOUBLE(1.0);
		       if (rp->part->dragable.y < 0) rp->drag->val.x = FROM_DOUBLE(1.0);
		       _edje_dragable_pos_set(ed, rp, rp->drag->val.x, rp->drag->val.y);
		    }
	       }
	     ed->dirty = 1;
#ifdef EDJE_CALC_CACHE
	     ed->all_part_change = 1;
#endif
	     if ((evas_object_clipees_get(ed->clipper)) &&
		 (evas_object_visible_get(obj)))
	       evas_object_show(ed->clipper);
	     
	     /* instantiate 'internal swallows' */
	     for (i = 0; i < ed->table_parts_size; i++)
	       {
		  Edje_Real_Part *rp;
		  /* XXX: curr_item and pack_it don't require to be NULL since
		   * XXX: they are just used when source != NULL and type == BOX,
		   * XXX: and they're always set in this case, but GCC fails to
		   * XXX: notice that, so let's shut it up
		   */
		  Eina_List *curr_item = NULL;
		  Edje_Pack_Element *pack_it = NULL;
		  const char *source = NULL;
		  
		  rp = ed->table_parts[i];

		  switch (rp->part->type)
		    {
		     case EDJE_PART_TYPE_GROUP:
			source = rp->part->source;
			break;
		     case EDJE_PART_TYPE_BOX:
		     case EDJE_PART_TYPE_TABLE:
			if (rp->part->items)
			  {
			     curr_item = rp->part->items;
			     pack_it = curr_item->data;
			     source = pack_it->source;
			  }
			break;
		     case EDJE_PART_TYPE_EXTERNAL:
			  {
			     Evas_Object *child_obj;
			     child_obj = _edje_external_type_add(rp->part->source, evas_object_evas_get(ed->obj), ed->obj, rp->part->default_desc->external_params, rp->part->name);
			     if (child_obj)
			       {
				  _edje_real_part_swallow(rp, child_obj);
				  rp->param1.external_params = _edje_external_params_parse(child_obj, rp->param1.description->external_params);
				  _edje_external_recalc_apply(ed, rp, NULL, rp->chosen_description);
			       }
			  }
			continue;
		     default:
			continue;
		    }

		  while (source)
		    {
		       Eina_List *l;
		       Evas_Object *child_obj;
		       Edje *child_ed;
		       const char *group_path_entry = eina_stringshare_add(source);
		       const char *data;

		       if (!group_path)
			 {
			    group_path = eina_list_append(NULL, eina_stringshare_add(group));
			    group_path_started = 1;
			 }
		       /* make sure that this group isn't already in the tree of parents */
		       EINA_LIST_FOREACH(group_path, l, data)
			 {
			    if (data == group_path_entry)
			      {
				 _edje_thaw(ed);
				 _edje_unblock(ed);
				 _edje_unref(ed);
				 _edje_file_del(ed);
				 eina_stringshare_del(group_path_entry);
				 if (group_path_started)
				   {
				      eina_stringshare_del(eina_list_data_get(group_path));
				      eina_list_free(group_path);
				   }
				 ed->load_error = EDJE_LOAD_ERROR_RECURSIVE_REFERENCE;
				 return 0;
			      }
			 }
		       
		       child_obj = edje_object_add(ed->evas);
		       group_path = eina_list_append(group_path, group_path_entry);
		       if (!_edje_object_file_set_internal(child_obj, file, source, group_path))
			 {
			    _edje_thaw(ed);
			    _edje_unblock(ed);
			    _edje_unref(ed);
			    _edje_file_del(ed);
			    
			    if (group_path_started)
			      {
				 while (group_path)
				   {
				      eina_stringshare_del(eina_list_data_get(group_path));
				      group_path = eina_list_remove_list(group_path, group_path);
				   }
			      }
			    ed->load_error = edje_object_load_error_get(child_obj);
			    return 0;
			 }
		       child_ed = _edje_fetch(child_obj);
		       child_ed->parent = eina_stringshare_add(rp->part->name);
		       
		       group_path = eina_list_remove(group_path, group_path_entry);
		       eina_stringshare_del(group_path_entry);

		       edje_object_signal_callback_add(child_obj, "*", "*", _cb_signal_repeat, obj);
		       if (rp->part->type == EDJE_PART_TYPE_GROUP)
			 {
			    _edje_real_part_swallow(rp, child_obj);
			    source = NULL;
			 }
		       else
			 {
			    _edje_object_pack_item_hints_set(child_obj, pack_it);
			    evas_object_show(child_obj);
			    if (pack_it->name)
			      evas_object_name_set(child_obj, pack_it->name);
			    if (rp->part->type == EDJE_PART_TYPE_BOX)
			      {
				 _edje_real_part_box_append(rp, child_obj);
				 evas_object_data_set(child_obj, "\377 edje.box_item", pack_it);
			      }
			    else if(rp->part->type == EDJE_PART_TYPE_TABLE)
			      {
				 _edje_real_part_table_pack(rp, child_obj, pack_it->col, pack_it->row, pack_it->colspan, pack_it->rowspan);
				 evas_object_data_set(child_obj, "\377 edje.table_item", pack_it);
			      }
			    rp->items = eina_list_append(rp->items, child_obj);
			    if (!(curr_item = curr_item->next))
			      source = NULL;
			    else
			      {
				 pack_it = curr_item->data;
				 source = pack_it->source;
			      }
			 }
		    }
	       }
	     
	     if (group_path_started)
	       {
		  const char *str;

		  EINA_LIST_FREE(group_path, str)
		    eina_stringshare_del(str);
	       }
	     
	     /* reswallow any swallows that existed before setting the file */
	     if (old_swallows)
	       {
		  while (old_swallows)
		    {
		       const char *name;
		       Evas_Object *swallow;
		       
		       name = eina_list_data_get(old_swallows);
		       old_swallows = eina_list_remove_list(old_swallows, old_swallows);
		       
		       swallow = eina_list_data_get(old_swallows);
		       old_swallows = eina_list_remove_list(old_swallows, old_swallows);
		       
		       edje_object_part_swallow(obj, name, swallow);
		       eina_stringshare_del(name);
		    }
	       }
	     
	     _edje_recalc(ed);
	     _edje_thaw(ed);
	     _edje_unblock(ed);
	     _edje_unref(ed);
	     ed->load_error = EDJE_LOAD_ERROR_NONE;
	     _edje_emit(ed, "load", NULL);
	     /* instantiate 'internal swallows' */
	     for (i = 0; i < ed->table_parts_size; i++)
	       {
		  Edje_Real_Part *rp;
		  
		  rp = ed->table_parts[i];
                  if ((rp->part->type == EDJE_PART_TYPE_TEXTBLOCK) &&
                      (rp->part->default_desc))
                    {
                       Edje_Style *stl  = NULL;
                       const char *style;
                       
                       style = rp->part->default_desc->text.style;
                       if (style)
                         {
                            EINA_LIST_FOREACH(ed->file->styles, l, stl)
                              {
                                 if ((stl->name) && (!strcmp(stl->name, style))) break;
                                 stl = NULL;
                              }
                         }
                       if (stl)
                         {
                            if (evas_object_textblock_style_get(rp->object) != stl->style)
                              evas_object_textblock_style_set(rp->object, stl->style);
                         }
                    }
               }
	  }
        _edje_entry_init(ed);
	return 1;
     }
   else
     return 0;
   ed->load_error = EDJE_LOAD_ERROR_NONE;
   _edje_entry_init(ed);
   return 1;
}

void
_edje_file_add(Edje *ed)
{
   if (_edje_edd_edje_file == NULL) return;
   ed->file = _edje_cache_file_coll_open(ed->path, ed->group,
					 &(ed->load_error),
					 &(ed->collection));

   if (!ed->collection)
     {
	if (ed->file)
	  {
	     _edje_cache_file_unref(ed->file);
	     ed->file = NULL;
	  }
     }
}

static Eina_List *
_edje_swallows_collect(Edje *ed)
{
   Eina_List *swallows = NULL;
   int i;

   if (!ed->file || !ed->table_parts) return NULL;
   for (i = 0; i < ed->table_parts_size; i++)
     {
	Edje_Real_Part *rp;

	rp = ed->table_parts[i];
	if (rp->part->type != EDJE_PART_TYPE_SWALLOW || !rp->swallowed_object) continue;
	swallows = eina_list_append(swallows, eina_stringshare_add(rp->part->name));
	swallows = eina_list_append(swallows, rp->swallowed_object);
     }
   return swallows;
}

void
_edje_file_del(Edje *ed)
{
   if (ed->freeze_calc)
     {
        _edje_freeze_calc_list = eina_list_remove(_edje_freeze_calc_list, ed);
        ed->freeze_calc = 0;
        _edje_freeze_calc_count--;
     }
   _edje_entry_shutdown(ed);
   _edje_message_del(ed);
   _edje_block_violate(ed);
   _edje_var_shutdown(ed);
   _edje_programs_patterns_clean(ed);
//   if (ed->collection)
//     {
//        if (ed->collection->script) _edje_embryo_script_shutdown(ed);
//     }

   if (!((ed->file) && (ed->collection))) return;
   if (ed->table_parts)
     {
	int i;
	for (i = 0; i < ed->table_parts_size; i++)
	  {
	     Edje_Real_Part *rp;

	     rp = ed->table_parts[i];
	     if (rp->part->entry_mode > EDJE_ENTRY_EDIT_MODE_NONE)
	       _edje_entry_real_part_shutdown(rp);
	     if (rp->object)
	       {
		  _edje_callbacks_del(rp->object, ed);
		  _edje_callbacks_focus_del(rp->object, ed);
		  evas_object_del(rp->object);
	       }
	     if (rp->swallowed_object)
	       {
                  _edje_real_part_swallow_clear(rp);
                  /* Objects swallowed by the app do not get deleted,
                   but those internally swallowed (GROUP type) do. */
		  switch (rp->part->type)
		    {
		     case EDJE_PART_TYPE_EXTERNAL:
			_edje_external_parsed_params_free(rp->swallowed_object, rp->param1.external_params);
			if (rp->param2)
			  _edje_external_parsed_params_free(rp->swallowed_object, rp->param2->external_params);
		     case EDJE_PART_TYPE_GROUP:
			evas_object_del(rp->swallowed_object);
		     default:
			break;
		    }
		  rp->swallowed_object = NULL;
	       }
	     if (rp->items)
	       {
		  /* evas_box/table handles deletion of objects */
		  rp->items = eina_list_free(rp->items);
	       }
	     if (rp->text.text) eina_stringshare_del(rp->text.text);
	     if (rp->text.font) eina_stringshare_del(rp->text.font);
	     if (rp->text.cache.in_str) eina_stringshare_del(rp->text.cache.in_str);
	     if (rp->text.cache.out_str) eina_stringshare_del(rp->text.cache.out_str);

	     if (rp->custom)
               {
#ifdef LUA2
                  // xxx: lua2
#else
                  if (ed->L)
                    {
                       _edje_lua_get_reg(ed->L, rp->custom->description);
                       _edje_lua_free_reg(ed->L, lua_touserdata(ed->L, -1)); // created in edje_lua.c::_edje_lua_part_fn_custom_state
                       lua_pop(ed->L, 1);
                       _edje_lua_free_reg(ed->L, rp->custom->description); // created in edje_lua.c::_edje_lua_part_fn_custom_state
                    }
#endif
                  _edje_collection_free_part_description_free(rp->custom->description,
							      ed->file->free_strings);
               }

	     /* Cleanup optional part. */
	     free(rp->drag);
	     free(rp->param1.set);

	     if (rp->param2)
	       free(rp->param2->set);
	     eina_mempool_free(_edje_real_part_state_mp, rp->param2);

	     if (rp->custom)
	       free(rp->custom->set);
	     eina_mempool_free(_edje_real_part_state_mp, rp->custom);

	     _edje_unref(rp->edje);
	     eina_mempool_free(_edje_real_part_mp, rp);
	  }
     }
   if ((ed->file) && (ed->collection))
     {
	Eina_List *l;
	Edje_Part *ep;

	_edje_textblock_styles_del(ed);
	EINA_LIST_FOREACH(ed->collection->parts, l, ep)
	  {
	    _edje_text_part_on_del(ed, ep);
	    _edje_color_class_on_del(ed, ep);
	  }
	
	_edje_cache_coll_unref(ed->file, ed->collection);
	ed->collection = NULL;
     }
   if (ed->file)
     {
	_edje_cache_file_unref(ed->file);
	ed->file = NULL;
     }
   if (ed->actions)
     {
	Edje_Running_Program *runp;

	EINA_LIST_FREE(ed->actions, runp)
	  {
	     _edje_anim_count--;
	     free(runp);
	  }
     }
   _edje_animators = eina_list_remove(_edje_animators, ed);
   if (ed->pending_actions)
     {
	Edje_Pending_Program *pp;

	EINA_LIST_FREE(ed->pending_actions, pp)
	  {
	     ecore_timer_del(pp->timer);
	     free(pp);
	  }
     }
   if (ed->L)
     {
#ifdef LUA2
        _edje_lua2_script_shutdown(ed);
#else
	_edje_lua_free_reg(ed->L, ed); // created in edje_lua.c::_edje_lua_script_fn_new/_edje_lua_group_fn_new
	_edje_lua_free_reg(ed->L, ed->L); // created in edje_program.c::_edje_program_run/edje_lua_script_only.c::_edje_lua_script_only_init
	_edje_lua_free_thread(ed, ed->L); // created in edje_program.c::_edje_program_run/edje_lua_script_only.c::_edje_lua_script_only_init
	ed->L = NULL;
#endif   
     }
   if (ed->table_parts) free(ed->table_parts);
   ed->table_parts = NULL;
   ed->table_parts_size = 0;
   if (ed->table_programs) free(ed->table_programs);
   ed->table_programs = NULL;
   ed->table_programs_size = 0;
}

void
_edje_file_free(Edje_File *edf)
{
   Edje_Color_Class *ecc;
   const Edje_File *prev;

   prev = _edje_file_get();
   _edje_file_set(edf);

#define HASH_FREE(Hash)				\
   eina_hash_free(Hash);			\
   Hash = NULL;

   HASH_FREE(edf->fonts);
   HASH_FREE(edf->collection);
   HASH_FREE(edf->data);

   if (edf->oef)
     {
	if (edf->oef->font_dir)
	  {
	     eina_list_free(edf->oef->font_dir->entries);

	     free(edf->oef->font_dir);
	  }

	if (edf->oef->collection_dir)
	  {
	     eina_list_free(edf->oef->collection_dir->entries);

	     free(edf->oef->collection_dir);
	  }
     }

   if (edf->image_dir)
     {
	unsigned int i;

	if (edf->free_strings)
	  {
	     for (i = 0; i < edf->image_dir->entries_count; ++i)
	       eina_stringshare_del(edf->image_dir->entries[i].entry);
	  }

	/* Sets have been added after edje received eet dictionnary support */
	for (i = 0; i < edf->image_dir->sets_count; ++i)
	  {
	     Edje_Image_Directory_Set_Entry *se;

	     EINA_LIST_FREE(edf->image_dir->sets[i].entries, se)
	       free(se);

	  }

	free(edf->image_dir->entries);
	free(edf->image_dir->sets);
	free(edf->image_dir);
     }

   EINA_LIST_FREE(edf->color_classes, ecc)
     {
	if (edf->free_strings && ecc->name) eina_stringshare_del(ecc->name);
	free(ecc);
     }

   if (edf->collection_patterns) edje_match_patterns_free(edf->collection_patterns);
   if (edf->path) eina_stringshare_del(edf->path);
   if (edf->free_strings && edf->compiler) eina_stringshare_del(edf->compiler);
   if (edf->collection_cache) _edje_cache_coll_flush(edf);
   _edje_textblock_style_cleanup(edf);
   if (edf->ef) eet_close(edf->ef);
   free(edf);

   _edje_file_set(prev);
}

void
_edje_collection_free(Edje_File *edf, Edje_Part_Collection *ec)
{
   Edje_Program *pr;
   Edje_Part *ep;

   _edje_embryo_script_shutdown(ec);
   EINA_LIST_FREE(ec->programs, pr)
     {
	Edje_Program_Target *prt;
	Edje_Program_After *pa;

        if (edf->free_strings)
          {
             if (pr->name) eina_stringshare_del(pr->name);
             if (pr->signal) eina_stringshare_del(pr->signal);
             if (pr->source) eina_stringshare_del(pr->source);
             if (pr->filter.part) eina_stringshare_del(pr->filter.part);
             if (pr->filter.state) eina_stringshare_del(pr->filter.state);
             if (pr->state) eina_stringshare_del(pr->state);
             if (pr->state2) eina_stringshare_del(pr->state2);
          }
	EINA_LIST_FREE(pr->targets, prt)
	  free(prt);
	EINA_LIST_FREE(pr->after, pa)
	  free(pa);
	free(pr);
     }
   EINA_LIST_FREE(ec->parts, ep)
     {
	Edje_Part_Description *desc;

	if (edf->free_strings && ep->name) eina_stringshare_del(ep->name);
	if (ep->default_desc)
	  {
	     _edje_collection_free_part_description_free(ep->default_desc, edf->free_strings);
	     ep->default_desc = NULL;
	  }
	EINA_LIST_FREE(ep->other_desc, desc)
	  _edje_collection_free_part_description_free(desc, edf->free_strings);
	free(ep);
     }
   if (ec->data)
     {
	Edje_Data *edt;

	EINA_LIST_FREE(ec->data, edt)
	  {
             if (edf->free_strings)
               {
                  if (edt->key) eina_stringshare_del(edt->key);
                  if (edt->value) eina_stringshare_del(edt->value);
               }
	     free(edt);
	  }
     }
#ifdef EDJE_PROGRAM_CACHE
   if (ec->prog_cache.no_matches) eina_hash_free(ec->prog_cache.no_matches);
   if (ec->prog_cache.matches)
     {
	eina_hash_foreach(ec->prog_cache.matches,
			  _edje_collection_free_prog_cache_matches_free_cb,
			  NULL);
	eina_hash_free(ec->prog_cache.matches);
     }
#endif
   if (ec->script) embryo_program_free(ec->script);
#ifdef LUA2   
   _edje_lua2_script_unload(ec);
#endif
   free(ec);
}

void
_edje_collection_free_part_description_free(Edje_Part_Description *desc, Eina_Bool free_strings)
{
   Edje_Part_Image_Id *pi;

   EINA_LIST_FREE(desc->image.tween_list, pi)
     free(pi);
   if (desc->external_params)
     _edje_external_params_free(desc->external_params, free_strings);
   if (free_strings)
     {
	if (desc->color_class)     eina_stringshare_del(desc->color_class);
	if (desc->text.text)       eina_stringshare_del(desc->text.text);
	if (desc->text.text_class) eina_stringshare_del(desc->text.text_class);
	if (desc->text.style)      eina_stringshare_del(desc->text.style);
	if (desc->text.font)       eina_stringshare_del(desc->text.font);
     }
   free(desc);
}

#ifdef EDJE_PROGRAM_CACHE
static Eina_Bool
_edje_collection_free_prog_cache_matches_free_cb(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
   eina_list_free((Eina_List *)data);
   return EINA_TRUE;
   key = NULL;
   hash = NULL;
   fdata = NULL;
}
#endif

static void
_edje_object_pack_item_hints_set(Evas_Object *obj, Edje_Pack_Element *it)
{
   Evas_Coord w = 0, h = 0, minw, minh;

   minw = it->min.w;
   minh = it->min.h;

   if ((minw <= 0) && (minh <= 0))
     {
	edje_object_size_min_get(obj, &w, &h);
	if ((w <= 0) && (h <= 0))
	  edje_object_size_min_calc(obj, &w, &h);
     }
   else
     {
	w = minw;
	h = minh;
     }
   if (((minw <= 0) && (minh <= 0)) && ((w > 0) || (h > 0)))
     evas_object_size_hint_min_set(obj, w, h);
   else
     evas_object_size_hint_min_set(obj, minw, minh);

   evas_object_size_hint_request_set(obj, it->prefer.w, it->prefer.h);
   evas_object_size_hint_max_set(obj, it->max.w, it->max.h);
   evas_object_size_hint_padding_set(obj, it->padding.l, it->padding.r, it->padding.t, it->padding.b);
   evas_object_size_hint_align_set(obj, it->align.x, it->align.y);
   evas_object_size_hint_weight_set(obj, it->weight.x, it->weight.y);
   evas_object_size_hint_aspect_set(obj, it->aspect.mode, it->aspect.w, it->aspect.h);

   evas_object_resize(obj, w, h);
}

static void
_cb_signal_repeat(void *data, Evas_Object *obj, const char *signal, const char *source)
{
   Evas_Object	*parent;
   Edje		*ed;
   char		 new_src[4096]; /* XXX is this max reasonable? */
   size_t	 length_parent = 0;
   size_t	 length_source;

   parent = data;
   ed = _edje_fetch(obj);
   if (!ed) return;
   /* Replace snprint("%s%c%s") == memcpy + *new_src + memcat */
   if (ed->parent)
     length_parent = strlen(ed->parent);
   length_source = strlen(source);
   if (length_source + length_parent + 2 > sizeof(new_src))
     return;

   if (ed->parent)
     memcpy(new_src, ed->parent, length_parent);
   new_src[length_parent] = EDJE_PART_PATH_SEPARATOR;
   memcpy(new_src + length_parent + 1, source, length_source + 1);

   edje_object_signal_emit(parent, signal, new_src);
}
