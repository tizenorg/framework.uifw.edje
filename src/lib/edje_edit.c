/*
 * TODO
 * -----------------------------------------------------------------
 * Add LUA Support :)
 * Remove images/fonts
 * Clean the saving routines
 *
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <string.h>
#include <limits.h>
#include <sys/stat.h>
#include <errno.h>

#ifndef _MSC_VER
# include <unistd.h>
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#ifdef HAVE_EVIL
# include <Evil.h>
#endif

#include "edje_private.h"


/* Get ed(Edje*) from obj(Evas_Object*) */
#define GET_ED_OR_RETURN(RET) \
   Edje *ed; \
   Edje_Edit *eed; \
   if (!evas_object_smart_type_check_ptr(obj, _edje_edit_type)) \
     return RET; \
   eed = evas_object_smart_data_get(obj); \
   if (!eed) return RET; \
   ed = (Edje *)eed;

/* Get rp(Edje_Real_Part*) from obj(Evas_Object*) and part(char*) */
#define GET_RP_OR_RETURN(RET) \
   Edje *ed; \
   Edje_Edit *eed; \
   Edje_Real_Part *rp; \
   if (!evas_object_smart_type_check_ptr(obj, _edje_edit_type)) \
     return RET; \
   eed = evas_object_smart_data_get(obj); \
   if (!eed) return RET; \
   ed = (Edje *)eed; \
   rp = _edje_real_part_get(ed, part); \
   if (!rp) return RET;

/* Get pd(Edje_Part_Description*) from obj(Evas_Object*), part(char*) and state (char*) */
#define GET_PD_OR_RETURN(RET) \
   Edje *ed; \
   Edje_Edit *eed; \
   Edje_Real_Part *rp; \
   Edje_Part_Description_Common *pd; \
   if (!evas_object_smart_type_check_ptr(obj, _edje_edit_type)) \
     return RET; \
   eed = evas_object_smart_data_get(obj); \
   if (!eed) return RET; \
   ed = (Edje *)eed; \
   rp = _edje_real_part_get(ed, part); \
   if (!rp) return RET; \
   pd = _edje_part_description_find_byname(eed, part, state, value); \
   if (!pd) return RET;

/* Get epr(Edje_Program*) from obj(Evas_Object*) and prog(char*)*/
#define GET_EPR_OR_RETURN(RET) \
   Edje_Program *epr; \
   if (!evas_object_smart_type_check_ptr(obj, _edje_edit_type)) \
     return RET; \
   epr = _edje_program_get_byname(obj, prog); \
   if (!epr) return RET;

static void *
_alloc(size_t size)
{
   void *mem;

   mem = calloc(1, size);
   if (mem) return mem;
   ERR("Edje_Edit: Error. memory allocation of %i bytes failed. %s",
       (int)size, strerror(errno));
   return NULL;
}

/*************/
/* INTERNALS */
/*************/

/* Edje_Edit smart! Overloads the edje one adding some more control stuff */
static const char _edje_edit_type[] = "edje_edit";

typedef struct _Edje_Edit Edje_Edit;
struct _Edje_Edit
{
   Edje base;
};

static void _edje_edit_smart_add(Evas_Object *obj);
static void _edje_edit_smart_del(Evas_Object *obj);

static Eina_Bool _edje_edit_smart_file_set(Evas_Object *obj, const char *file, const char *group);
static Eina_Bool _edje_edit_edje_file_save(Eet_File *eetf, Edje_File *ef);

EVAS_SMART_SUBCLASS_NEW(_edje_edit_type, _edje_edit, Edje_Smart_Api,
			Edje_Smart_Api, _edje_object_smart_class_get, NULL)

static void
_edje_edit_smart_set_user(Edje_Smart_Api *sc)
{
   sc->base.add = _edje_edit_smart_add;
   sc->base.del = _edje_edit_smart_del;
   sc->file_set = _edje_edit_smart_file_set;
}

static void
_edje_edit_smart_add(Evas_Object *obj)
{
   Edje_Edit *eed;

   eed = evas_object_smart_data_get(obj);
   if (!eed)
     {
	const Evas_Smart *smart;
	const Evas_Smart_Class *sc;

	eed = calloc(1, sizeof(Edje_Edit));
	if (!eed) return;

	smart = evas_object_smart_smart_get(obj);
	sc = evas_smart_class_get(smart);
	eed->base.api = (const Edje_Smart_Api *)sc;

	evas_object_smart_data_set(obj, eed);
     }

   _edje_edit_parent_sc->base.add(obj);
}

static void
_edje_edit_smart_del(Evas_Object *obj)
{
//   Edje_Edit *eed;

//   eed = evas_object_smart_data_get(obj);
   _edje_edit_parent_sc->base.del(obj);
}

static Eina_Bool
_edje_edit_smart_file_set(Evas_Object *obj, const char *file, const char *group)
{
//   Edje_Edit *eed;

//   eed = evas_object_smart_data_get(obj);
   /* Nothing custom here yet, so we just call the parent function.
    * TODO and maybes:
    *  * The whole point of this thing is keep track of stuff such as
    *    strings to free and who knows what, so we need to take care
    *    of those if the file/group changes.
    *  * Maybe have the possibility to open just files, not always with
    *    a group given.
    *  * A way to skip the cache? Could help avoid some issues when editing
    *    a group being used by the application in some other way, or multiple
    *    opens of the same file.
    *  * Here we probably want to allow opening groups with broken references
    *    (GROUP parts or BOX/TABLE items pointing to non-existant/renamed
    *    groups).
    */
   return _edje_edit_parent_sc->file_set(obj, file, group);
}

EAPI Evas_Object *
edje_edit_object_add(Evas *e)
{
   return evas_object_smart_add(e, _edje_edit_smart_class_new());
}
/* End of Edje_Edit smart stuff */

static Edje_Part_Description_Common *
_edje_part_description_find_byname(Edje_Edit *eed, const char *part, const char *state, double value)
{
   Edje_Real_Part *rp;
   Edje_Part_Description_Common *pd;

   if (!eed || !part || !state) return NULL;

   rp = _edje_real_part_get((Edje *)eed, part);
   if (!rp) return NULL;

   pd = _edje_part_description_find((Edje *)eed, rp, state, value);

   return pd;
}

static int
_edje_image_id_find(Evas_Object *obj, const char *image_name)
{
   unsigned int i;

   GET_ED_OR_RETURN(-1);

   if (!ed->file) return -1;
   if (!ed->file->image_dir) return -1;

   //printf("SEARCH IMAGE %s\n", image_name);

   for (i = 0; i < ed->file->image_dir->entries_count; ++i)
     if (ed->file->image_dir->entries[i].entry
	 && !strcmp(image_name, ed->file->image_dir->entries[i].entry))
       return i;

   return -1;
}

static const char *
_edje_image_name_find(Evas_Object *obj, int image_id)
{
   GET_ED_OR_RETURN(NULL);

   if (!ed->file) return NULL;
   if (!ed->file->image_dir) return NULL;

   /* Special case for external image */
   if (image_id < 0) image_id = -image_id - 1;

   //printf("SEARCH IMAGE ID %d\n", image_id);
   if ((unsigned int) image_id >= ed->file->image_dir->entries_count) return NULL;
   return ed->file->image_dir->entries[image_id].entry;
}

static void
_edje_real_part_free(Edje_Real_Part *rp)
{
   if (!rp) return;

   if (rp->object)
     {
	_edje_callbacks_del(rp->object, rp->edje);
	evas_object_del(rp->object);
     }

   if (rp->swallowed_object)
     {
	evas_object_smart_member_del(rp->swallowed_object);
	evas_object_event_callback_del(rp->swallowed_object,
				       EVAS_CALLBACK_FREE,
				       _edje_object_part_swallow_free_cb);
	evas_object_clip_unset(rp->swallowed_object);
	evas_object_data_del(rp->swallowed_object, "\377 edje.swallowing_part");
	if (rp->part->mouse_events)
	  _edje_callbacks_del(rp->swallowed_object, rp->edje);

	if (rp->part->type == EDJE_PART_TYPE_GROUP ||
	    rp->part->type == EDJE_PART_TYPE_EXTERNAL)
	  evas_object_del(rp->swallowed_object);

	rp->swallowed_object = NULL;
     }

   if (rp->text.text) eina_stringshare_del(rp->text.text);
   if (rp->text.font) eina_stringshare_del(rp->text.font);
   if (rp->text.cache.in_str) eina_stringshare_del(rp->text.cache.in_str);
   if (rp->text.cache.out_str) eina_stringshare_del(rp->text.cache.out_str);

   if (rp->custom)
     {
	_edje_collection_free_part_description_clean(rp->part->type, rp->custom->description, 0);
	free(rp->custom);
	rp->custom = NULL;
     }

   free(rp->drag);

   if (rp->param2)
     free(rp->param2->set);
   eina_mempool_free(_edje_real_part_state_mp, rp->param2);

   if (rp->custom)
     free(rp->custom->set);
   eina_mempool_free(_edje_real_part_state_mp, rp->custom);

   _edje_unref(rp->edje);
   eina_mempool_free(_edje_real_part_mp, rp);
}

static Eina_Bool
_edje_import_font_file(Edje *ed, const char *path, const char *entry)
{
   void *fdata = NULL;
   long fsize = 0;

   /* Read font data from file */
   {
      FILE *f = fopen(path, "rb");
      if (!f)
	{
	   ERR("Unable to open font file \"%s\"", path);
	   return EINA_FALSE;
	}

      fseek(f, 0, SEEK_END);
      fsize = ftell(f);
      rewind(f);
      fdata = malloc(fsize);
      if (!fdata)
         {
	    ERR("Unable to alloc font file \"%s\"", path);
	    fclose(f);
	    return EINA_FALSE;
         }
      if (fread(fdata, fsize, 1, f) != 1)
	 {
            free(fdata);
            fclose(f);
	    ERR("Unable to read all of font file \"%s\"", path);
	    return EINA_FALSE;
	 }
      fclose(f);
   }

   /* Write font to edje file */
   {
      /* open the eet file */
      Eet_File *eetf = eet_open(ed->path, EET_FILE_MODE_READ_WRITE);
      if (!eetf)
	{
	   ERR("Unable to open \"%s\" for writing output", ed->path);
	   free(fdata);
	   return EINA_FALSE;
	}

      if (eet_write(eetf, entry, fdata, fsize, 1) <= 0)
        {
           ERR("Unable to write font part \"%s\" as \"%s\" part entry",
	       path, entry);
           eet_close(eetf);
           free(fdata);
           return EINA_FALSE;
        }

      free(fdata);

      /* write the edje_file */
      if (!_edje_edit_edje_file_save(eetf, ed->file))
	{
	   eet_delete(eetf, entry);
	   eet_close(eetf);
	   return EINA_FALSE;
	}

      eet_close(eetf);
   }

   return EINA_TRUE;
}


static Eina_Bool
_edje_import_image_file(Edje *ed, const char *path, int id)
{
   char entry[PATH_MAX];
   Evas_Object *im;
   Eet_File *eetf;
   void *im_data;
   int  im_w, im_h;
   int  im_alpha;
   int bytes;

   /* Try to load the file */
   im = evas_object_image_add(ed->evas);
   if (!im) return EINA_FALSE;

   evas_object_image_file_set(im, path, NULL);
   if (evas_object_image_load_error_get(im) != EVAS_LOAD_ERROR_NONE)
     {
        ERR("Edje_Edit: unable to load image \"%s\"."
	    "Missing PNG or JPEG loader modules for Evas or "
	    "file does not exist, or is not readable.", path);
        evas_object_del(im);
	im = NULL;
	return EINA_FALSE;
     }

   /* Write the loaded image to the edje file */

   evas_object_image_size_get(im, &im_w, &im_h);
   im_alpha = evas_object_image_alpha_get(im);
   im_data = evas_object_image_data_get(im, 0);
   if ((!im_data) || !(im_w > 0) || !(im_h > 0))
     {
	evas_object_del(im);
	return EINA_FALSE;
     }

   /* open the eet file */
   eetf = eet_open(ed->path, EET_FILE_MODE_READ_WRITE);
   if (!eetf)
     {
	ERR("Unable to open \"%s\" for writing output", ed->path);
	evas_object_del(im);
	return EINA_FALSE;
     }

   snprintf(entry, sizeof(entry), "images/%i", id);

   /* write the image data */
   bytes = eet_data_image_write(eetf, entry,
				im_data, im_w, im_h,
				im_alpha,
				0, 100, 1);
   if (bytes <= 0)
     {
	ERR("Unable to write image part \"%s\" part entry to %s",
	    entry, ed->path);
	eet_close(eetf);
	evas_object_del(im);
	return EINA_FALSE;
     }

   evas_object_del(im);

   /* write the edje_file */
   if (!_edje_edit_edje_file_save(eetf, ed->file))
     {
	eet_delete(eetf, entry);
	eet_close(eetf);
	return EINA_FALSE;
     }

   eet_close(eetf);
   return EINA_TRUE;
}

static int
_edje_part_id_find(Edje *ed, const char *part)
{
   int id;

   for (id = 0; id < ed->table_parts_size; id++)
     {
	Edje_Real_Part *rp = ed->table_parts[id];

	if (!strcmp(rp->part->name, part))
	  return id;
     }
   return -1;
}

static void
_edje_part_description_id_set(int type, Edje_Part_Description_Common *c, int old_id, int new_id)
{
   if (c->rel1.id_x == old_id) c->rel1.id_x = new_id;
   if (c->rel1.id_y == old_id) c->rel1.id_y = new_id;
   if (c->rel2.id_x == old_id) c->rel2.id_x = new_id;
   if (c->rel2.id_y == old_id) c->rel2.id_y = new_id;

   if (type == EDJE_PART_TYPE_TEXT
       || type == EDJE_PART_TYPE_TEXTBLOCK)
     {
	Edje_Part_Description_Text *t;

	t = (Edje_Part_Description_Text *) c;

	if (t->text.id_source == old_id) t->text.id_source = new_id;
	if (t->text.id_text_source == old_id) t->text.id_text_source = new_id;
     }
}

static void
_edje_part_program_id_set(Edje_Program *epr, int old_id, int new_id)
{
   Edje_Program_Target *pt;
   Eina_List *ll, *l_next;

   if (epr->action != EDJE_ACTION_TYPE_STATE_SET)
     return;

   EINA_LIST_FOREACH_SAFE(epr->targets, ll, l_next, pt)
     {
	if (pt->id == old_id)
	  {
	     if (new_id == -1)
	       epr->targets = eina_list_remove_list(epr->targets, ll);
	     else
	       pt->id = new_id;
	  }
     }
}

static void
_edje_part_id_set(Edje *ed, Edje_Real_Part *rp, int new_id)
{
   /* This function change the id of a given real_part.
    * All the depedency will be updated too.
    * Also the table_parts is updated, and the current *rp in the table
    * is lost.
    * If new Id = -1 then all the depencies will be deleted
    */
   Edje_Part *part;
   unsigned int j;
   int old_id;

   part = rp->part;

   if (!part) return;
   //printf("CHANGE ID OF PART %s TO %d\n", part->name, new_id);

   if (!ed || new_id < -1) return;

   if (part->id == new_id) return;

   old_id = part->id;
   part->id = new_id;

   /* Fix all the dependecies in all parts... */
   for (j = 0; j < ed->collection->parts_count; ++j)
     {
	Edje_Part *p;
	unsigned int k;

	p = ed->collection->parts[j];

	//printf("   search id: %d in %s\n", old_id, p->name);
	if (p->clip_to_id == old_id) p->clip_to_id = new_id;
	if (p->dragable.confine_id == old_id) p->dragable.confine_id = new_id;

	/* ...in default description */
	_edje_part_description_id_set(p->type, p->default_desc, old_id, new_id);

	/* ...and in all other descriptions */
	for (k = 0; k < p->other.desc_count; ++k)
	  _edje_part_description_id_set(p->type, p->other.desc[k], old_id, new_id);
     }

   /*...and also in programs targets */
#define EDJE_EDIT_PROGRAM_ID_SET(Array, Ed, It, Old, New)		\
   for (It = 0; It < Ed->collection->programs.Array##_count; ++It)	\
     _edje_part_program_id_set(Ed->collection->programs.Array[It], Old, New);

   EDJE_EDIT_PROGRAM_ID_SET(fnmatch, ed, j, old_id, new_id);
   EDJE_EDIT_PROGRAM_ID_SET(strcmp, ed, j, old_id, new_id);
   EDJE_EDIT_PROGRAM_ID_SET(strncmp, ed, j, old_id, new_id);
   EDJE_EDIT_PROGRAM_ID_SET(strrncmp, ed, j, old_id, new_id);
   EDJE_EDIT_PROGRAM_ID_SET(nocmp, ed, j, old_id, new_id);

   /* Adjust table_parts */
   if (new_id >= 0)
     ed->table_parts[new_id] = rp;
}

static void
_edje_part_description_id_switch(int type, Edje_Part_Description_Common *c, int id1, int id2)
{
   if (c->rel1.id_x == id1) c->rel1.id_x = id2;
   else if (c->rel1.id_x == id2) c->rel1.id_x = id1;
   if (c->rel1.id_y == id1) c->rel1.id_y = id2;
   else if (c->rel1.id_y == id2) c->rel1.id_y = id1;
   if (c->rel2.id_x == id1) c->rel2.id_x = id2;
   else if (c->rel2.id_x == id2) c->rel2.id_x = id1;
   if (c->rel2.id_y == id1) c->rel2.id_y = id2;
   else if (c->rel2.id_y == id2) c->rel2.id_y = id1;

   if (type == EDJE_PART_TYPE_TEXT
       || type == EDJE_PART_TYPE_TEXTBLOCK)
     {
	Edje_Part_Description_Text *t;

	t = (Edje_Part_Description_Text *) c;

	if (t->text.id_source == id1) t->text.id_source = id2;
	else if (t->text.id_source == id2) t->text.id_source = id1;
	if (t->text.id_text_source == id1) t->text.id_text_source = id2;
	else if (t->text.id_text_source == id2) t->text.id_text_source = id2;
     }
}

static void
_edje_part_program_id_switch(Edje_Program *epr, int id1, int id2)
{
   Edje_Program_Target *pt;
   Eina_List *ll;

   if (epr->action != EDJE_ACTION_TYPE_STATE_SET)
     return;

   EINA_LIST_FOREACH(epr->targets, ll, pt)
     {
	if (pt->id == id1) pt->id = id2;
	else if (pt->id == id2) pt->id = id1;
     }
}

static void
_edje_parts_id_switch(Edje *ed, Edje_Real_Part *rp1, Edje_Real_Part *rp2)
{
   /* This function switch the id of two parts.
    * All the depedency will be updated too.
    * Also the table_parts is updated,
    * The parts list isn't touched
    */
   int id1;
   int id2;
   unsigned int i;

   //printf("SWITCH ID OF PART %d AND %d\n", rp1->part->id, rp2->part->id);

   if (!ed || !rp1 || !rp2) return;
   if (rp1 == rp2) return;

   id1 = rp1->part->id;
   id2 = rp2->part->id;

   /* Switch ids */
   rp1->part->id = id2;
   rp2->part->id = id1;

   /* adjust table_parts */
   ed->table_parts[id1] = rp2;
   ed->table_parts[id2] = rp1;

   // Fix all the dependecies in all parts...
   for (i = 0; i < ed->collection->parts_count; ++i)
     {
	unsigned int j;
	Edje_Part *p;

	p = ed->collection->parts[i];

	//printf("   search id: %d in %s\n", old_id, p->name);
	if (p->clip_to_id == id1) p->clip_to_id = id2;
	else if (p->clip_to_id == id2) p->clip_to_id = id1;
	if (p->dragable.confine_id == id1) p->dragable.confine_id = id2;
	else if (p->dragable.confine_id == id2) p->dragable.confine_id = id1;

	// ...in default description
	_edje_part_description_id_switch(p->type, p->default_desc, id1, id2);

	// ...and in all other descriptions
	for (j = 0; j < p->other.desc_count; ++j)
	  _edje_part_description_id_switch(p->type, p->other.desc[j], id1, id2);
     }

   //...and also in programs targets
#define EDJE_EDIT_PROGRAM_SWITCH(Array, Ed, It, Id1, Id2)		\
   for (It = 0; It < Ed->collection->programs.Array##_count; ++It)	\
     _edje_part_program_id_switch(Ed->collection->programs.Array[i], Id1, Id2);

   EDJE_EDIT_PROGRAM_SWITCH(fnmatch, ed, i, id1, id2);
   EDJE_EDIT_PROGRAM_SWITCH(strcmp, ed, i, id1, id2);
   EDJE_EDIT_PROGRAM_SWITCH(strncmp, ed, i, id1, id2);
   EDJE_EDIT_PROGRAM_SWITCH(strrncmp, ed, i, id1, id2);
   EDJE_EDIT_PROGRAM_SWITCH(nocmp, ed, i, id1, id2);
   //TODO Real part dependencies are ok?
}

static void
_edje_fix_parts_id(Edje *ed)
{
   /* We use this to clear the id hole leaved when a part is removed.
    * After the execution of this function all parts will have a right
    * (uniqe & ordered) id. The table_parts is also updated.
    */
   unsigned int i;
   int correct_id;
   int count;

   //printf("FIXING PARTS ID \n");

   //TODO order the list first to be more robust

   /* Give a correct id to all the parts */
   correct_id = 0;
   for (i = 0; i < ed->collection->parts_count; ++i)
     {
	Edje_Part *p;

	p = ed->collection->parts[i];

	//printf(" [%d]Checking part: %s id: %d\n", correct_id, p->name, p->id);
	if (p->id != correct_id)
	  if (ed->table_parts[p->id])
	    _edje_part_id_set(ed, ed->table_parts[p->id], correct_id);

	correct_id++;
     }

   /* If we have removed some parts realloc table_parts */
   count = ed->collection->parts_count;
   if (count != ed->table_parts_size)
     {
        ed->table_parts = realloc(ed->table_parts, sizeof(Edje_Real_Part *) * count);
        ed->table_parts_size = count;
     }

   //printf("\n");
}

static void
_edje_if_string_free(Edje *ed, const char *str)
{
   Eet_Dictionary *dict;

   if (!ed || !str) return;

   dict = eet_dictionary_get(ed->file->ef);
   if (eet_dictionary_string_check(dict, str)) return;
   eina_stringshare_del(str);
   str = NULL;
}

static Edje_Style *
_edje_edit_style_get(Edje *ed, const char *name)
{
   Eina_List *l;
   Edje_Style *s;

   if (!ed || !ed->file || !ed->file->styles || !name)
      return NULL;

   EINA_LIST_FOREACH(ed->file->styles, l, s)
      if (s->name && !strcmp(s->name, name))
         return s;

   return NULL;
}

static Edje_Style_Tag *
_edje_edit_style_tag_get(Edje *ed, const char *style, const char *name)
{
   Eina_List *l;
   Edje_Style *s;
   Edje_Style_Tag *t;

   if (!ed || !ed->file || !ed->file->styles || !name)
      return NULL;

   s = _edje_edit_style_get(ed, style);

   EINA_LIST_FOREACH(s->tags, l, t)
      if (t->key && !strcmp(t->key, name))
         return t;

   return NULL;
}

static Edje_External_Directory_Entry *
_edje_edit_external_get(Edje *ed, const char *name)
{
   unsigned int i;

   if (!ed || !ed->file || !ed->file->external_dir || !name)
     return NULL;

   for (i = 0; i < ed->file->external_dir->entries_count; ++i)
     if (ed->file->external_dir->entries[i].entry
	 && !strcmp(ed->file->external_dir->entries[i].entry, name))
       return ed->file->external_dir->entries + i;

   return NULL;
}

void
_edje_edit_group_references_update(Evas_Object *obj, const char *old_group_name, const char *new_group_name)
{
   Eina_Iterator *i;
   Eina_List *pll, *pl;
//   Edje_Part_Collection *pc;
   Edje_Part_Collection_Directory_Entry *pce;
   char *part_name;
   const char *source, *old;
   Edje_Part_Type type;
   Evas_Object *part_obj;

   GET_ED_OR_RETURN();

//   pc = ed->collection;

   part_obj = edje_edit_object_add(ed->evas);

   old = eina_stringshare_add(old_group_name);

   i = eina_hash_iterator_data_new(ed->file->collection);

   EINA_ITERATOR_FOREACH(i, pce)
     {
	edje_object_file_set(part_obj, ed->file->path, pce->entry);

	pl = edje_edit_parts_list_get(part_obj);

	EINA_LIST_FOREACH(pl, pll, part_name)
	  {
	     source = edje_edit_part_source_get(part_obj, part_name);
	     type = edje_edit_part_type_get(part_obj, part_name);

	     if (type ==  EDJE_PART_TYPE_GROUP && source == old)
	       edje_edit_part_source_set(part_obj, part_name, new_group_name);

	     if (source)
	       eina_stringshare_del(source);
	  }
     }

   eina_iterator_free(i);

   eina_stringshare_del(old);

   evas_object_del(part_obj);
}

/*****************/
/*  GENERAL API  */
/*****************/

EAPI void
edje_edit_string_list_free(Eina_List *lst)
{
   //printf("FREE LIST: \n");
   while (lst)
     {
        if (eina_list_data_get(lst)) eina_stringshare_del(eina_list_data_get(lst));
	//printf("FREE: %s\n", eina_list_data_get(lst));
	lst = eina_list_remove(lst, eina_list_data_get(lst));
     }
}

EAPI void
edje_edit_string_free(const char *str)
{
   if (str) eina_stringshare_del(str);
}

EAPI const char*
edje_edit_compiler_get(Evas_Object *obj)
{
   GET_ED_OR_RETURN(0);
   return eina_stringshare_add(ed->file->compiler);
}

/****************/
/*  GROUPS API  */
/****************/

/**
 * @brief Add an edje (empty) group to an edje object's group set.
 *
 * @param obj The pointer to edje object.
 * @param name The name of the group.
 *
 * @return 1 If it could allocate memory to the part group added
 * or zero if not.
 *
 * This function adds, at run time, one more group, which will reside
 * in memory, to the group set found in the .edj file which @a obj was
 * loaded with. This group can be manipulated by other API functions,
 * like @c edje_edit_part_add(), for example. If desired, the new
 * group can be actually commited the respective .edj by use of @c
 * edje_edit_save().
 *
 */
EAPI Eina_Bool
edje_edit_group_add(Evas_Object *obj, const char *name)
{
   Edje_Part_Collection_Directory_Entry *de;
   Edje_Part_Collection_Directory_Entry *d;
   Edje_Part_Collection *pc;
   int id;
   int search;
   //Code *cd;

   GET_ED_OR_RETURN(EINA_FALSE);

   //printf("ADD GROUP: %s \n", name);

   /* check if a group with the same name already exists */
   if (eina_hash_find(ed->file->collection, name))
     return EINA_FALSE;

   /* Create structs */
   de = _alloc(sizeof(Edje_Part_Collection_Directory_Entry));
   if (!de) return EINA_FALSE;

   pc = _alloc(sizeof(Edje_Part_Collection));
   if (!pc)
     {
	free(de);
	return EINA_FALSE;
     }

   /* Search first free id */
   id = -1;
   search = 0;
   while (id == -1)
     {
	Eina_Iterator *i;
	Eina_Bool found = 0;

	i = eina_hash_iterator_data_new(ed->file->collection);

	EINA_ITERATOR_FOREACH(i, d)
	  {
	     // printf("search if %d is free [id %d]\n", search, d->id);
	     if (search == d->id)
	       {
		  found = 1;
		  break;
	       }
	  }

	eina_iterator_free(i);

	if (!found) id = search;
	else search++;
     }

   /* Init Edje_Part_Collection_Directory_Entry */
   //printf(" new id: %d\n", id);
   de->id = id;
   de->entry = eina_stringshare_add(name);
   eina_hash_direct_add(ed->file->collection, de->entry, de);

   /* Init Edje_Part_Collection */
   pc->id = id;
   pc->references = 0;
   memset(&pc->programs, 0, sizeof (pc->programs));
   pc->parts = NULL;
   pc->data = NULL;
   pc->script = NULL;
   pc->part = eina_stringshare_add(name);

   //cd = _alloc(sizeof(Code));
   //codes = eina_list_append(codes, cd);
#define EDIT_EMN(Tp, Sz, Ce)							\
   Ce->mp.Tp = eina_mempool_add("chained_mempool", #Tp, NULL, sizeof (Sz), 10);

   EDIT_EMN(RECTANGLE, Edje_Part_Description_Common, de);
   EDIT_EMN(TEXT, Edje_Part_Description_Text, de);
   EDIT_EMN(IMAGE, Edje_Part_Description_Image, de);
   EDIT_EMN(SWALLOW, Edje_Part_Description_Common, de);
   EDIT_EMN(TEXTBLOCK, Edje_Part_Description_Text, de);
   EDIT_EMN(GROUP, Edje_Part_Description_Common, de);
   EDIT_EMN(BOX, Edje_Part_Description_Box, de);
   EDIT_EMN(TABLE, Edje_Part_Description_Table, de);
   EDIT_EMN(EXTERNAL, Edje_Part_Description_External, de);
   EDIT_EMN(part, Edje_Part, de);

   ed->file->collection_cache = eina_list_prepend(ed->file->collection_cache, pc);
   _edje_cache_coll_clean(ed->file);

   return EINA_TRUE;
}

/**
 * @brief Delete the specified group from the edje file.
 *
 * @param obj The pointer to the edje object.
 * @param group_name Group to delete.
 *
 * @return @c EINA_TRUE on success, @c EINA_FALSE on failure.
 *
 * This function deletes the given group from the file @a obj is set to. This
 * operation can't be undone as all references to the group are removed from
 * the file.
 * This function may fail if the group to be deleted is currently in use.
 *
 */
EAPI Eina_Bool
edje_edit_group_del(Evas_Object *obj, const char *group_name)
{
   Edje_Part_Collection_Directory_Entry *e;
   Edje_Part_Collection *die = NULL;
   Edje_Part_Collection *g;
   Eina_List *l;
   Eet_File *eetf;
   char buf[32];

   GET_ED_OR_RETURN(EINA_FALSE);

   /* if (eina_hash_find(ed->file->collection_hash, group_name)) */
   /*   return EINA_FALSE; */

   if (strcmp(ed->group, group_name) == 0) return EINA_FALSE;

   _edje_edit_group_references_update(obj, group_name, NULL);

   e = eina_hash_find(ed->file->collection, group_name);
   if (!e) return EINA_FALSE;

   if (e->id == ed->collection->id) return EINA_FALSE;
   if (e->ref)
     {
	ERR("EEK: Group \"%s\" still in use !", group_name);
	die = e->ref;
	e->ref = NULL;
	eina_hash_del(ed->file->collection, group_name, e);
     }

   EINA_LIST_FOREACH(ed->file->collection_cache, l, g)
     if (g->id == e->id)
       {
	  ed->file->collection_cache =
	    eina_list_remove_list(ed->file->collection_cache, l);
	  die = g;
	  break;
       }

   /* Remove collection/id from eet file */
   eetf = eet_open(ed->file->path, EET_FILE_MODE_READ_WRITE);
   if (!eetf)
     {
	ERR("Edje_Edit: Error. unable to open \"%s\" "
	    "for writing output", ed->file->path);
	return EINA_FALSE;
     }
   snprintf(buf, sizeof(buf), "collections/%d", e->id);
   eet_delete(eetf, buf);
   eet_close(eetf);

   /* Free Group */
   if (die) _edje_collection_free(ed->file, die, e);

   /* we need to save everything to make sure the file won't have broken
    * references the next time is loaded */
   edje_edit_save_all(obj);

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_group_exist(Evas_Object *obj, const char *group)
{
   GET_ED_OR_RETURN(EINA_FALSE);

   if (eina_hash_find(ed->file->collection, group))
     return EINA_TRUE;
   return EINA_FALSE;
}

EAPI Eina_Bool
edje_edit_group_name_set(Evas_Object *obj, const char *new_name)
{
   Edje_Part_Collection_Directory_Entry *pce;
   Edje_Part_Collection *pc;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!new_name) return EINA_FALSE;

   pc = ed->collection;

   if (!strcmp(pc->part, new_name)) return EINA_TRUE;

   if (edje_edit_group_exist(obj, new_name)) return EINA_FALSE;

   _edje_edit_group_references_update(obj, pc->part, new_name);

   //printf("Set name of current group: %s [id: %d][new name: %s]\n",
	// pc->part, pc->id, new_name);

   //if (pc->part && ed->file->free_strings) eina_stringshare_del(pc->part); TODO FIXME
   pce = eina_hash_find(ed->file->collection, pc->part);

   eina_hash_move(ed->file->collection, pce->entry, new_name);

   pce->entry = eina_stringshare_add(new_name);
   pc->part = pce->entry;

   return EINA_TRUE;
}

#define FUNC_GROUP_ACCESSOR(Class, Value)			\
  EAPI int							\
  edje_edit_group_##Class##_##Value##_get(Evas_Object *obj)	\
  {								\
     GET_ED_OR_RETURN(-1);					\
     if (!ed->collection) return -1;				\
     return ed->collection->prop.Class.Value;			\
  }								\
  EAPI void							\
  edje_edit_group_##Class##_##Value##_set(Evas_Object *obj, int v)	\
  {								\
     GET_ED_OR_RETURN();					\
     ed->collection->prop.Class.Value = v;			\
  }

FUNC_GROUP_ACCESSOR(min, w);
FUNC_GROUP_ACCESSOR(min, h);
FUNC_GROUP_ACCESSOR(max, w);
FUNC_GROUP_ACCESSOR(max, h);

/***************/
/*  DATA API   */
/***************/

EAPI Eina_List *
edje_edit_group_data_list_get(Evas_Object * obj)
{
   Eina_Iterator *it;
   Eina_List *datas = NULL;
   const char *key;

   GET_ED_OR_RETURN(NULL);

   if (!ed->file || !ed->collection || !ed->collection->data)
     return NULL;

   it = eina_hash_iterator_key_new(ed->collection->data);
   if (!it) return NULL;

   EINA_ITERATOR_FOREACH(it, key)
     datas = eina_list_append(datas, eina_stringshare_add(key));

   eina_iterator_free(it);

   return datas;
}

EAPI Eina_List *
edje_edit_data_list_get(Evas_Object * obj)
{
   Eina_Iterator *i;
   Eina_List *datas = NULL;
   const char *key;

   GET_ED_OR_RETURN(NULL);

   if (!ed->file || !ed->file->data)
     return NULL;

   i = eina_hash_iterator_key_new(ed->file->data);

   EINA_ITERATOR_FOREACH(i, key)
     datas = eina_list_append(datas, eina_stringshare_add(key));

   eina_iterator_free(i);

   return datas;
}

EAPI Eina_Bool
edje_edit_group_data_add(Evas_Object *obj, const char *key, const char *value)
{
   GET_ED_OR_RETURN(EINA_FALSE);

   if (!key || !ed->file || !ed->collection)
     return EINA_FALSE;

   if (!ed->collection->data)
     ed->collection->data = eina_hash_string_small_new(NULL);

   if (eina_hash_find(ed->collection->data, key))
     return EINA_FALSE;

   return eina_hash_add(ed->collection->data, key, eina_stringshare_add(value));
}

EAPI Eina_Bool
edje_edit_data_add(Evas_Object *obj, const char *itemname, const char *value)
{
   GET_ED_OR_RETURN(EINA_FALSE);

   if (!itemname || !ed->file)
     return EINA_FALSE;

   if (eina_hash_find(ed->file->data, itemname))
     return EINA_FALSE;

   eina_hash_add(ed->file->data, itemname, eina_stringshare_add(value));

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_group_data_del(Evas_Object *obj, const char *key)
{
   const char *value;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!key || !ed->file || !ed->collection)
     return EINA_FALSE;

   value = eina_hash_find(ed->collection->data, key);
   if (!value) return EINA_FALSE;

   _edje_if_string_free(ed, value);
   return eina_hash_del(ed->collection->data, key, value);
}

EAPI Eina_Bool
edje_edit_data_del(Evas_Object *obj, const char *itemname)
{
   GET_ED_OR_RETURN(EINA_FALSE);

   if (!itemname || !ed->file || !ed->file->data)
     return 0;

   return eina_hash_del(ed->file->data, itemname, NULL);
}

EAPI const char *
edje_edit_group_data_value_get(Evas_Object * obj, char *key)
{
   const char *value;

   GET_ED_OR_RETURN(NULL);

   if (!key || !ed->file || !ed->collection)
     return NULL;

   value = eina_hash_find(ed->collection->data, key);
   if (value) value = eina_stringshare_add(value);
   return value;
}

EAPI const char *
edje_edit_data_value_get(Evas_Object * obj, char *itemname)
{
   GET_ED_OR_RETURN(NULL);

   if (!itemname || !ed->file || !ed->file->data)
     return NULL;

   return eina_stringshare_add(eina_hash_find(ed->file->data, itemname));
}

EAPI Eina_Bool
edje_edit_group_data_value_set(Evas_Object *obj, const char *key, const char *value)
{
   const char *old_value;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!key || !value || !ed->file || !ed->collection)
     return EINA_FALSE;

   old_value = eina_hash_find(ed->collection->data, key);
   if (old_value)
     {
        value = eina_stringshare_add(value);
	eina_hash_modify(ed->collection->data, key, value);
	_edje_if_string_free(ed, old_value);
	return EINA_TRUE;
     }

   return EINA_FALSE;
}

EAPI Eina_Bool
edje_edit_data_value_set(Evas_Object *obj, const char *itemname, const char *value)
{
   const char *old;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!itemname || !value || !ed->file || !ed->file->data)
     return EINA_FALSE;

   old = eina_hash_find(ed->file->data, itemname);
   if (old)
     {
        value = eina_stringshare_add(value);
        eina_hash_modify(ed->file->data, itemname, value);
        _edje_if_string_free(ed, old);
        return EINA_TRUE;
     }
   return EINA_FALSE;
}

EAPI Eina_Bool
edje_edit_group_data_name_set(Evas_Object *obj, const char *key,  const char *new_key)
{
   const char *value;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!key || !new_key || !ed->file || !ed->collection) {
      return EINA_FALSE;
   }

   value = eina_hash_find(ed->collection->data, key);
   if (value)
     {
	eina_hash_del(ed->collection->data, key, value);
	return eina_hash_add(ed->collection->data, new_key, value);
     }

   return EINA_FALSE;
}

EAPI Eina_Bool
edje_edit_data_name_set(Evas_Object *obj, const char *itemname,  const char *newname)
{
   const char *value;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!itemname || !newname || !ed->file || !ed->file->data)
     return EINA_FALSE;

   /* Get value and prevent it's destruction */
   value = eina_hash_find(ed->file->data, itemname);
   value = eina_stringshare_add(value);
   if (!value) return EINA_FALSE;

   eina_hash_del(ed->file->data, itemname, NULL);
   eina_hash_add(ed->file->data, itemname, value);

   return EINA_TRUE;
}

/***********************/
/*  COLOR CLASSES API  */
/***********************/

EAPI Eina_List *
edje_edit_color_classes_list_get(Evas_Object * obj)
{
   Eina_List *classes = NULL;
   Eina_List *l;
   Edje_Color_Class *cc;

   GET_ED_OR_RETURN(NULL);

   if (!ed->file || !ed->file->color_classes)
      return NULL;
   //printf("GET CLASSES LIST %d %d\n", eina_list_count(ed->color_classes), eina_list_count(ed->file->color_classes));
   EINA_LIST_FOREACH(ed->file->color_classes, l, cc)
     classes = eina_list_append(classes, eina_stringshare_add(cc->name));

   return classes;
}

EAPI Eina_Bool
edje_edit_color_class_colors_get(Evas_Object *obj, const char *class_name, int *r, int *g, int *b, int *a, int *r2, int *g2, int *b2, int *a2, int *r3, int *g3, int *b3, int *a3)
{
   Eina_List *l;
   Edje_Color_Class *cc;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!ed->file || !ed->file->color_classes)
      return EINA_FALSE;

   EINA_LIST_FOREACH(ed->file->color_classes, l, cc)
     if (!strcmp(cc->name, class_name))
       {
	 if (r) *r = cc->r;
	 if (g) *g = cc->g;
	 if (b) *b = cc->b;
	 if (a) *a = cc->a;

	 if (r2) *r2 = cc->r2;
	 if (g2) *g2 = cc->g2;
	 if (b2) *b2 = cc->b2;
	 if (a2) *a2 = cc->a2;

	 if (r3) *r3 = cc->r3;
	 if (g3) *g3 = cc->g3;
	 if (b3) *b3 = cc->b3;
	 if (a3) *a3 = cc->a3;

	 return EINA_TRUE;
       }
   return EINA_FALSE;
}

EAPI Eina_Bool
edje_edit_color_class_colors_set(Evas_Object *obj, const char *class_name, int r, int g, int b, int a, int r2, int g2, int b2, int a2, int r3, int g3, int b3, int a3)
{
   Eina_List *l;
   Edje_Color_Class *cc;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!ed->file || !ed->file->color_classes)
      return EINA_FALSE;

   EINA_LIST_FOREACH(ed->file->color_classes, l, cc)
     if (!strcmp(cc->name, class_name))
       {
	 if (r > -1) cc->r = r;
	 if (g > -1) cc->g = g;
	 if (b > -1) cc->b = b;
	 if (a > -1) cc->a = a;

	 if (r2 > -1) cc->r2 = r2;
	 if (g2 > -1) cc->g2 = g2;
	 if (b2 > -1) cc->b2 = b2;
	 if (a2 > -1) cc->a2 = a2;

	 if (r3 > -1) cc->r3 = r3;
	 if (g3 > -1) cc->g3 = g3;
	 if (b3 > -1) cc->b3 = b3;
	 if (a3 > -1) cc->a3 = a3;

	 return EINA_TRUE;
       }
   return EINA_FALSE;
}

EAPI Eina_Bool
edje_edit_color_class_add(Evas_Object *obj, const char *name)
{
   Eina_List *l;
   Edje_Color_Class *c;
   Edje_Color_Class *cc;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!name || !ed->file)
     return EINA_FALSE;

   EINA_LIST_FOREACH(ed->file->color_classes, l, cc)
     if (strcmp(cc->name, name) == 0)
       return EINA_FALSE;

   c = _alloc(sizeof(Edje_Color_Class));
   if (!c) return EINA_FALSE;

   c->name = (char*)eina_stringshare_add(name);
   c->r = c->g = c->b = c->a = 255;
   c->r2 = c->g2 = c->b2 = c->a2 = 255;
   c->r3 = c->g3 = c->b3 = c->a3 = 255;

   ed->file->color_classes = eina_list_append(ed->file->color_classes, c);

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_color_class_del(Evas_Object *obj, const char *name)
{
   Eina_List *l;
   Edje_Color_Class *cc;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!name || !ed->file || !ed->file->color_classes)
     return EINA_FALSE;

   EINA_LIST_FOREACH(ed->file->color_classes, l, cc)
     if (strcmp(cc->name, name) == 0)
       {
	 _edje_if_string_free(ed, cc->name);
	 ed->file->color_classes = eina_list_remove(ed->file->color_classes, cc);
	 free(cc);
	 return EINA_TRUE;
       }
   return EINA_FALSE;
}

EAPI Eina_Bool
edje_edit_color_class_name_set(Evas_Object *obj, const char *name, const char *newname)
{
   Eina_List *l;
   Edje_Color_Class *cc;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!ed->file || !ed->file->color_classes)
      return EINA_FALSE;

   EINA_LIST_FOREACH(ed->file->color_classes, l, cc)
     if (!strcmp(cc->name, name))
       {
	 _edje_if_string_free(ed, cc->name);
	 cc->name = (char*)eina_stringshare_add(newname);
	 return EINA_TRUE;
       }

   return EINA_FALSE;
}



/*********************/
/*  TEXT STYLES API  */
/*********************/

EAPI Eina_List *
edje_edit_styles_list_get(Evas_Object * obj)
{
   Eina_List *styles = NULL;
   Eina_List *l;
   Edje_Style *s;

   GET_ED_OR_RETURN(NULL);

   if (!ed->file || !ed->file->styles)
      return NULL;
   //printf("GET STYLES LIST %d\n", eina_list_count(ed->file->styles));
   EINA_LIST_FOREACH(ed->file->styles, l, s)
     styles = eina_list_append(styles, eina_stringshare_add(s->name));

   return styles;
}

EAPI Eina_Bool
edje_edit_style_add(Evas_Object * obj, const char* style)
{
   Edje_Style *s;
   GET_ED_OR_RETURN(EINA_FALSE);
   //printf("ADD STYLE '%s'\n", style);

   s = _edje_edit_style_get(ed, style);
   if (s) return EINA_FALSE;

   s = _alloc(sizeof(Edje_Style));
   if (!s) return EINA_FALSE;
   s->name = (char*)eina_stringshare_add(style);
   s->tags = NULL;
   s->style = NULL;

   ed->file->styles = eina_list_append(ed->file->styles, s);
   return EINA_TRUE;
}

EAPI void
edje_edit_style_del(Evas_Object * obj, const char* style)
{
   Edje_Style *s;

   GET_ED_OR_RETURN();
   //printf("DEL STYLE '%s'\n", style);

   s = _edje_edit_style_get(ed, style);
   if (!s) return;

   ed->file->styles = eina_list_remove(ed->file->styles, s);

   _edje_if_string_free(ed, s->name);
   //~ //s->style HOWTO FREE ???
   while (s->tags)
   {
      Edje_Style_Tag *t;

      t = s->tags->data;

      s->tags = eina_list_remove(s->tags, t);
      _edje_if_string_free(ed, t->key);
      _edje_if_string_free(ed, t->value);
      _edje_if_string_free(ed, t->font);
      _edje_if_string_free(ed, t->text_class);
      free(t);
      t = NULL;
   }
   free(s);
   s = NULL;
   s = NULL;
}


EAPI Eina_List *
edje_edit_style_tags_list_get(Evas_Object * obj, const char* style)
{
   Eina_List *tags = NULL;
   Eina_List *l;
   Edje_Style *s;
   Edje_Style_Tag *t;

   GET_ED_OR_RETURN(NULL);
   if (!ed->file || !ed->file->styles || !style)
      return NULL;

   s = _edje_edit_style_get(ed, style);

   //printf("GET STYLE TAG LIST %d\n", eina_list_count(s->tags));
   EINA_LIST_FOREACH(s->tags, l, t)
      tags = eina_list_append(tags, eina_stringshare_add(t->key));

   return tags;
}

EAPI void
edje_edit_style_tag_name_set(Evas_Object * obj, const char* style, const char* tag, const char*new_name)
{
   Edje_Style_Tag *t;

   GET_ED_OR_RETURN();
   //printf("SET TAG NAME for '%s' FOR STYLE '%s'\n", tag, style);

   if (!ed->file || !ed->file->styles || !style || !tag)
      return;

   t = _edje_edit_style_tag_get(ed, style, tag);
   if (!t) return;
   _edje_if_string_free(ed, t->key);
   t->key = eina_stringshare_add(new_name);
}

EAPI const char*
edje_edit_style_tag_value_get(Evas_Object * obj, const char* style, const char* tag)
{
   Edje_Style_Tag *t;

   GET_ED_OR_RETURN(NULL);
   //printf("GET TAG '%s' FOR STYLE '%s'\n", tag, style);

   if (!ed->file || !ed->file->styles || !style || !tag)
      return NULL;

   t = _edje_edit_style_tag_get(ed, style, tag);
   if (t && t->value)
      return eina_stringshare_add(t->value);

   return NULL;
}

EAPI void
edje_edit_style_tag_value_set(Evas_Object * obj, const char* style, const char* tag, const char*new_value)
{
   Edje_Style_Tag *t;

   GET_ED_OR_RETURN();
   //printf("SET TAG VALUE for '%s' FOR STYLE '%s'\n", tag, style);

   if (!ed->file || !ed->file->styles || !style || !tag)
      return;

   t = _edje_edit_style_tag_get(ed, style, tag);
   if (!t) return;
   _edje_if_string_free(ed, t->value);
   t->value = eina_stringshare_add(new_value);
}

EAPI Eina_Bool
edje_edit_style_tag_add(Evas_Object * obj, const char* style, const char* tag_name)
{
   Edje_Style *s;
   Edje_Style_Tag *t;

   GET_ED_OR_RETURN(EINA_FALSE);
   //printf("ADD TAG '%s' IN STYLE '%s'\n", tag_name, style);

   t = _edje_edit_style_tag_get(ed, style, tag_name);
   if (t) return EINA_FALSE;
   s = _edje_edit_style_get(ed, style);
   if (!s) return EINA_FALSE;

   t = _alloc(sizeof(Edje_Style_Tag));
   if (!t) return EINA_FALSE;
   t->key = eina_stringshare_add(tag_name);
   t->value = NULL;
   t->font = NULL;
   t->text_class = NULL;

   s->tags = eina_list_append(s->tags, t);
   return EINA_TRUE;
}

EAPI void
edje_edit_style_tag_del(Evas_Object * obj, const char* style, const char* tag)
{
   Edje_Style *s;
   Edje_Style_Tag *t;

   GET_ED_OR_RETURN();
   //printf("DEL TAG '%s' IN STYLE '%s'\n", tag, style);

   s = _edje_edit_style_get(ed, style);
   t = _edje_edit_style_tag_get(ed, style, tag);

   s->tags = eina_list_remove(s->tags, t);
   _edje_if_string_free(ed, t->key);
   _edje_if_string_free(ed, t->value);
   _edje_if_string_free(ed, t->font);
   _edje_if_string_free(ed, t->text_class);
   free(t);
   t = NULL;
}

/*******************/
/*  EXTERNALS API  */
/*******************/

EAPI Eina_List *
edje_edit_externals_list_get(Evas_Object *obj)
{
   Eina_List *externals = NULL;
   unsigned int i;

   GET_ED_OR_RETURN(NULL);

   if (!ed->file || !ed->file->external_dir)
      return NULL;
   //printf("GET STYLES LIST %d\n", eina_list_count(ed->file->styles));
   for (i = 0; i < ed->file->external_dir->entries_count; ++i)
     externals = eina_list_append(externals,
				  eina_stringshare_add(ed->file->external_dir->entries[i].entry));

   return externals;
}

EAPI Eina_Bool
edje_edit_external_add(Evas_Object *obj, const char *external)
{
   Edje_External_Directory_Entry *e;
   unsigned int freeid;
   unsigned int i;

   GET_ED_OR_RETURN(EINA_FALSE);

   e = _edje_edit_external_get(ed, external);
   if (e) return EINA_FALSE;

   if (!ed->file->external_dir)
     ed->file->external_dir = _alloc(sizeof(Edje_External_Directory));

   for (i = 0; i < ed->file->external_dir->entries_count; ++i)
     if (!ed->file->external_dir->entries[i].entry)
       break ;

   if (i == ed->file->external_dir->entries_count)
     {
	Edje_External_Directory_Entry *tmp;
	unsigned int max;

	max = ed->file->external_dir->entries_count + 1;
	tmp = realloc(ed->file->external_dir->entries,
		      sizeof (Edje_External_Directory_Entry) * max);

	if (!tmp) return EINA_FALSE;

	ed->file->external_dir->entries = tmp;
	freeid = ed->file->external_dir->entries_count;
	ed->file->external_dir->entries_count = max;
     }
   else
     freeid = i;

   ed->file->external_dir->entries[freeid].entry = (char*)eina_stringshare_add(external);

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_external_del(Evas_Object *obj, const char *external)
{
   Edje_External_Directory_Entry *e;

   GET_ED_OR_RETURN(EINA_FALSE);

   e = _edje_edit_external_get(ed, external);
   if (!e) return EINA_FALSE;

   _edje_if_string_free(ed, e->entry);
   e->entry = NULL;

   return EINA_TRUE;
}

/***************/
/*  PARTS API  */
/***************/

EAPI Eina_List *
edje_edit_parts_list_get(Evas_Object *obj)
{
   Eina_List *parts = NULL;
   int i;

   GET_ED_OR_RETURN(NULL);

   //printf("EE: Found %d parts\n", ed->table_parts_size);

   parts = NULL;
   for (i = 0; i < ed->table_parts_size; i++)
     {
	Edje_Real_Part *rp;

	rp = ed->table_parts[i];
	parts = eina_list_append(parts, eina_stringshare_add(rp->part->name));
     }

   return parts;
}

EAPI Eina_Bool
edje_edit_part_name_set(Evas_Object *obj, const char* part, const char* new_name)
{
   GET_RP_OR_RETURN(EINA_FALSE);

   if (!new_name) return EINA_FALSE;
   if (!strcmp(part, new_name)) return EINA_TRUE;
   if (_edje_real_part_get(ed, new_name)) return EINA_FALSE;

   //printf("Set name of part: %s [new name: %s]\n", part, new_name);

   _edje_if_string_free(ed, rp->part->name);
   rp->part->name = (char *)eina_stringshare_add(new_name);

   return EINA_TRUE;
}

#define FUNC_PART_API_STRING(Value)					\
  EAPI const char *							\
  edje_edit_part_api_##Value##_get(Evas_Object *obj, const char *part)	\
  {									\
     GET_RP_OR_RETURN(NULL);						\
     return eina_stringshare_add(rp->part->api.Value);			\
  }									\
  EAPI Eina_Bool							\
  edje_edit_part_api_##Value##_set(Evas_Object *obj, const char *part, const char *s) \
  {									\
     GET_RP_OR_RETURN(EINA_FALSE);					\
     _edje_if_string_free(ed, rp->part->api.name);			\
     rp->part->api.Value = eina_stringshare_add(s);			\
     return EINA_TRUE;							\
  }

FUNC_PART_API_STRING(name);
FUNC_PART_API_STRING(description);

Eina_Bool
_edje_edit_real_part_add(Evas_Object *obj, const char *name, Edje_Part_Type type, const char *source)
{
   Edje_Part_Collection_Directory_Entry *ce;
   Edje_Part_Collection *pc;
   Edje_Part **tmp;
   Edje_Part *ep;
   Edje_Real_Part *rp;
   int id;

   GET_ED_OR_RETURN(EINA_FALSE);

   //printf("ADD PART: %s [type: %d]\n", name, type);

   /* Check if part already exists */
   if (_edje_real_part_get(ed, name))
     return EINA_FALSE;

   /* Alloc Edje_Part or return */
   ep = _alloc(sizeof(Edje_Part));
   if (!ep) return EINA_FALSE;

   /* Alloc Edje_Real_Part or return */
   rp = _alloc(sizeof(Edje_Real_Part));
   if (!rp)
     {
	free(ep);
	return EINA_FALSE;
     }

   /* Init Edje_Part */
   pc = ed->collection;

   tmp = realloc(pc->parts, (pc->parts_count + 1) * sizeof (Edje_Part *));
   if (!tmp)
     {
	free(ep);
	free(rp);
	return EINA_FALSE;
     }

   id = pc->parts_count++;

   pc->parts = tmp;
   pc->parts[id] = ep;

   ep->id = id;
   ep->type = type;
   ep->name = eina_stringshare_add(name);
   ep->mouse_events = 1;
   ep->repeat_events = 0;
   ep->ignore_flags = EVAS_EVENT_FLAG_NONE;
   ep->pointer_mode = EVAS_OBJECT_POINTER_MODE_AUTOGRAB;
   ep->precise_is_inside = 0;
   ep->use_alternate_font_metrics = 0;
   ep->clip_to_id = -1;
   ep->dragable.confine_id = -1;
   ep->dragable.event_id = -1;
   if (source)
     ep->source = eina_stringshare_add(source);

   ep->default_desc = NULL;
   ep->other.desc = NULL;
   ep->other.desc_count = 0;

   /* Init Edje_Real_Part */
   rp->edje = ed;
   _edje_ref(rp->edje);
   rp->part = ep;

   if (ep->type == EDJE_PART_TYPE_RECTANGLE)
     rp->object = evas_object_rectangle_add(ed->evas);
   else if (ep->type == EDJE_PART_TYPE_IMAGE)
     rp->object = evas_object_image_add(ed->evas);
   else if (ep->type == EDJE_PART_TYPE_TEXT)
     {
	_edje_text_part_on_add(ed, rp);
	rp->object = evas_object_text_add(ed->evas);
	evas_object_text_font_source_set(rp->object, ed->path);
     }
   else if (ep->type == EDJE_PART_TYPE_SWALLOW ||
	    ep->type == EDJE_PART_TYPE_GROUP ||
	    ep->type == EDJE_PART_TYPE_EXTERNAL)
     {
	rp->object = evas_object_rectangle_add(ed->evas);
	evas_object_color_set(rp->object, 0, 0, 0, 0);
	evas_object_pass_events_set(rp->object, 1);
	evas_object_pointer_mode_set(rp->object, EVAS_OBJECT_POINTER_MODE_NOGRAB);
     }
   else if (ep->type == EDJE_PART_TYPE_TEXTBLOCK)
     rp->object = evas_object_textblock_add(ed->evas);
   else
     ERR("wrong part type %i!", ep->type);
   if (rp->object)
     {
	evas_object_show(rp->object);
	evas_object_smart_member_add(rp->object, ed->obj);
	evas_object_layer_set(rp->object, evas_object_layer_get(ed->obj));
	if (ep->type != EDJE_PART_TYPE_SWALLOW && ep->type != EDJE_PART_TYPE_GROUP)
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
		  evas_object_pointer_mode_set(rp->object,
					       EVAS_OBJECT_POINTER_MODE_NOGRAB);
	       }
	     if (ep->precise_is_inside)
	       evas_object_precise_is_inside_set(rp->object, 1);
	  }
	if (ep->type == EDJE_PART_TYPE_EXTERNAL)
	  {
	     Evas_Object *child;
	     child = _edje_external_type_add(source, evas_object_evas_get(ed->obj), ed->obj, NULL, name);
	     if (child)
	       _edje_real_part_swallow(rp, child);
	  }
	evas_object_clip_set(rp->object, ed->clipper);
	evas_object_show(ed->clipper);
     }

   /* Update table_parts */
   ed->table_parts_size++;
   ed->table_parts = realloc(ed->table_parts,
			     sizeof(Edje_Real_Part *) * ed->table_parts_size);

   ed->table_parts[ep->id % ed->table_parts_size] = rp;

   /* Create default description */
   edje_edit_state_add(obj, name, "default", 0.0);
   edje_edit_part_selected_state_set(obj, name, "default", 0.0);

   ce = eina_hash_find(ed->file->collection, ed->group);
   ce->count.part++;

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_part_add(Evas_Object *obj, const char *name, Edje_Part_Type type)
{
   if (type == EDJE_PART_TYPE_EXTERNAL)
     return EINA_FALSE;
   return _edje_edit_real_part_add(obj, name, type, NULL);
}

EAPI Eina_Bool
edje_edit_part_external_add(Evas_Object *obj, const char *name, const char *source)
{
   if (!source)
     return EINA_FALSE;
   return _edje_edit_real_part_add(obj, name, EDJE_PART_TYPE_EXTERNAL, source);
}

EAPI Eina_Bool
edje_edit_part_del(Evas_Object *obj, const char* part)
{
   Edje_Part_Collection_Directory_Entry *ce;
   Edje_Part_Collection *pc;
   Edje_Part *ep;
   unsigned int k;
   unsigned int id;
   unsigned int i;

   GET_RP_OR_RETURN(EINA_FALSE);

   //printf("REMOVE PART: %s\n", part);

   ep = rp->part;
   id = ep->id;

   /* Unlik Edje_Real_Parts that link to the removed one */
   for (i = 0; i < ed->table_parts_size; i++)
     {
	Edje_Real_Part *real;

	if (i == id) continue; //don't check the deleted id
	real = ed->table_parts[i];

	if (real->text.source == rp) real->text.source = NULL;
	if (real->text.text_source == rp) real->text.text_source = NULL;

	if (real->param1.rel1_to_x == rp) real->param1.rel1_to_x = NULL;
	if (real->param1.rel1_to_y == rp) real->param1.rel1_to_y = NULL;
	if (real->param1.rel2_to_x == rp) real->param1.rel2_to_x = NULL;
	if (real->param1.rel2_to_y == rp) real->param1.rel2_to_y = NULL;

	if (real->param2)
	  {
	     if (real->param2->rel1_to_x == rp) real->param2->rel1_to_x = NULL;
	     if (real->param2->rel1_to_y == rp) real->param2->rel1_to_y = NULL;
	     if (real->param2->rel2_to_x == rp) real->param2->rel2_to_x = NULL;
	     if (real->param2->rel2_to_y == rp) real->param2->rel2_to_y = NULL;
	  }

	if (real->custom)
	  {
	     if (real->custom->rel1_to_x == rp) real->custom->rel1_to_x = NULL;
	     if (real->custom->rel1_to_y == rp) real->custom->rel1_to_y = NULL;
	     if (real->custom->rel2_to_x == rp) real->custom->rel2_to_x = NULL;
	     if (real->custom->rel2_to_y == rp) real->custom->rel2_to_y = NULL;
	  }

	if (real->clip_to == rp)
	  {
	     evas_object_clip_set(real->object, ed->clipper);
	     real->clip_to = NULL;
	  }
	if (real->drag && real->drag->confine_to == rp)
	  real->drag->confine_to = NULL;
     }

   /* Unlink all the parts and descriptions that refer to id */
   _edje_part_id_set(ed, rp, -1);

   /* Remove part from parts list */
   pc = ed->collection;
   pc->parts_count--;
   if (id < pc->parts_count) /* Forward parts */
     {
	int mcount = (pc->parts_count - id) * sizeof(Edje_Part *);
	memmove(&pc->parts[id], &pc->parts[id+1], mcount);
     }
   pc->parts[pc->parts_count] = NULL;
   _edje_fix_parts_id(ed);

   /* Free Edje_Part and all descriptions */
   ce = eina_hash_find(ed->file->collection, ed->group);

   _edje_if_string_free(ed, ep->name);
   if (ep->default_desc)
     {
	_edje_collection_free_part_description_free(ep->type, ep->default_desc, ce, 0);
	ep->default_desc = NULL;
     }

   for (k = 0; k < ep->other.desc_count; ++k)
     _edje_collection_free_part_description_free(ep->type, ep->other.desc[k], ce, 0);

   free(ep->other.desc);
   eina_mempool_free(ce->mp.part, ep);

   /* Free Edje_Real_Part */
   _edje_real_part_free(rp);

   /* if all parts are gone, hide the clipper */
   if (ed->table_parts_size == 0)
     evas_object_hide(ed->clipper);

   edje_object_calc_force(obj);

   ce->count.part--;

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_part_exist(Evas_Object *obj, const char *part)
{
   GET_RP_OR_RETURN(EINA_FALSE);
   return EINA_TRUE;
}

EAPI const char*
edje_edit_part_below_get(Evas_Object *obj, const char* part)
{
   Edje_Real_Part *prev;

   GET_RP_OR_RETURN(0);

   if (rp->part->id < 1) return NULL;

   prev = ed->table_parts[(rp->part->id - 1) % ed->table_parts_size];

   return eina_stringshare_add(prev->part->name);
}

EAPI const char*
edje_edit_part_above_get(Evas_Object *obj, const char* part)
{
   Edje_Real_Part *next;

   GET_RP_OR_RETURN(0);

   if (rp->part->id >= ed->table_parts_size - 1) return 0;

   next = ed->table_parts[(rp->part->id + 1) % ed->table_parts_size];

   return eina_stringshare_add(next->part->name);
}

EAPI Eina_Bool
edje_edit_part_restack_below(Evas_Object *obj, const char* part)
{
   Edje_Part_Collection *group;
   Edje_Real_Part *prev;
   Edje_Part *swap;

   GET_RP_OR_RETURN(EINA_FALSE);

   //printf("RESTACK PART: %s BELOW\n", part);

   if (rp->part->id < 1) return EINA_FALSE;
   group = ed->collection;

   /* update parts list */
   prev = ed->table_parts[(rp->part->id - 1) % ed->table_parts_size];

   swap = group->parts[rp->part->id];
   group->parts[rp->part->id] = group->parts[prev->part->id];
   group->parts[prev->part->id] = swap;

   _edje_parts_id_switch(ed, rp, prev);

   evas_object_stack_below(rp->object, prev->object);
   if (rp->swallowed_object)
     evas_object_stack_above(rp->swallowed_object, rp->object);

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_part_restack_above(Evas_Object *obj, const char* part)
{
   Edje_Part_Collection *group;
   Edje_Real_Part *next;
   Edje_Part *swap;

   GET_RP_OR_RETURN(EINA_FALSE);

   //printf("RESTACK PART: %s ABOVE\n", part);

   if (rp->part->id >= ed->table_parts_size - 1) return EINA_FALSE;

   group = ed->collection;

   /* update parts list */
   next = ed->table_parts[(rp->part->id + 1) % ed->table_parts_size];

   swap = group->parts[rp->part->id];
   group->parts[rp->part->id] = group->parts[next->part->id];
   group->parts[next->part->id] = swap;

   /* update ids */
   _edje_parts_id_switch(ed, rp, next);

   evas_object_stack_above(rp->object, next->object);
   if (rp->swallowed_object)
     evas_object_stack_above(rp->swallowed_object, rp->object);

   return EINA_TRUE;
}

EAPI Edje_Part_Type
edje_edit_part_type_get(Evas_Object *obj, const char *part)
{
   GET_RP_OR_RETURN(0);

   return rp->part->type;
}

EAPI const char *
edje_edit_part_selected_state_get(Evas_Object *obj, const char *part, double *value)
{
   GET_RP_OR_RETURN(NULL);

   if (!rp->chosen_description)
     {
	if (value) *value = 0.0; // FIXME: Make sure edje_edit supports correctly the default having any value
	return eina_stringshare_add("default");
     }

   if (value) *value = rp->chosen_description->state.value;
   return eina_stringshare_add(rp->chosen_description->state.name);
}

EAPI Eina_Bool
edje_edit_part_selected_state_set(Evas_Object *obj, const char *part, const char *state, double value)
{
   Edje_Part_Description_Common *pd;

   GET_RP_OR_RETURN(EINA_FALSE);

   pd = _edje_part_description_find_byname(eed, part, state, value);
   if (!pd) return EINA_FALSE;

   //printf("EDJE: Set state: %s %f\n", pd->state.name, pd->state.value);
   _edje_part_description_apply(ed, rp, pd->state.name, pd->state.value, NULL, 0.0);

   edje_object_calc_force(obj);
   return EINA_TRUE;
}

EAPI const char *
edje_edit_part_clip_to_get(Evas_Object *obj, const char *part)
{
   Edje_Real_Part *clip = NULL;

   GET_RP_OR_RETURN(NULL);

   //printf("Get clip_to for part: %s [to_id: %d]\n", part, rp->part->clip_to_id);
   if (rp->part->clip_to_id < 0) return NULL;

   clip = ed->table_parts[rp->part->clip_to_id % ed->table_parts_size];
   if (!clip || !clip->part || !clip->part->name) return NULL;

   return eina_stringshare_add(clip->part->name);
}

EAPI Eina_Bool
edje_edit_part_clip_to_set(Evas_Object *obj, const char *part, const char *clip_to)
{
   Edje_Real_Part *clip;
   Evas_Object *o, *oo;

   GET_RP_OR_RETURN(EINA_FALSE);

   /* unset clipping */
   if (!clip_to)
     {
	//printf("UnSet clip_to for part: %s\n", part);

	if (rp->clip_to && rp->clip_to->object)
	  {
	     evas_object_pointer_mode_set(rp->clip_to->object,
					  EVAS_OBJECT_POINTER_MODE_AUTOGRAB);
	     evas_object_clip_unset(rp->object);
	  }

	evas_object_clip_set(rp->object, ed->clipper);
	if (rp->swallowed_object)
	  evas_object_clip_set(rp->swallowed_object, ed->clipper);

	rp->part->clip_to_id = -1;
	rp->clip_to = NULL;

	edje_object_calc_force(obj);

	return EINA_TRUE;
     }

   /* set clipping */
   //printf("Set clip_to for part: %s [to: %s]\n", part, clip_to);
   clip = _edje_real_part_get(ed, clip_to);
   if (!clip || !clip->part) return EINA_FALSE;
   o = clip->object;
   while ((oo = evas_object_clip_get(o)))
     {
	if (o == rp->object)
	  return EINA_FALSE;
	o = oo;
     }

   rp->part->clip_to_id = clip->part->id;
   rp->clip_to = clip;

   evas_object_pass_events_set(rp->clip_to->object, 1);
   evas_object_pointer_mode_set(rp->clip_to->object, EVAS_OBJECT_POINTER_MODE_NOGRAB);
   evas_object_clip_set(rp->object, rp->clip_to->object);
   if (rp->swallowed_object)
     evas_object_clip_set(rp->swallowed_object, rp->clip_to->object);

   edje_object_calc_force(obj);

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_part_mouse_events_get(Evas_Object *obj, const char *part)
{
   GET_RP_OR_RETURN(EINA_FALSE);
   //printf("Get mouse_events for part: %s [%d]\n", part, rp->part->mouse_events);
   return rp->part->mouse_events;
}

EAPI void
edje_edit_part_mouse_events_set(Evas_Object *obj, const char *part, Eina_Bool mouse_events)
{
   GET_RP_OR_RETURN();

   if (!rp->object) return;

   //printf("Set mouse_events for part: %s [%d]\n", part, mouse_events);

   rp->part->mouse_events = mouse_events ? 1 : 0;

   if (mouse_events)
     {
	evas_object_pass_events_set(rp->object, 0);
	_edje_callbacks_add(rp->object, ed, rp);
     }
   else
     {
	evas_object_pass_events_set(rp->object, 1);
	_edje_callbacks_del(rp->object, ed);
     }
}

EAPI Eina_Bool
edje_edit_part_repeat_events_get(Evas_Object *obj, const char *part)
{
   GET_RP_OR_RETURN(EINA_FALSE);

   //printf("Get repeat_events for part: %s [%d]\n", part, rp->part->repeat_events);
   return rp->part->repeat_events;
}

EAPI void
edje_edit_part_repeat_events_set(Evas_Object *obj, const char *part, Eina_Bool repeat_events)
{
   GET_RP_OR_RETURN();

   if (!rp->object) return;

   //printf("Set repeat_events for part: %s [%d]\n", part, repeat_events);

   rp->part->repeat_events = repeat_events ? 1 : 0;

   if (repeat_events)
     evas_object_repeat_events_set(rp->object, 1);
   else
     evas_object_repeat_events_set(rp->object, 0);
}

EAPI Evas_Event_Flags
edje_edit_part_ignore_flags_get(Evas_Object *obj, const char *part)
{
   GET_RP_OR_RETURN(0);

   return rp->part->ignore_flags;
}

EAPI void
edje_edit_part_ignore_flags_set(Evas_Object *obj, const char *part, Evas_Event_Flags ignore_flags)
{
   GET_RP_OR_RETURN();

   if (!rp->object) return;
   //printf("Set ignore_flags for part: %s [%#x]\n", part, ignore_flags);

   rp->part->ignore_flags = ignore_flags;
}

EAPI void
edje_edit_part_scale_set(Evas_Object *obj, const char *part, Eina_Bool scale)
{
   GET_RP_OR_RETURN();

   rp->part->scale = scale;
   edje_object_calc_force(obj);
}

EAPI Eina_Bool
edje_edit_part_scale_get(Evas_Object *obj, const char *part)
{
   GET_RP_OR_RETURN(EINA_FALSE);

   return rp->part->scale;
}

EAPI const char *
edje_edit_part_source_get(Evas_Object *obj, const char *part)
{
   //Edje_Real_Part *clip = NULL;

   GET_RP_OR_RETURN(NULL);

   //printf("Get source for part: %s\n", part);
   if (!rp->part->source) return NULL;

   return eina_stringshare_add(rp->part->source);
}

EAPI Eina_Bool
edje_edit_part_source_set(Evas_Object *obj, const char *part, const char *source)
{
   GET_RP_OR_RETURN(EINA_FALSE);

   Evas_Object *child_obj;
   //printf("Set source for part: %s [source: %s]\n", part, source);

   if (rp->part->type == EDJE_PART_TYPE_EXTERNAL)
     return EINA_FALSE;

   _edje_if_string_free(ed, rp->part->source);

   if (rp->swallowed_object)
     {
       _edje_real_part_swallow_clear(rp);
       evas_object_del(rp->swallowed_object);
       rp->swallowed_object = NULL;
     }
   if (source)
     {
	rp->part->source = eina_stringshare_add(source);
	child_obj = edje_object_add(ed->evas);
	edje_object_file_set(child_obj, ed->file->path, source);
	_edje_real_part_swallow(rp, child_obj);
     }
   else
     rp->part->source = NULL;
   return EINA_TRUE;
}

EAPI int
edje_edit_part_drag_x_get(Evas_Object *obj, const char *part)
{
   GET_RP_OR_RETURN(0);
   //printf("Get dragX for part: %s\n", part);
   return rp->part->dragable.x;
}

EAPI void
edje_edit_part_drag_x_set(Evas_Object *obj, const char *part, int drag)
{
   GET_RP_OR_RETURN();
   //printf("Set dragX for part: %s\n", part);
   rp->part->dragable.x = drag;

   if (!drag && !rp->part->dragable.y)
     {
	free(rp->drag);
	rp->drag = NULL;
	return ;
     }

   if (rp->drag) return;

   rp->drag = _alloc(sizeof (Edje_Real_Part_Drag));
   if (!rp->drag) return;

   rp->drag->step.x = rp->part->dragable.step_x;
   rp->drag->step.y = rp->part->dragable.step_y;
}

EAPI int
edje_edit_part_drag_y_get(Evas_Object *obj, const char *part)
{
   GET_RP_OR_RETURN(0);
   //printf("Get dragY for part: %s\n", part);
   return rp->part->dragable.y;
}

EAPI void
edje_edit_part_drag_y_set(Evas_Object *obj, const char *part, int drag)
{
   GET_RP_OR_RETURN();
   //printf("Set dragY for part: %s\n", part);
   rp->part->dragable.y = drag;

   if (!drag && !rp->part->dragable.x)
     {
	free(rp->drag);
	rp->drag = NULL;
	return ;
     }

   if (rp->drag) return;

   rp->drag = _alloc(sizeof (Edje_Real_Part_Drag));
   if (!rp->drag) return;

   rp->drag->step.x = rp->part->dragable.step_x;
   rp->drag->step.y = rp->part->dragable.step_y;
}

#define FUNC_PART_DRAG_INT(Class, Value)				\
  EAPI int								\
  edje_edit_part_drag_##Class##_##Value##_get(Evas_Object *obj, const char *part) \
  {									\
     GET_RP_OR_RETURN(0);						\
     return rp->part->dragable.Class##_##Value;				\
  }									\
  EAPI void								\
  edje_edit_part_drag_##Class##_##Value##_set(Evas_Object *obj, const char *part, int v) \
  {									\
     GET_RP_OR_RETURN();						\
     rp->part->dragable.Class##_##Value = v;				\
  }

FUNC_PART_DRAG_INT(step, x);
FUNC_PART_DRAG_INT(step, y);
FUNC_PART_DRAG_INT(count, x);
FUNC_PART_DRAG_INT(count, y);

#define FUNC_PART_DRAG_ID(Id)			\
  EAPI const char*				\
  edje_edit_part_drag_##Id##_get(Evas_Object *obj, const char *part)	\
  {									\
     Edje_Real_Part *p;							\
									\
     GET_RP_OR_RETURN(NULL);						\
									\
     if (rp->part->dragable.Id##_id < 0)				\
       return NULL;							\
     									\
     p = ed->table_parts[rp->part->dragable.Id##_id];			\
     return eina_stringshare_add(p->part->name);			\
  }									\
  EAPI void								\
  edje_edit_part_drag_##Id##_set(Evas_Object *obj, const char *part, const char *e) \
  {									\
     Edje_Real_Part *e_part;						\
									\
     GET_RP_OR_RETURN();						\
     if (!e)								\
       {								\
	  rp->part->dragable.Id##_id = -1;				\
	  return ;							\
       }								\
									\
     e_part = _edje_real_part_get(ed, e);				\
     rp->part->dragable.Id##_id = e_part->part->id;			\
  }

FUNC_PART_DRAG_ID(confine);
FUNC_PART_DRAG_ID(event);

/*********************/
/*  PART STATES API  */
/*********************/
EAPI Eina_List *
edje_edit_part_states_list_get(Evas_Object *obj, const char *part)
{
   char state_name[PATH_MAX];
   Eina_List *states = NULL;
   unsigned int i;

   GET_RP_OR_RETURN(NULL);

   //Is there a better place to put this? maybe edje_edit_init() ?
#ifdef HAVE_LOCALE_H
   setlocale(LC_NUMERIC, "C");
#endif

   states = NULL;

   //append default state
   snprintf(state_name, PATH_MAX,
            "%s %.2f",
	    rp->part->default_desc->state.name,
	    rp->part->default_desc->state.value);
   states = eina_list_append(states, eina_stringshare_add(state_name));
   //printf("NEW STATE def: %s\n", state->state.name);

   //append other states
   for (i = 0; i < rp->part->other.desc_count; ++i)
     {
	snprintf(state_name, sizeof(state_name),
	         "%s %.2f",
		 rp->part->other.desc[i]->state.name,
		 rp->part->other.desc[i]->state.value);
	states = eina_list_append(states, eina_stringshare_add(state_name));
	//printf("NEW STATE: %s\n", state_name);
     }
   return states;
}

EAPI Eina_Bool
edje_edit_state_name_set(Evas_Object *obj, const char *part, const char *state, double value, const char *new_name, double new_value)
{
   int part_id;
   int i;

   GET_PD_OR_RETURN(EINA_FALSE);
   //printf("Set name of state: %s in part: %s [new name: %s]\n",
     //     part, state, new_name);

   if (!new_name) return EINA_FALSE;

   /* update programs */
   /* update the 'state' field in all programs. update only if program has
      a single target */
   part_id = _edje_part_id_find(ed, part);
   for (i = 0; i < ed->table_programs_size; i++)
     {
	Edje_Program *epr = ed->table_programs[i];

	if (eina_list_count(epr->targets) == 1)
	  {
	     Edje_Program_Target *t = eina_list_data_get(epr->targets);

	     if (t->id == part_id &&
		 !strcmp(epr->state, pd->state.name) &&
		 pd->state.value == epr->value)
	       {
		  _edje_if_string_free(ed, epr->state);
		  epr->state = eina_stringshare_add(new_name);
		  epr->value = value;
	       }
	  }
     }

   /* set name */
   _edje_if_string_free(ed, pd->state.name);
   pd->state.name = (char *)eina_stringshare_add(new_name);
   /* set value */
   pd->state.value = new_value;

   return EINA_TRUE;
}

EAPI void
edje_edit_state_del(Evas_Object *obj, const char *part, const char *state, double value)
{
   Edje_Part_Collection_Directory_Entry *ce;
   Edje_Part_Description_Common *pd;
   unsigned int i;

   GET_RP_OR_RETURN();

   pd = _edje_part_description_find_byname(eed, part, state, value);
   if (!pd) return;

   /* Don't allow to delete default state, for now at least; */
   if (pd == rp->part->default_desc)
     return;

   /* And if we are deleting the current state, go back to default first */
   if (pd == rp->chosen_description)
     _edje_part_description_apply(ed, rp, "default", 0.0, NULL, 0.0);

   ce = eina_hash_find(ed->file->collection, ed->group);

   for (i = 0; i < rp->part->other.desc_count; ++i)
     if (pd == rp->part->other.desc[i])
       {
	  memmove(rp->part->other.desc + i,
		  rp->part->other.desc + i + 1,
		  sizeof (Edje_Part_Description_Common*) * (rp->part->other.desc_count - i - 1));
	  rp->part->other.desc_count--;
	  break;
       }

   _edje_collection_free_part_description_free(rp->part->type, pd, ce, 0);
}

static Edje_Part_Description_Common *
_edje_edit_state_alloc(int type, Edje *ed)
{
   Edje_Part_Collection_Directory_Entry *ce;
   Edje_Part_Description_Common *pd = NULL;

   ce = eina_hash_find(ed->file->collection, ed->group);

   switch (type)
     {
      case EDJE_PART_TYPE_RECTANGLE:
	 pd = eina_mempool_malloc(ce->mp.RECTANGLE, sizeof (Edje_Part_Description_Common));
	 ce->count.RECTANGLE++;
	 break;
      case EDJE_PART_TYPE_SWALLOW:
	 pd = eina_mempool_malloc(ce->mp.SWALLOW, sizeof (Edje_Part_Description_Common));
	 ce->count.SWALLOW++;
	 break;
      case EDJE_PART_TYPE_GROUP:
	 pd = eina_mempool_malloc(ce->mp.GROUP, sizeof (Edje_Part_Description_Common));
	 ce->count.GROUP++;
	 break;

#define EDIT_ALLOC_POOL(Short, Type, Name)				\
	 case EDJE_PART_TYPE_##Short:					\
	   {								\
	      Edje_Part_Description_##Type *Name;			\
	      								\
	      Name = eina_mempool_malloc(ce->mp.Short,			\
					 sizeof (Edje_Part_Description_##Type)); \
              memset(Name, 0, sizeof(Edje_Part_Description_##Type));    \
	      pd = &Name->common;					\
	      ce->count.Short++;					\
	      break;							\
	   }

	 EDIT_ALLOC_POOL(IMAGE, Image, image);
	 EDIT_ALLOC_POOL(TEXT, Text, text);
	 EDIT_ALLOC_POOL(TEXTBLOCK, Text, text);
	 EDIT_ALLOC_POOL(BOX, Box, box);
	 EDIT_ALLOC_POOL(TABLE, Table, table);
	 EDIT_ALLOC_POOL(EXTERNAL, External, external_params);
     }

   return pd;
}

EAPI void
edje_edit_state_add(Evas_Object *obj, const char *part, const char *name, double value)
{
   Edje_Part_Description_Common *pd;

   GET_RP_OR_RETURN();

   //printf("ADD STATE: %s TO PART: %s\n", name , part);
   pd = _edje_edit_state_alloc(rp->part->type, ed);
   if (!pd) return;

   if (!rp->part->default_desc)
     {
	rp->part->default_desc = pd;
     }
   else
     {
	Edje_Part_Description_Common **tmp;

	tmp = realloc(rp->part->other.desc,
		      sizeof (Edje_Part_Description_Common *) * (rp->part->other.desc_count + 1));
	if (!tmp)
	  {
	     free(pd);
	     return;
	  }
	rp->part->other.desc = tmp;
	rp->part->other.desc[rp->part->other.desc_count++] = pd;
     }

   memset(pd, 0, sizeof (*pd));

   pd->state.name = eina_stringshare_add(name);
   pd->state.value = value;
   pd->visible = 1;
   pd->align.x = 0.5;
   pd->align.y = 0.5;
   pd->min.w = 0;
   pd->min.h = 0;
   pd->fixed.w = 0;
   pd->fixed.h = 0;
   pd->max.w = -1;
   pd->max.h = -1;
   pd->rel1.relative_x = 0.0;
   pd->rel1.relative_y = 0.0;
   pd->rel1.offset_x = 0;
   pd->rel1.offset_y = 0;
   pd->rel1.id_x = -1;
   pd->rel1.id_y = -1;
   pd->rel2.relative_x = 1.0;
   pd->rel2.relative_y = 1.0;
   pd->rel2.offset_x = -1;
   pd->rel2.offset_y = -1;
   pd->rel2.id_x = -1;
   pd->rel2.id_y = -1;
   pd->color_class = NULL;
   pd->color.r = 255;
   pd->color.g = 255;
   pd->color.b = 255;
   pd->color.a = 255;
   pd->color2.r = 0;
   pd->color2.g = 0;
   pd->color2.b = 0;
   pd->color2.a = 255;
   pd->map.id_persp = -1;
   pd->map.id_light = -1;
   pd->map.rot.id_center = -1;
   pd->map.rot.x = FROM_DOUBLE(0.0);
   pd->map.rot.y = FROM_DOUBLE(0.0);
   pd->map.rot.z = FROM_DOUBLE(0.0);
   pd->map.on = 0;
   pd->map.smooth = 1;
   pd->map.alpha = 1;
   pd->map.backcull = 0;
   pd->map.persp_on = 0;
   pd->persp.zplane = 0;
   pd->persp.focal = 1000;

   if (rp->part->type == EDJE_PART_TYPE_TEXT
       || rp->part->type == EDJE_PART_TYPE_TEXTBLOCK)
     {
	Edje_Part_Description_Text *text;

	text = (Edje_Part_Description_Text*) pd;

	memset(&text->text, 0, sizeof (text->text));

	text->text.color3.r = 0;
	text->text.color3.g = 0;
	text->text.color3.b = 0;
	text->text.color3.a = 128;
	text->text.align.x = 0.5;
	text->text.align.y = 0.5;
	text->text.id_source = -1;
	text->text.id_text_source = -1;
     }
   else if (rp->part->type == EDJE_PART_TYPE_IMAGE)
     {
	Edje_Part_Description_Image *img;

	img = (Edje_Part_Description_Image*) pd;

	memset(&img->image, 0, sizeof (img->image));

	img->image.id = -1;
	img->image.fill.smooth = 1;
	img->image.fill.pos_rel_x = 0.0;
	img->image.fill.pos_abs_x = 0;
	img->image.fill.rel_x = 1.0;
	img->image.fill.abs_x = 0;
	img->image.fill.pos_rel_y = 0.0;
	img->image.fill.pos_abs_y = 0;
	img->image.fill.rel_y = 1.0;
	img->image.fill.abs_y = 0;
	img->image.fill.angle = 0;
	img->image.fill.spread = 0;
	img->image.fill.type = EDJE_FILL_TYPE_SCALE;
     }
   else if (rp->part->type == EDJE_PART_TYPE_EXTERNAL)
     {
	Edje_Part_Description_External *external;
	Edje_External_Param_Info *pi;

	external = (Edje_Part_Description_External*) pd;

        external->external_params = NULL;

	if (rp->part->source)
	  {
	     pi = (Edje_External_Param_Info *)edje_external_param_info_get(rp->part->source);
	     while (pi && pi->name)
	       {
		  Edje_External_Param *p;
		  p = _alloc(sizeof(Edje_External_Param));
		  /* error checking.. meh */
		  p->name = eina_stringshare_add(pi->name);
		  p->type = pi->type;
		  switch(p->type)
		    {
		     case EDJE_EXTERNAL_PARAM_TYPE_INT:
		     case EDJE_EXTERNAL_PARAM_TYPE_BOOL:
			if (pi->info.i.def != EDJE_EXTERNAL_INT_UNSET)
			  p->i = pi->info.i.def;
			break;
		     case EDJE_EXTERNAL_PARAM_TYPE_DOUBLE:
			if (pi->info.d.def != EDJE_EXTERNAL_DOUBLE_UNSET)
			  p->d = pi->info.d.def;
			break;
		     case EDJE_EXTERNAL_PARAM_TYPE_CHOICE:
			if (pi->info.c.def)
			  p->s = eina_stringshare_add(pi->info.c.def);
			break;
		     case EDJE_EXTERNAL_PARAM_TYPE_STRING:
			if (pi->info.s.def)
			  p->s = eina_stringshare_add(pi->info.s.def);
			break;
		     default:
			ERR("unknown external parameter type '%d'", p->type);
		    }
		  external->external_params = eina_list_append(external->external_params, p);
		  pi++;
	       }
	     if (external->external_params)
	       rp->param1.external_params = _edje_external_params_parse(rp->swallowed_object, external->external_params);
	  }
     }
   else if (rp->part->type == EDJE_PART_TYPE_BOX)
     {
	Edje_Part_Description_Box *box;

	box = (Edje_Part_Description_Box*) pd;
	memset(&box->box, 0, sizeof (box->box));
     }
   else if (rp->part->type == EDJE_PART_TYPE_TABLE)
     {
	Edje_Part_Description_Table *table;

	table = (Edje_Part_Description_Table*) pd;
	memset(&table->table, 0, sizeof (table->table));
     }
}

EAPI Eina_Bool
edje_edit_state_exist(Evas_Object *obj, const char *part, const char *state, double value)
{
   GET_PD_OR_RETURN(EINA_FALSE);
   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_state_copy(Evas_Object *obj, const char *part, const char *from, double val_from, const char *to, double val_to)
{
   Edje_Part_Description_Common *pdfrom, *pdto;
   Edje_External_Param *p;

   GET_RP_OR_RETURN(EINA_FALSE);

   pdfrom = _edje_part_description_find_byname(eed, part, from, val_from);
   if (!pdfrom)
     return EINA_FALSE;

   pdto = _edje_part_description_find_byname(eed, part, to, val_to);
   if (!pdto)
     {
	Edje_Part_Description_Common **tmp;

	pdto = _edje_edit_state_alloc(rp->part->type, ed);
	if (!pdto) return EINA_FALSE;
	/* No need to check for default desc, at this point it must exist */

	tmp = realloc(rp->part->other.desc,
		      sizeof (Edje_Part_Description_Common *) * (rp->part->other.desc_count + 1));
	if (!tmp)
	  {
	     free(pdto);
	     return EINA_FALSE;
	  }
	rp->part->other.desc = tmp;
	rp->part->other.desc[rp->part->other.desc_count++] = pdto;
     }

#define PD_STRING_COPY(To, From, _x)			\
   _edje_if_string_free(ed, To->_x);			\
   To->_x = (char *)eina_stringshare_add(From->_x);

   /* Copy all value */
   *pdto = *pdfrom;
   /* Keeping the pdto state name and value */
   pdto->state.name = eina_stringshare_add(to);
   pdto->state.value = val_to;
   /* Update pointer. */
   PD_STRING_COPY(pdto, pdfrom, color_class);

   switch (rp->part->type)
     {
      case EDJE_PART_TYPE_IMAGE:
	{
	   Edje_Part_Description_Image *img_to = (Edje_Part_Description_Image*) pdto;
	   Edje_Part_Description_Image *img_from = (Edje_Part_Description_Image*) pdfrom;
	   unsigned int i;

	   img_to->image = img_from->image;

	   /* Update pointers. */
	   for (i = 0; i < img_to->image.tweens_count; ++i)
	     free(img_to->image.tweens[i]);
	   if (img_to->image.tweens_count > 0)
		free(img_to->image.tweens);

	   img_to->image.tweens_count = img_from->image.tweens_count;
	   img_to->image.tweens = calloc(img_to->image.tweens_count,
					 sizeof (Edje_Part_Image_Id*));
	   if (!img_to->image.tweens)
	     break;

	   for (i = 0; i < img_to->image.tweens_count; ++i)
	     {
		Edje_Part_Image_Id *new_i;
		new_i = _alloc(sizeof(Edje_Part_Image_Id));
		if (!new_i) continue ;

		*new_i = *img_from->image.tweens[i];

		img_to->image.tweens[i] = new_i;
	     }
	   break;
	}
      case EDJE_PART_TYPE_TEXT:
      case EDJE_PART_TYPE_TEXTBLOCK:
	{
	   Edje_Part_Description_Text *text_to = (Edje_Part_Description_Text*) pdto;
	   Edje_Part_Description_Text *text_from = (Edje_Part_Description_Text*) pdfrom;

	   text_to->text = text_from->text;

	   /* Update pointers. */
	   PD_STRING_COPY(text_to, text_from, text.text);
	   PD_STRING_COPY(text_to, text_from, text.text_class);
	   PD_STRING_COPY(text_to, text_from, text.style);
	   PD_STRING_COPY(text_to, text_from, text.font);
	   PD_STRING_COPY(text_to, text_from, text.repch);
	   break;
	}
      case EDJE_PART_TYPE_BOX:
	{
	   Edje_Part_Description_Box *box_to = (Edje_Part_Description_Box*) pdto;
	   Edje_Part_Description_Box *box_from = (Edje_Part_Description_Box*) pdfrom;

	   box_to->box = box_from->box;

	   PD_STRING_COPY(box_to, box_from, box.layout);
	   PD_STRING_COPY(box_to, box_from, box.alt_layout);
	   break;
	}
      case EDJE_PART_TYPE_TABLE:
	{
	   Edje_Part_Description_Table *table_to = (Edje_Part_Description_Table*) pdto;
	   Edje_Part_Description_Table *table_from = (Edje_Part_Description_Table*) pdfrom;

	   table_to->table = table_from->table;
	   break;
	}
      case EDJE_PART_TYPE_EXTERNAL:
	{
	   Edje_Part_Description_External *ext_to = (Edje_Part_Description_External*) pdto;
	   Edje_Part_Description_External *ext_from = (Edje_Part_Description_External*) pdfrom;
	   Eina_List *l;

	   /* XXX: optimize this, most likely we don't need to remove and add */
	   EINA_LIST_FREE(ext_to->external_params, p)
	     {
		_edje_if_string_free(ed, p->name);
		if (p->s)
		  _edje_if_string_free(ed, p->s);
		free(p);
	     }
	   EINA_LIST_FOREACH(ext_from->external_params, l, p)
	     {
		Edje_External_Param *new_p;
		new_p = _alloc(sizeof(Edje_External_Param));
		new_p->name = eina_stringshare_add(p->name);
		new_p->type = p->type;
		switch (p->type)
		  {
		   case EDJE_EXTERNAL_PARAM_TYPE_INT:
		      new_p->i = p->i;
		      break;
		   case EDJE_EXTERNAL_PARAM_TYPE_DOUBLE:
		      new_p->d = p->d;
		      break;
		   case EDJE_EXTERNAL_PARAM_TYPE_CHOICE:
		   case EDJE_EXTERNAL_PARAM_TYPE_STRING:
		      new_p->s = eina_stringshare_add(p->s);
		      break;
		   default:
		      break;
		  }
		ext_to->external_params = eina_list_append(ext_to->external_params, new_p);
	     }
	   break;
	}
     }

#undef PD_STRING_COPY

   return EINA_TRUE;
}

#define FUNC_STATE_RELATIVE_DOUBLE(Sub, Value)				\
  EAPI double								\
  edje_edit_state_##Sub##_relative_##Value##_get(Evas_Object *obj, const char *part, const char *state, double value) \
  {									\
     GET_PD_OR_RETURN(0);						\
     return TO_DOUBLE(pd->Sub.relative_##Value);			\
  }									\
  EAPI void								\
  edje_edit_state_##Sub##_relative_##Value##_set(Evas_Object *obj, const char *part, const char *state, double value, double v) \
  {									\
     GET_PD_OR_RETURN();						\
     pd->Sub.relative_##Value = FROM_DOUBLE(v);			\
     edje_object_calc_force(obj);				\
  }

FUNC_STATE_RELATIVE_DOUBLE(rel1, x);
FUNC_STATE_RELATIVE_DOUBLE(rel1, y);
FUNC_STATE_RELATIVE_DOUBLE(rel2, x);
FUNC_STATE_RELATIVE_DOUBLE(rel2, y);

#define FUNC_STATE_OFFSET_INT(Sub, Value)					\
  EAPI int								\
  edje_edit_state_##Sub##_offset_##Value##_get(Evas_Object *obj, const char *part, const char *state, double value) \
  {									\
     GET_PD_OR_RETURN(0);						\
     return pd->Sub.offset_##Value;				\
  }									\
  EAPI void								\
  edje_edit_state_##Sub##_offset_##Value##_set(Evas_Object *obj, const char *part, const char *state, double value, double v) \
  {									\
     GET_PD_OR_RETURN();						\
     pd->Sub.offset_##Value = TO_INT(FROM_DOUBLE(v));		\
     edje_object_calc_force(obj);					\
  }

FUNC_STATE_OFFSET_INT(rel1, x);
FUNC_STATE_OFFSET_INT(rel1, y);
FUNC_STATE_OFFSET_INT(rel2, x);
FUNC_STATE_OFFSET_INT(rel2, y);

#define FUNC_STATE_REL(Sub, Value)		\
  EAPI const char *				\
  edje_edit_state_##Sub##_to_##Value##_get(Evas_Object *obj, const char *part, const char *state, double value)	\
  {									\
     Edje_Real_Part *rel;						\
									\
     GET_PD_OR_RETURN(NULL);						\
									\
     if (pd->Sub.id_##Value == -1) return NULL;			\
									\
     rel = ed->table_parts[pd->Sub.id_##Value % ed->table_parts_size]; \
     									\
     if (rel->part->name) return eina_stringshare_add(rel->part->name);	\
     return NULL;							\
  }									\
  EAPI void								\
  edje_edit_state_##Sub##_to_##Value##_set(Evas_Object *obj, const char *part, const char *state, double value, const char *to)	\
  {									\
     Edje_Real_Part *relp;						\
									\
     GET_PD_OR_RETURN();						\
									\
     if (to)								\
       {								\
	  relp = _edje_real_part_get(ed, to);				\
	  if (!relp) return;						\
	  pd->Sub.id_##Value = relp->part->id;			\
       }								\
     else								\
       pd->Sub.id_##Value = -1;					\
									\
  }
//note after this call edje_edit_part_selected_state_set() to update !! need to fix this
//_edje_part_description_apply(ed, rp, pd->state.name, pd->state.value, "state", 0.1); //Why segfault??
// edje_object_calc_force(obj);//don't work for redraw

FUNC_STATE_REL(rel1, x);
FUNC_STATE_REL(rel1, y);
FUNC_STATE_REL(rel2, x);
FUNC_STATE_REL(rel2, y);

//colors
#define FUNC_COLOR(Code)						\
  EAPI void								\
  edje_edit_state_##Code##_get(Evas_Object *obj, const char *part, const char *state, double value, int *r, int *g, int *b, int *a) \
  {									\
     GET_PD_OR_RETURN();						\
									\
     if (r) *r = pd->Code.r;						\
     if (g) *g = pd->Code.g;						\
     if (b) *b = pd->Code.b;						\
     if (a) *a = pd->Code.a;						\
  }									\
  EAPI void								\
  edje_edit_state_##Code##_set(Evas_Object *obj, const char *part, const char *state, double value, int r, int g, int b, int a) \
  {									\
     GET_PD_OR_RETURN();						\
     									\
     if (r > -1 && r < 256) pd->Code.r = r;				\
     if (g > -1 && g < 256) pd->Code.g = g;				\
     if (b > -1 && b < 256) pd->Code.b = b;				\
     if (a > -1 && a < 256) pd->Code.a = a;				\
     									\
     edje_object_calc_force(obj);					\
  }

FUNC_COLOR(color);
FUNC_COLOR(color2);

EAPI void
edje_edit_state_color3_get(Evas_Object *obj, const char *part, const char *state, double value, int *r, int *g, int *b, int *a)
{
   Edje_Part_Description_Text *txt;

   GET_PD_OR_RETURN();

   if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
       (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
     {
        if (r) *r = 0;
        if (g) *g = 0;
        if (b) *b = 0;
        if (a) *a = 0;
        return;
     }

   txt = (Edje_Part_Description_Text*) pd;

   if (r) *r = txt->text.color3.r;
   if (g) *g = txt->text.color3.g;
   if (b) *b = txt->text.color3.b;
   if (a) *a = txt->text.color3.a;
}

EAPI void
edje_edit_state_color3_set(Evas_Object *obj, const char *part, const char *state, double value, int r, int g, int b, int a)
{
   Edje_Part_Description_Text *txt;

   GET_PD_OR_RETURN();

   if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
       (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
     return;

   txt = (Edje_Part_Description_Text*) pd;

   if (r > -1 && r < 256) txt->text.color3.r = r;
   if (g > -1 && g < 256) txt->text.color3.g = g;
   if (b > -1 && b < 256) txt->text.color3.b = b;
   if (a > -1 && a < 256) txt->text.color3.a = a;

   edje_object_calc_force(obj);
}

#define FUNC_STATE_DOUBLE(Class, Value)					\
  EAPI double								\
  edje_edit_state_##Class##_##Value##_get(Evas_Object *obj, const char *part, const char *state, double value) \
  {									\
     GET_PD_OR_RETURN(0);						\
     return TO_DOUBLE(pd->Class.Value);				\
  }									\
  EAPI void								\
  edje_edit_state_##Class##_##Value##_set(Evas_Object *obj, const char *part, const char *state, double value, double v) \
  {									\
     GET_PD_OR_RETURN();						\
     pd->Class.Value = FROM_DOUBLE(v);				\
     edje_object_calc_force(obj);					\
  }

#define FUNC_STATE_INT(Class, Value)					\
  EAPI int								\
  edje_edit_state_##Class##_##Value##_get(Evas_Object *obj, const char *part, const char *state, double value) \
  {									\
     GET_PD_OR_RETURN(0);						\
     return pd->Class.Value;					\
  }									\
  EAPI void								\
  edje_edit_state_##Class##_##Value##_set(Evas_Object *obj, const char *part, const char *state, double value, int v) \
  {									\
     GET_PD_OR_RETURN();						\
     pd->Class.Value = v;					\
     edje_object_calc_force(obj);					\
  }

FUNC_STATE_DOUBLE(align, x);
FUNC_STATE_DOUBLE(align, y);
FUNC_STATE_INT(min, w);
FUNC_STATE_INT(min, h);
FUNC_STATE_INT(max, w);
FUNC_STATE_INT(max, h);
FUNC_STATE_DOUBLE(aspect, min);
FUNC_STATE_DOUBLE(aspect, max);

#define FUNC_STATE_DOUBLE_FILL(Class, Type, Value)			\
  EAPI double								\
  edje_edit_state_fill_##Type##_relative_##Value##_get(Evas_Object *obj, const char *part, const char *state, double value) \
  {									\
     Edje_Part_Description_Image *img;					\
									\
     GET_PD_OR_RETURN(0);						\
                                                                        \
     if (rp->part->type != EDJE_PART_TYPE_IMAGE)                        \
       return 0;                                                        \
									\
     img = (Edje_Part_Description_Image*) pd;				\
									\
     return TO_DOUBLE(img->image.fill.Class##rel_##Value);		\
  }									\
  EAPI void								\
  edje_edit_state_fill_##Type##_relative_##Value##_set(Evas_Object *obj, const char *part, const char *state, double value, double v) \
  {									\
     Edje_Part_Description_Image *img;					\
									\
     GET_PD_OR_RETURN();						\
                                                                        \
     if (rp->part->type != EDJE_PART_TYPE_IMAGE)                        \
       return;                                                          \
									\
     img = (Edje_Part_Description_Image*) pd;				\
									\
     img->image.fill.Class##rel_##Value = FROM_DOUBLE(v);		\
     edje_object_calc_force(obj);					\
  }

#define FUNC_STATE_INT_FILL(Class, Type, Value)				\
  EAPI int								\
  edje_edit_state_fill_##Type##_offset_##Value##_get(Evas_Object *obj, const char *part, const char *state, double value) \
  {									\
     Edje_Part_Description_Image *img;					\
									\
     GET_PD_OR_RETURN(0);						\
                                                                        \
     if (rp->part->type != EDJE_PART_TYPE_IMAGE)                        \
       return 0;                                                        \
									\
     img = (Edje_Part_Description_Image*) pd;				\
									\
     return img->image.fill.Class##abs_##Value;				\
  }									\
  EAPI void								\
  edje_edit_state_fill_##Type##_offset_##Value##_set(Evas_Object *obj, const char *part, const char *state, double value, double v) \
  {									\
     Edje_Part_Description_Image *img;					\
									\
     GET_PD_OR_RETURN();						\
                                                                        \
     if (rp->part->type != EDJE_PART_TYPE_IMAGE)                        \
       return;                                                          \
									\
     img = (Edje_Part_Description_Image*) pd;				\
									\
     img->image.fill.Class##abs_##Value = FROM_DOUBLE(v);		\
     edje_object_calc_force(obj);					\
  }

FUNC_STATE_DOUBLE_FILL(pos_, origin, x);
FUNC_STATE_DOUBLE_FILL(pos_, origin, y);
FUNC_STATE_INT_FILL(pos_, origin, x);
FUNC_STATE_INT_FILL(pos_, origin, y);

FUNC_STATE_DOUBLE_FILL(, size, x);
FUNC_STATE_DOUBLE_FILL(, size, y);
FUNC_STATE_INT_FILL(, size, x);
FUNC_STATE_INT_FILL(, size, y);

EAPI Eina_Bool
edje_edit_state_visible_get(Evas_Object *obj, const char *part, const char *state, double value)
{
   GET_PD_OR_RETURN(EINA_FALSE);
   //printf("Get state visible flag of part: %s state: %s\n", part, state);
   return pd->visible;
}

EAPI void
edje_edit_state_visible_set(Evas_Object *obj, const char *part, const char *state, double value, Eina_Bool visible)
{
   GET_PD_OR_RETURN();
   //printf("Set state visible flag of part: %s state: %s to: %d\n", part, state, visible);
   if (visible) pd->visible = 1;
   else         pd->visible = 0;
   edje_object_calc_force(obj);
}

EAPI unsigned char
edje_edit_state_aspect_pref_get(Evas_Object *obj, const char *part, const char *state, double value)
{
   GET_PD_OR_RETURN(0);

   //printf("GET ASPECT_PREF of state '%s' [%d]\n", state, pd->aspect.prefer);
   return pd->aspect.prefer;
}

EAPI void
edje_edit_state_aspect_pref_set(Evas_Object *obj, const char *part, const char *state, double value, unsigned char pref)
{
   GET_PD_OR_RETURN();

   //printf("SET ASPECT_PREF of state '%s' [to: %d]\n", state, pref);
   pd->aspect.prefer = pref;
}

EAPI const char*
edje_edit_state_color_class_get(Evas_Object *obj, const char *part, const char *state, double value)
{
   GET_PD_OR_RETURN(NULL);
   //printf("Get ColorClass of part: %s state: %s\n", part, state);
   return eina_stringshare_add(pd->color_class);
}

EAPI void
edje_edit_state_color_class_set(Evas_Object *obj, const char *part, const char *state, double value, const char *color_class)
{
   GET_PD_OR_RETURN();
   //printf("Set ColorClass of part: %s state: %s [to: %s]\n", part, state, color_class);
   _edje_if_string_free(ed, pd->color_class);
   pd->color_class = (char*)eina_stringshare_add(color_class);
}

EAPI const Eina_List *
edje_edit_state_external_params_list_get(Evas_Object *obj, const char *part, const char *state, double value)
{
   Edje_Part_Description_External *external;

   GET_PD_OR_RETURN(NULL);

   if (rp->part->type != EDJE_PART_TYPE_EXTERNAL)
     return NULL;

   external = (Edje_Part_Description_External *) pd;

   return external->external_params;
}

EAPI Eina_Bool
edje_edit_state_external_param_get(Evas_Object *obj, const char *part, const char *state, double value, const char *param, Edje_External_Param_Type *type, void **val)
{
   Edje_Part_Description_External *external;
   Edje_External_Param *p;
   Eina_List *l;

   GET_PD_OR_RETURN(EINA_FALSE);

   if (rp->part->type != EDJE_PART_TYPE_EXTERNAL)
     return EINA_FALSE;

   external = (Edje_Part_Description_External *) pd;

   EINA_LIST_FOREACH(external->external_params, l, p)
      if (!strcmp(p->name, param))
	{
	   if (type) *type = p->type;
	   if (val)
	      switch (p->type)
		{
		 case EDJE_EXTERNAL_PARAM_TYPE_INT:
		 case EDJE_EXTERNAL_PARAM_TYPE_BOOL:
		    *val = &p->i;
		    break;
		 case EDJE_EXTERNAL_PARAM_TYPE_DOUBLE:
		    *val = &p->d;
		    break;
		 case EDJE_EXTERNAL_PARAM_TYPE_CHOICE:
		 case EDJE_EXTERNAL_PARAM_TYPE_STRING:
		    *val = (void *)p->s;
		    break;
		 default:
		    ERR("unknown external parameter type '%d'", p->type);
		}
	   return EINA_TRUE;
	}

   return EINA_FALSE;
}

EAPI Eina_Bool
edje_edit_state_external_param_int_get(Evas_Object *obj, const char *part, const char *state, double value, const char *param, int *val)
{
   Edje_Part_Description_External *external;
   Edje_External_Param *p;
   Eina_List *l;

   GET_PD_OR_RETURN(EINA_FALSE);

   if (rp->part->type != EDJE_PART_TYPE_EXTERNAL)
     {
        if (val) *val = 0;
        return EINA_FALSE;
     }

   external = (Edje_Part_Description_External *) pd;

   EINA_LIST_FOREACH(external->external_params, l, p)
      if (!strcmp(p->name, param))
	{
	   if (p->type != EDJE_EXTERNAL_PARAM_TYPE_INT)
	     return EINA_FALSE;
	   if (val)
	     *val = p->i;
	   return EINA_TRUE;
	}

   return EINA_FALSE;
}

EAPI Eina_Bool
edje_edit_state_external_param_bool_get(Evas_Object *obj, const char *part, const char *state, double value, const char *param, Eina_Bool *val)
{
   Edje_Part_Description_External *external;
   Edje_External_Param *p;
   Eina_List *l;

   GET_PD_OR_RETURN(EINA_FALSE);

   if (rp->part->type != EDJE_PART_TYPE_EXTERNAL)
     {
        if (val) *val = 0;
        return EINA_FALSE;
     }

   external = (Edje_Part_Description_External *) pd;

   EINA_LIST_FOREACH(external->external_params, l, p)
      if (!strcmp(p->name, param))
	{
	   if (p->type != EDJE_EXTERNAL_PARAM_TYPE_BOOL)
	     return EINA_FALSE;
	   if (val)
	     *val = p->i;
	   return EINA_TRUE;
	}

   return EINA_FALSE;
}

EAPI Eina_Bool
edje_edit_state_external_param_double_get(Evas_Object *obj, const char *part, const char *state, double value, const char *param, double *val)
{
   Edje_Part_Description_External *external;
   Edje_External_Param *p;
   Eina_List *l;

   GET_PD_OR_RETURN(EINA_FALSE);

   if (rp->part->type != EDJE_PART_TYPE_EXTERNAL)
     {
        if (val) *val = 0;
        return EINA_FALSE;
     }

   external = (Edje_Part_Description_External *) pd;

   EINA_LIST_FOREACH(external->external_params, l, p)
      if (!strcmp(p->name, param))
	{
	   if (p->type != EDJE_EXTERNAL_PARAM_TYPE_DOUBLE)
	     return EINA_FALSE;
	   if (val)
	     *val = p->d;
	   return EINA_TRUE;
	}

   return EINA_FALSE;
}

EAPI Eina_Bool
edje_edit_state_external_param_string_get(Evas_Object *obj, const char *part, const char *state, double value, const char *param, const char **val)
{
   Edje_Part_Description_External *external;
   Edje_External_Param *p;
   Eina_List *l;

   GET_PD_OR_RETURN(EINA_FALSE);

   if (rp->part->type != EDJE_PART_TYPE_EXTERNAL)
     {
        if (val) *val = NULL;
        return EINA_FALSE;
     }

   external = (Edje_Part_Description_External *) pd;

   EINA_LIST_FOREACH(external->external_params, l, p)
      if (!strcmp(p->name, param))
	{
	   if (p->type != EDJE_EXTERNAL_PARAM_TYPE_STRING)
	     return EINA_FALSE;
	   if (val)
	     *val = p->s;
	   return EINA_TRUE;
	}

   return EINA_FALSE;
}

EAPI Eina_Bool
edje_edit_state_external_param_choice_get(Evas_Object *obj, const char *part, const char *state, double value, const char *param, const char **val)
{
   Edje_Part_Description_External *external;
   Edje_External_Param *p;
   Eina_List *l;

   GET_PD_OR_RETURN(EINA_FALSE);

   if (rp->part->type != EDJE_PART_TYPE_EXTERNAL)
     {
        if (val) *val = NULL;
        return EINA_FALSE;
     }

   external = (Edje_Part_Description_External *) pd;

   EINA_LIST_FOREACH(external->external_params, l, p)
      if (!strcmp(p->name, param))
	{
	   if (p->type != EDJE_EXTERNAL_PARAM_TYPE_CHOICE)
	     return EINA_FALSE;
	   if (val)
	     *val = p->s;
	   return EINA_TRUE;
	}

   return EINA_FALSE;
}

/**
 * Arguments should have proper sized values matching their types:
 *   - EDJE_EXTERNAL_PARAM_TYPE_INT: int
 *   - EDJE_EXTERNAL_PARAM_TYPE_BOOL: int
 *   - EDJE_EXTERNAL_PARAM_TYPE_DOUBLE: double
 *   - EDJE_EXTERNAL_PARAM_TYPE_STRING: char*
 *   - EDJE_EXTERNAL_PARAM_TYPE_CHOICE: char*
 *
 * @note: The validation of the parameter will occur only if the part
 * is in the same state as the one being modified.
 */
EAPI Eina_Bool
edje_edit_state_external_param_set(Evas_Object *obj, const char *part, const char *state, double value, const char *param, Edje_External_Param_Type type, ...)
{
   va_list ap;
   Eina_List *l;
   Edje_Part_Description_External *external;
   Edje_External_Param *p = NULL, old_p = { 0, 0, 0, 0, 0 };
   int found = 0;

   GET_PD_OR_RETURN(EINA_FALSE);

   if (rp->part->type != EDJE_PART_TYPE_EXTERNAL)
     return EINA_FALSE;

   external = (Edje_Part_Description_External *) pd;

   va_start(ap, type);

   EINA_LIST_FOREACH(external->external_params, l, p)
      if (!strcmp(p->name, param))
	{
	   found = 1;
	   old_p = *p;
	   break;
	}

   if (!found)
     {
	p = _alloc(sizeof(Edje_External_Param));
	if (!p)
	  {
	     va_end(ap);
	     return EINA_FALSE;
	  }
	p->name = eina_stringshare_add(param);
     }

   p->type = type;
   p->i = 0;
   p->d = 0;
   _edje_if_string_free(ed, p->s);
   p->s = NULL;

   switch (type)
     {
      case EDJE_EXTERNAL_PARAM_TYPE_INT:
      case EDJE_EXTERNAL_PARAM_TYPE_BOOL:
	 p->i = (int)va_arg(ap, int);
	 break;
      case EDJE_EXTERNAL_PARAM_TYPE_DOUBLE:
	 p->d = (double)va_arg(ap, double);
	 break;
      case EDJE_EXTERNAL_PARAM_TYPE_CHOICE:
      case EDJE_EXTERNAL_PARAM_TYPE_STRING:
	 p->s = eina_stringshare_add((const char *)va_arg(ap, char *));
	 break;
      default:
	 ERR("unknown external parameter type '%d'", type);
	 va_end(ap);
	 if (!found) free(p);
	 else *p = old_p;
	 return EINA_FALSE;
     }

   va_end(ap);

   //FIXME:
   //For now, we're just setting the value if the state is the selected state.
   //This is a conceptual error and is incoherent with the rest of the API!
     {
	const char *sname;
	double svalue;
	sname = edje_edit_part_selected_state_get(obj, part, &svalue);
	if (!strcmp(state, sname) && svalue == value)
	  if (!edje_object_part_external_param_set(obj, part, p))
	    if ((type == EDJE_EXTERNAL_PARAM_TYPE_CHOICE) ||
		  (type == EDJE_EXTERNAL_PARAM_TYPE_STRING))
	      {
		 _edje_if_string_free(ed, p->s);
		 if (!found) free(p);
		 else *p = old_p;
		 eina_stringshare_del(sname);
		 return EINA_FALSE;
	      }
	eina_stringshare_del(sname);
     }

   if (!found)
     external->external_params = eina_list_append(external->external_params, p);

   _edje_external_parsed_params_free(rp->swallowed_object,
				     rp->param1.external_params);
   rp->param1.external_params = \
			     _edje_external_params_parse(rp->swallowed_object,
							 external->external_params);


   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_state_external_param_int_set(Evas_Object *obj, const char *part, const char *state, double value, const char *param, int val)
{
   return edje_edit_state_external_param_set(obj, part, state, value, param, EDJE_EXTERNAL_PARAM_TYPE_INT, val);
}

EAPI Eina_Bool
edje_edit_state_external_param_bool_set(Evas_Object *obj, const char *part, const char *state, double value, const char *param, Eina_Bool val)
{
   return edje_edit_state_external_param_set(obj, part, state, value, param, EDJE_EXTERNAL_PARAM_TYPE_BOOL, (int)val);
}

EAPI Eina_Bool
edje_edit_state_external_param_double_set(Evas_Object *obj, const char *part, const char *state, double value, const char *param, double val)
{
   return edje_edit_state_external_param_set(obj, part, state, value, param, EDJE_EXTERNAL_PARAM_TYPE_DOUBLE, val);
}

EAPI Eina_Bool
edje_edit_state_external_param_string_set(Evas_Object *obj, const char *part, const char *state, double value, const char *param, const char *val)
{
   return edje_edit_state_external_param_set(obj, part, state, value, param, EDJE_EXTERNAL_PARAM_TYPE_STRING, val);
}

EAPI Eina_Bool
edje_edit_state_external_param_choice_set(Evas_Object *obj, const char *part, const char *state, double value, const char *param, const char *val)
{
   return edje_edit_state_external_param_set(obj, part, state, value, param, EDJE_EXTERNAL_PARAM_TYPE_CHOICE, val);
}

/**************/
/*  TEXT API */
/**************/

EAPI const char *
edje_edit_state_text_get(Evas_Object *obj, const char *part, const char *state, double value)
{
   Edje_Part_Description_Text *txt;

   GET_PD_OR_RETURN(NULL);

   if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
       (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
     return NULL;

   txt = (Edje_Part_Description_Text *) pd;
   //printf("GET TEXT of state: %s\n", state);

   if (txt->text.text)
     return eina_stringshare_add(txt->text.text);

   return NULL;
}

EAPI void
edje_edit_state_text_set(Evas_Object *obj, const char *part, const char *state, double value, const char *text)
{
   Edje_Part_Description_Text *txt;

   GET_PD_OR_RETURN();

   //printf("SET TEXT of state: %s\n", state);

   if (!text) return;

   if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
       (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
     return;

   txt = (Edje_Part_Description_Text *) pd;

   _edje_if_string_free(ed, txt->text.text);
   txt->text.text = (char *)eina_stringshare_add(text);

   edje_object_calc_force(obj);
}

EAPI int
edje_edit_state_text_size_get(Evas_Object *obj, const char *part, const char *state, double value)
{
   Edje_Part_Description_Text *txt;

   GET_PD_OR_RETURN(-1);

   if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
       (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
     return -1;

   txt = (Edje_Part_Description_Text *) pd;
   //printf("GET TEXT_SIZE of state: %s [%d]\n", state, pd->text.size);
   return txt->text.size;
}

EAPI void
edje_edit_state_text_size_set(Evas_Object *obj, const char *part, const char *state, double value, int size)
{
   Edje_Part_Description_Text *txt;

   GET_PD_OR_RETURN();

   //printf("SET TEXT_SIZE of state: %s [%d]\n", state, size);

   if (size < 0) return;

   if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
       (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
     return;

   txt = (Edje_Part_Description_Text *) pd;

   txt->text.size = size;

   edje_object_calc_force(obj);
}

#define FUNC_TEXT_DOUBLE(Name, Value)					\
  EAPI double								\
  edje_edit_state_text_##Name##_get(Evas_Object *obj, const char *part, const char *state, double value) \
  {									\
     Edje_Part_Description_Text *txt;					\
									\
     GET_PD_OR_RETURN(0);						\
									\
     if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&                     \
         (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))                  \
       return 0;                                                        \
                                                                        \
     txt = (Edje_Part_Description_Text *) pd;				\
     return TO_DOUBLE(txt->text.Value);					\
  }									\
  EAPI void								\
  edje_edit_state_text_##Name##_set(Evas_Object *obj, const char *part, const char *state, double value, double v) \
  {									\
     Edje_Part_Description_Text *txt;					\
									\
     GET_PD_OR_RETURN();						\
									\
     if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&                     \
         (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))                  \
       return;                                                          \
                                                                        \
     txt = (Edje_Part_Description_Text *) pd;				\
     txt->text.Value = FROM_DOUBLE(v);					\
     edje_object_calc_force(obj);					\
  }									\

FUNC_TEXT_DOUBLE(align_x, align.x);
FUNC_TEXT_DOUBLE(align_y, align.y);
FUNC_TEXT_DOUBLE(elipsis, elipsis);

#define FUNC_TEXT_BOOL_FIT(Value)					\
  EAPI Eina_Bool							\
  edje_edit_state_text_fit_##Value##_get(Evas_Object *obj, const char *part, const char *state, double value) \
  {									\
     Edje_Part_Description_Text *txt;					\
									\
     GET_PD_OR_RETURN(EINA_FALSE);					\
                                                                        \
     if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&                     \
         (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))                  \
       return EINA_FALSE;                                               \
									\
     txt = (Edje_Part_Description_Text *) pd;				\
     return txt->text.fit_##Value;					\
  }									\
  EAPI void								\
  edje_edit_state_text_fit_##Value##_set(Evas_Object *obj, const char *part, const char *state, double value, Eina_Bool fit) \
  {									\
     Edje_Part_Description_Text *txt;					\
									\
     GET_PD_OR_RETURN();						\
                                                                        \
     if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&                     \
         (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))                  \
       return;                                                          \
									\
     txt = (Edje_Part_Description_Text *) pd;				\
     txt->text.fit_##Value = fit ? 1 : 0;				\
     edje_object_calc_force(obj);					\
  }

FUNC_TEXT_BOOL_FIT(x);
FUNC_TEXT_BOOL_FIT(y);

EAPI Eina_List *
edje_edit_fonts_list_get(Evas_Object *obj)
{
   Eina_Iterator *it;
   Eina_List *fonts = NULL;
   Edje_Font_Directory_Entry *f;

   GET_ED_OR_RETURN(NULL);

   if (!ed->file || !ed->file->fonts) return NULL;

   it = eina_hash_iterator_data_new(ed->file->fonts);
   if (!it) return NULL;

   EINA_ITERATOR_FOREACH(it, f)
     fonts = eina_list_append(fonts, f);

   eina_iterator_free(it);

   return fonts;
}

EAPI Eina_Bool
edje_edit_font_add(Evas_Object *obj, const char* path, const char* alias)
{
   char entry[PATH_MAX];
   struct stat st;
   Edje_Font_Directory_Entry *fnt;

   GET_ED_OR_RETURN(EINA_FALSE);

   INF("ADD FONT: %s\n", path);

   if (!path) return EINA_FALSE;
   if (stat(path, &st) || !S_ISREG(st.st_mode)) return EINA_FALSE;
   if (!ed->file) return EINA_FALSE;
   if (!ed->path) return EINA_FALSE;

   /* Alias */
   if (!alias)
     {
	if ((alias = strrchr(path, '/'))) alias ++;
	else alias = (char *)path;
     }
   snprintf(entry, sizeof(entry), "edje/fonts/%s", alias);

   /* Check if exists */
   fnt = eina_hash_find(ed->file->fonts, alias);
   if (fnt)
     return EINA_FALSE;

   /* Create Edje_Font_Directory_Entry */
   fnt = _alloc(sizeof(Edje_Font_Directory_Entry));
   if (!fnt)
     {
	ERR("Unable to alloc font entry part \"%s\"", alias);
	return EINA_FALSE;
     }
   fnt->entry = eina_stringshare_add(alias);

   eina_hash_direct_add(ed->file->fonts, fnt->entry, fnt);

   /* Import font */
   if (!_edje_import_font_file(ed, path, entry))
     {
	eina_hash_del(ed->file->fonts, fnt->entry, fnt);
	return EINA_FALSE;
     }

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_font_del(Evas_Object *obj, const char* alias)
{
   Edje_Font_Directory_Entry *fnt;

   GET_ED_OR_RETURN(EINA_FALSE);

   INF("DEL FONT: %s\n", alias);

   if (!alias) return EINA_FALSE;
   if (!ed->file) return EINA_FALSE;
   if (!ed->path) return EINA_FALSE;

   fnt = eina_hash_find(ed->file->fonts, alias);
   if (!fnt)
     {
	WRN("Unable to find font entry part \"%s\"", alias);
	return EINA_TRUE;
     }

   /* Erase font to edje file */
   {
      char entry[PATH_MAX];
      Eet_File *eetf;

      /* open the eet file */
      eetf = eet_open(ed->path, EET_FILE_MODE_READ_WRITE);
      if (!eetf)
	{
	   ERR("Unable to open \"%s\" for writing output", ed->path);
	   return EINA_FALSE;
	}

      snprintf(entry, sizeof(entry), "fonts/%s", alias);

      if (eet_delete(eetf, entry) <= 0)
        {
           ERR("Unable to delete \"%s\" font entry", entry);
           eet_close(eetf);
           return EINA_FALSE;
        }

      /* write the edje_file */
      if (!_edje_edit_edje_file_save(eetf, ed->file))
	{
	   eet_close(eetf);
	   return EINA_FALSE;
	}
      eet_close(eetf);
   }

   eina_hash_del(ed->file->fonts, alias, fnt);

   return EINA_TRUE;
}

EAPI const char *
edje_edit_state_font_get(Evas_Object *obj, const char *part, const char *state, double value)
{
   Edje_Part_Description_Text *txt;

   GET_PD_OR_RETURN(NULL);

   if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
       (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
     return NULL;

   txt = (Edje_Part_Description_Text*) pd;

   if (!txt->text.font) return NULL;
   return eina_stringshare_add(txt->text.font);
}

EAPI void
edje_edit_state_font_set(Evas_Object *obj, const char *part, const char *state, double value, const char *font)
{
   Edje_Part_Description_Text *txt;

   GET_PD_OR_RETURN();

   if ((rp->part->type != EDJE_PART_TYPE_TEXT) &&
       (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK))
     return;

   txt = (Edje_Part_Description_Text*) pd;

   _edje_if_string_free(ed, txt->text.font);
   txt->text.font = (char *)eina_stringshare_add(font);

   edje_object_calc_force(obj);
}

EAPI Edje_Text_Effect
edje_edit_part_effect_get(Evas_Object *obj, const char *part)
{
   GET_RP_OR_RETURN(0);

   //printf("GET EFFECT of part: %s\n", part);
   return rp->part->effect;
}

EAPI void
edje_edit_part_effect_set(Evas_Object *obj, const char *part, Edje_Text_Effect effect)
{
   GET_RP_OR_RETURN();

   //printf("SET EFFECT of part: %s [%d]\n", part, effect);
   rp->part->effect = effect;

   edje_object_calc_force(obj);
}

/****************/
/*  IMAGES API  */
/****************/

EAPI Eina_List *
edje_edit_images_list_get(Evas_Object *obj)
{
   Eina_List *images = NULL;
   unsigned int i;

   GET_ED_OR_RETURN(NULL);

   if (!ed->file) return NULL;
   if (!ed->file->image_dir) return NULL;

   //printf("GET IMAGES LIST for %s\n", ed->file->path);
   for (i = 0; i < ed->file->image_dir->entries_count; ++i)
     images = eina_list_append(images,
			       eina_stringshare_add(ed->file->image_dir->entries[i].entry));

   return images;
}

EAPI Eina_Bool
edje_edit_image_add(Evas_Object *obj, const char* path)
{
   Edje_Image_Directory_Entry *de;
   unsigned int i;
   int free_id = -1;
   char *name;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!path) return EINA_FALSE;
   if (!ed->file) return EINA_FALSE;
   if (!ed->path) return EINA_FALSE;

   /* Create Image_Directory if not exist */
   if (!ed->file->image_dir)
     {
	ed->file->image_dir = _alloc(sizeof(Edje_Image_Directory));
	if (!ed->file->image_dir) return EINA_FALSE;
     }

   /* Image name */
   if ((name = strrchr(path, '/'))) name++;
   else name = (char *)path;

   /* Loop trough image directory to find if image exist */
   for (i = 0; i < ed->file->image_dir->entries_count; ++i)
     {
	de = ed->file->image_dir->entries + i;

	if (!de->entry)
	  free_id = i;
	else if (!strcmp(name, de->entry))
	  return EINA_FALSE;
     }

   if (free_id == -1)
     {
	Edje_Image_Directory_Entry *tmp;
	unsigned int count;

	count = ed->file->image_dir->entries_count + 1;

	tmp = realloc(ed->file->image_dir->entries,
		      sizeof (Edje_Image_Directory_Entry) * count);
	if (!tmp) return EINA_FALSE;

	ed->file->image_dir->entries = tmp;
	free_id = ed->file->image_dir->entries_count;
	ed->file->image_dir->entries_count = count;
     }

   /* Set Image Entry */
   de = ed->file->image_dir->entries + free_id;
   de->entry = eina_stringshare_add(name);
   de->id = free_id;
   de->source_type = 1;
   de->source_param = 1;

   /* Import image */
   if (!_edje_import_image_file(ed, path, free_id))
     {
	eina_stringshare_del(de->entry);
	de->entry = NULL;
	return EINA_FALSE;
     }

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_image_del(Evas_Object *obj, const char* name)
{
   Edje_Image_Directory_Entry *de;
   unsigned int i;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!name) return EINA_FALSE;
   if (!ed->file) return EINA_FALSE;
   if (!ed->path) return EINA_FALSE;

   /* Create Image_Directory if not exist */
   if (!ed->file->image_dir)
     return EINA_TRUE;

   for (i = 0; i < ed->file->image_dir->entries_count; ++i)
     {
	de = ed->file->image_dir->entries + i;

	if (de->entry
	    && !strcmp(name, de->entry))
	  break;
     }

   if (i == ed->file->image_dir->entries_count)
     {
	WRN("Unable to find image entry part \"%s\"", name);
	return EINA_TRUE;
     }

   {
      char entry[PATH_MAX];
      Eet_File *eetf;

      /* open the eet file */
      eetf = eet_open(ed->path, EET_FILE_MODE_READ_WRITE);
      if (!eetf)
	{
	   ERR("Unable to open \"%s\" for writing output", ed->path);
	   return EINA_FALSE;
	}

      snprintf(entry, sizeof(entry), "images/%i", de->id);

      if (eet_delete(eetf, entry) <= 0)
        {
           ERR("Unable to delete \"%s\" font entry", entry);
           eet_close(eetf);
           return EINA_FALSE;
        }

      /* write the edje_file */
      if (!_edje_edit_edje_file_save(eetf, ed->file))
	{
	   eet_close(eetf);
	   return EINA_FALSE;
	}

      eet_close(eetf);
   }

   _edje_if_string_free(ed, de->entry);
   de->entry = NULL;

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_image_data_add(Evas_Object *obj, const char *name, int id)
{
   Edje_Image_Directory_Entry *de;

   GET_ED_OR_RETURN(EINA_FALSE);

   if (!name) return EINA_FALSE;
   if (!ed->file) return EINA_FALSE;
   if (!ed->path) return EINA_FALSE;

   /* Create Image_Directory if not exist */
   if (!ed->file->image_dir)
     {
	ed->file->image_dir = _alloc(sizeof(Edje_Image_Directory));
	if (!ed->file->image_dir) return EINA_FALSE;
     }

   /* Loop trough image directory to find if image exist */
   if (id < 0) id = - id - 1;
   if ((unsigned int) id >= ed->file->image_dir->entries_count) return EINA_FALSE;

   de = ed->file->image_dir->entries + id;
   eina_stringshare_replace(&de->entry, name);
   de->source_type = 1;
   de->source_param = 1;

   return EINA_TRUE;
}

EAPI int
edje_edit_image_id_get(Evas_Object *obj, const char *image_name)
{
   return _edje_image_id_find(obj, image_name);
}

EAPI Edje_Edit_Image_Comp
edje_edit_image_compression_type_get(Evas_Object *obj, const char *image)
{
   Edje_Image_Directory_Entry *de = NULL;
   unsigned int i;

   GET_ED_OR_RETURN(-1);

   if (!ed->file) return -1;
   if (!ed->file->image_dir) return -1;

   for (i = 0; i < ed->file->image_dir->entries_count; ++i)
     {
	de = ed->file->image_dir->entries + i;

	if (de->entry
	    && !strcmp(image, de->entry))
	  break;
     }

   if (i == ed->file->image_dir->entries_count) return -1;

   switch(de->source_type)
     {
	case EDJE_IMAGE_SOURCE_TYPE_INLINE_PERFECT:
		if (de->source_param == 0) // RAW
		  return EDJE_EDIT_IMAGE_COMP_RAW;
		else // COMP
		  return EDJE_EDIT_IMAGE_COMP_COMP;
		break;
	case EDJE_IMAGE_SOURCE_TYPE_INLINE_LOSSY: // LOSSY
		return EDJE_EDIT_IMAGE_COMP_LOSSY;
		break;
	case EDJE_IMAGE_SOURCE_TYPE_EXTERNAL: // USER
		return EDJE_EDIT_IMAGE_COMP_USER;
		break;
     }

   return -1;
}

EAPI int
edje_edit_image_compression_rate_get(Evas_Object *obj, const char *image)
{
   Edje_Image_Directory_Entry *de;
   unsigned int i;

   GET_ED_OR_RETURN(-1);

   // Gets the Image Entry
   for (i = 0; i < ed->file->image_dir->entries_count; ++i)
     {
	de = ed->file->image_dir->entries + i;
	if (de->entry
	    && !strcmp(de->entry, image))
	  break;
     }

   if (i == ed->file->image_dir->entries_count) return -1;
   if (de->source_type != EDJE_IMAGE_SOURCE_TYPE_INLINE_LOSSY) return -2;

   return de->source_param;
}

EAPI const char *
edje_edit_state_image_get(Evas_Object *obj, const char *part, const char *state, double value)
{
   Edje_Part_Description_Image *img;
   char *image;

   GET_PD_OR_RETURN(NULL);

   if (rp->part->type != EDJE_PART_TYPE_IMAGE)
     return NULL;

   img = (Edje_Part_Description_Image *) pd;

   image = (char *)_edje_image_name_find(obj, img->image.id);
   if (!image) return NULL;

   //printf("GET IMAGE for %s [%s]\n", state, image);
   return eina_stringshare_add(image);
}

EAPI void
edje_edit_state_image_set(Evas_Object *obj, const char *part, const char *state, double value, const char *image)
{
   Edje_Part_Description_Image *img;
   int id;

   GET_PD_OR_RETURN();

   if (!image) return;

   if (rp->part->type != EDJE_PART_TYPE_IMAGE)
     return;

   id = _edje_image_id_find(obj, image);
   //printf("SET IMAGE for %s [%s]\n", state, image);

   img = (Edje_Part_Description_Image *) pd;

   if (id > -1) img->image.id = id;

   edje_object_calc_force(obj);
}

EAPI Eina_List *
edje_edit_state_tweens_list_get(Evas_Object *obj, const char *part, const char *state, double value)
{
   Edje_Part_Description_Image *img;
   Eina_List *tweens = NULL;
   const char *name;
   unsigned int i;

   GET_PD_OR_RETURN(NULL);

   if (rp->part->type != EDJE_PART_TYPE_IMAGE)
     return NULL;

   img = (Edje_Part_Description_Image *) pd;

   for (i = 0; i < img->image.tweens_count; ++i)
     {
	name = _edje_image_name_find(obj, img->image.tweens[i]->id);
	//printf("   t: %s\n", name);
	tweens = eina_list_append(tweens, eina_stringshare_add(name));
     }

   return tweens;
}

EAPI Eina_Bool
edje_edit_state_tween_add(Evas_Object *obj, const char *part, const char *state, double value, const char *tween)
{
   Edje_Part_Description_Image *img;
   Edje_Part_Image_Id **tmp;
   Edje_Part_Image_Id *i;
   int id;

   GET_PD_OR_RETURN(EINA_FALSE);

   if (rp->part->type != EDJE_PART_TYPE_IMAGE)
     return EINA_FALSE;

   id = _edje_image_id_find(obj, tween);
   if (id < EINA_FALSE) return 0;

   /* alloc Edje_Part_Image_Id */
   i = _alloc(sizeof(Edje_Part_Image_Id));
   if (!i) return EINA_FALSE;
   i->id = id;

   img = (Edje_Part_Description_Image *) pd;

   /* add to tween list */
   tmp = realloc(img->image.tweens,
		 sizeof (Edje_Part_Image_Id*) * img->image.tweens_count);
   if (!tmp)
     {
	free(i);
	return EINA_FALSE;
     }

   tmp[img->image.tweens_count++] = i;
   img->image.tweens = tmp;

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_state_tween_del(Evas_Object *obj, const char *part, const char *state, double value, const char *tween)
{
   Edje_Part_Description_Image *img;
   unsigned int i;
   int search;

   GET_PD_OR_RETURN(EINA_FALSE);

   if (rp->part->type != EDJE_PART_TYPE_IMAGE)
     return EINA_FALSE;

   img = (Edje_Part_Description_Image *) pd;

   if (!img->image.tweens_count) return EINA_FALSE;

   search = _edje_image_id_find(obj, tween);
   if (search < 0) return EINA_FALSE;

   for (i = 0; i < img->image.tweens_count; ++i)
     {
	if (img->image.tweens[i]->id == search)
	  {
	     img->image.tweens_count--;
	     free(img->image.tweens[i]);
	     memmove(img->image.tweens + i,
		     img->image.tweens + i + 1,
		     sizeof (Edje_Part_Description_Image*) * (img->image.tweens_count - i));
	     return EINA_TRUE;
	  }
     }
   return EINA_FALSE;
}

EAPI void
edje_edit_state_image_border_get(Evas_Object *obj, const char *part, const char *state, double value, int *l, int *r, int *t, int *b)
{
   Edje_Part_Description_Image *img;

   GET_PD_OR_RETURN();

   if (rp->part->type != EDJE_PART_TYPE_IMAGE)
     {
        if (l) *l = 0;
        if (r) *r = 0;
        if (t) *t = 0;
        if (b) *b = 0;
        return;
     }

   img = (Edje_Part_Description_Image *) pd;

   //printf("GET IMAGE_BORDER of state '%s'\n", state);

   if (l) *l = img->image.border.l;
   if (r) *r = img->image.border.r;
   if (t) *t = img->image.border.t;
   if (b) *b = img->image.border.b;
}

EAPI void
edje_edit_state_image_border_set(Evas_Object *obj, const char *part, const char *state, double value, int l, int r, int t, int b)
{
   Edje_Part_Description_Image *img;

   GET_PD_OR_RETURN();

   if (rp->part->type != EDJE_PART_TYPE_IMAGE)
     return;

   img = (Edje_Part_Description_Image *) pd;

   //printf("SET IMAGE_BORDER of state '%s'\n", state);

   if (l > -1) img->image.border.l = l;
   if (r > -1) img->image.border.r = r;
   if (t > -1) img->image.border.t = t;
   if (b > -1) img->image.border.b = b;

   edje_object_calc_force(obj);
}

EAPI unsigned char
edje_edit_state_image_border_fill_get(Evas_Object *obj, const char *part, const char *state, double value)
{
   Edje_Part_Description_Image *img;

   GET_PD_OR_RETURN(0);

   if (rp->part->type != EDJE_PART_TYPE_IMAGE)
     return 0;

   img = (Edje_Part_Description_Image *) pd;

   if (img->image.border.no_fill == 0) return 1;
   else if (img->image.border.no_fill == 1) return 0;
   else if (img->image.border.no_fill == 2) return 2;
   return 0;
}

EAPI void
edje_edit_state_image_border_fill_set(Evas_Object *obj, const char *part, const char *state, double value, unsigned char fill)
{
   Edje_Part_Description_Image *img;

   GET_PD_OR_RETURN();

   if (rp->part->type != EDJE_PART_TYPE_IMAGE)
     return;

   img = (Edje_Part_Description_Image *) pd;

   if (fill == 0) img->image.border.no_fill = 1;
   else if (fill == 1) img->image.border.no_fill = 0;
   else if (fill == 2) img->image.border.no_fill = 2;

   edje_object_calc_force(obj);
}

/******************/
/*  PROGRAMS API  */
/******************/
Edje_Program *
_edje_program_get_byname(Evas_Object *obj, const char *prog_name)
{
   Edje_Program *epr;
   int i;

   GET_ED_OR_RETURN(NULL);

   if (!prog_name) return NULL;

   for (i = 0; i < ed->table_programs_size; i++)
     {
	epr = ed->table_programs[i];
	if ((epr->name) && (strcmp(epr->name, prog_name) == 0))
	  return epr;
     }
   return NULL;
}

EAPI Eina_List *
edje_edit_programs_list_get(Evas_Object *obj)
{
   Eina_List *progs = NULL;
   int i;

   GET_ED_OR_RETURN(NULL);

   //printf("EE: Found %d programs\n", ed->table_programs_size);

   for (i = 0; i < ed->table_programs_size; i++)
     {
	Edje_Program *epr;

	epr = ed->table_programs[i];
	progs = eina_list_append(progs, eina_stringshare_add(epr->name));
     }

   return progs;
}

EAPI Eina_Bool
edje_edit_program_add(Evas_Object *obj, const char *name)
{
   Edje_Program *epr;
   Edje_Part_Collection *pc;

   GET_ED_OR_RETURN(EINA_FALSE);

   //printf("ADD PROGRAM [new name: %s]\n", name);

   //Check if program already exists
   if (_edje_program_get_byname(obj, name))
     return EINA_FALSE;

   //Alloc Edje_Program or return
   epr = _alloc(sizeof(Edje_Program));
   if (!epr) return EINA_FALSE;

   //Add program to group
   pc = ed->collection;

   /* By default, source and signal are empty, so they fill in nocmp category */
   ed->collection->programs.nocmp = realloc(ed->collection->programs.nocmp,
					    sizeof (Edje_Program*) * (ed->collection->programs.nocmp_count + 1));
   ed->collection->programs.nocmp[ed->collection->programs.nocmp_count++] = epr;

   //Init Edje_Program
   epr->id = ed->table_programs_size;
   epr->name = eina_stringshare_add(name);
   epr->signal = NULL;
   epr->source = NULL;
   epr->filter.part = NULL;
   epr->filter.state = NULL;
   epr->in.from = 0.0;
   epr->in.range = 0.0;
   epr->action = 0;
   epr->state = NULL;
   epr->value = 0.0;
   epr->state2 = NULL;
   epr->value2 = 0.0;
   epr->tween.mode = 1;
   epr->tween.time = ZERO;
   epr->targets = NULL;
   epr->after = NULL;


   //Update table_programs
   ed->table_programs_size++;
   ed->table_programs = realloc(ed->table_programs,
                                sizeof(Edje_Program *) * ed->table_programs_size);
   ed->table_programs[epr->id % ed->table_programs_size] = epr;

   //Update patterns
   _edje_programs_patterns_clean(ed);
   _edje_programs_patterns_init(ed);

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_program_del(Evas_Object *obj, const char *prog)
{
   Eina_List *l, *l_next;
   Edje_Program_Target *prt;
   Edje_Program_After *pa;
   Edje_Part_Collection *pc;
   Edje_Program *p;
   int id, i;
   int old_id = -1;

   GET_ED_OR_RETURN(EINA_FALSE);
   GET_EPR_OR_RETURN(EINA_FALSE);

   pc = ed->collection;

   //Remove program from programs list
   id = epr->id;
   edje_edit_program_remove(ed, epr);

   /* fix table program */
   if (epr->id != ed->table_programs_size - 1)
     {
	/* If the removed program is not the last in the list/table,
	 * put the last one in its place and update references to it later */
	ed->table_programs[epr->id] = ed->table_programs[ed->table_programs_size - 1];
	old_id = ed->table_programs_size - 1;
	ed->table_programs[epr->id]->id = epr->id;
     }

   //Free Edje_Program
   _edje_if_string_free(ed, epr->name);
   _edje_if_string_free(ed, epr->signal);
   _edje_if_string_free(ed, epr->source);
   _edje_if_string_free(ed, epr->filter.part);
   _edje_if_string_free(ed, epr->filter.state);
   _edje_if_string_free(ed, epr->state);
   _edje_if_string_free(ed, epr->state2);

   EINA_LIST_FREE(epr->targets, prt)
     free(prt);
   EINA_LIST_FREE(epr->after, pa)
     free(pa);
   free(epr);

   ed->table_programs_size--;
   ed->table_programs = realloc(ed->table_programs,
                             sizeof(Edje_Program *) * ed->table_programs_size);

   //We also update all other programs that point to old_id and id
   for (i = 0; i < ed->table_programs_size; i++)
     {
	Edje_Program_After *pa;

	p = ed->table_programs[i];

	/* check in afters */
	EINA_LIST_FOREACH_SAFE(p->after, l, l_next, pa)
	  {
	     if (pa->id == old_id)
	       pa->id = id;
	     else if (pa->id == id)
	       p->after = eina_list_remove_list(p->after, l);
	  }
	/* check in targets */
	if (p->action == EDJE_ACTION_TYPE_ACTION_STOP)
	  {
	     Edje_Program_Target *pt;

	     EINA_LIST_FOREACH_SAFE(p->targets, l, l_next, pt)
	       {
		  if (pt->id == old_id)
		    pt->id = id;
		  else if (pt->id == id)
		    p->targets = eina_list_remove_list(p->targets, l);
	       }
	  }
     }

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_program_exist(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(EINA_FALSE);

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_program_run(Evas_Object *obj, const char *prog)
{
   GET_ED_OR_RETURN(EINA_FALSE);
   GET_EPR_OR_RETURN(EINA_FALSE);

   _edje_program_run(ed, epr, 0, "", "");
   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_program_name_set(Evas_Object *obj, const char *prog, const char* new_name)
{
   GET_ED_OR_RETURN(EINA_FALSE);
   GET_EPR_OR_RETURN(EINA_FALSE);

   if (!new_name) return EINA_FALSE;

   if (_edje_program_get_byname(obj, new_name)) return EINA_FALSE;

   //printf("SET NAME for program: %s [new name: %s]\n", prog, new_name);

   _edje_if_string_free(ed, epr->name);
   epr->name = eina_stringshare_add(new_name);

   return EINA_TRUE;
}

EAPI const char *
edje_edit_program_source_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(NULL);

   if (!epr->source) return NULL;
   //printf("GET SOURCE for program: %s [%s]\n", prog, epr->source);
   return eina_stringshare_add(epr->source);
}

EAPI Eina_Bool
edje_edit_program_source_set(Evas_Object *obj, const char *prog, const char *source)
{
   GET_ED_OR_RETURN(EINA_FALSE);
   GET_EPR_OR_RETURN(EINA_FALSE);

   if (!source) return EINA_FALSE;

   /* Remove from program array */
   edje_edit_program_remove(ed, epr);
   _edje_if_string_free(ed, epr->source);

   /* Insert it back */
   epr->source = eina_stringshare_add(source);
   edje_edit_program_insert(ed, epr);

   //Update patterns
   _edje_programs_patterns_clean(ed);
   _edje_programs_patterns_init(ed);

   return EINA_TRUE;
}

EAPI const char *
edje_edit_program_filter_part_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(NULL);

   if (!epr->filter.part) return NULL;
   return eina_stringshare_add(epr->filter.part);
}

EAPI Eina_Bool
edje_edit_program_filter_part_set(Evas_Object *obj, const char *prog, const char *filter_part)
{
   GET_ED_OR_RETURN(EINA_FALSE);
   GET_EPR_OR_RETURN(EINA_FALSE);

   if (!filter_part) return EINA_FALSE;

   _edje_if_string_free(ed, epr->filter.part);
   epr->filter.part = eina_stringshare_add(filter_part);

   return EINA_TRUE;
}

EAPI const char *
edje_edit_program_filter_state_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(NULL);

   if (!epr->filter.state) return NULL;
   return eina_stringshare_add(epr->filter.state);
}

EAPI Eina_Bool
edje_edit_program_filter_state_set(Evas_Object *obj, const char *prog, const char *filter_state)
{
   GET_ED_OR_RETURN(EINA_FALSE);
   GET_EPR_OR_RETURN(EINA_FALSE);

   if (!filter_state) return EINA_FALSE;

   _edje_if_string_free(ed, epr->filter.state);
   epr->filter.state = eina_stringshare_add(filter_state);

   return EINA_TRUE;
}

EAPI const char *
edje_edit_program_signal_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(NULL);

   if (!epr->signal) return NULL;
   //printf("GET SIGNAL for program: %s [%s]\n", prog, epr->signal);
   return eina_stringshare_add(epr->signal);
}

EAPI Eina_Bool
edje_edit_program_signal_set(Evas_Object *obj, const char *prog, const char *signal)
{
   GET_ED_OR_RETURN(EINA_FALSE);
   GET_EPR_OR_RETURN(EINA_FALSE);

   if (!signal) return EINA_FALSE;

   /* Remove from program array */
   edje_edit_program_remove(ed, epr);
   _edje_if_string_free(ed, epr->signal);

   /* Insert it back */
   epr->signal = eina_stringshare_add(signal);
   edje_edit_program_insert(ed, epr);

   //Update patterns
   _edje_programs_patterns_clean(ed);
   _edje_programs_patterns_init(ed);

   return EINA_TRUE;
}

EAPI const char *
edje_edit_program_state_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(NULL);

   if (!epr->state) return NULL;
   //printf("GET STATE for program: %s [%s %.2f]\n", prog, epr->state, epr->value);
   return eina_stringshare_add(epr->state);
}

EAPI Eina_Bool
edje_edit_program_state_set(Evas_Object *obj, const char *prog, const char *state)
{
   GET_ED_OR_RETURN(EINA_FALSE);
   GET_EPR_OR_RETURN(EINA_FALSE);

   //printf("SET STATE for program: %s\n", prog);

   _edje_if_string_free(ed, epr->state);
   epr->state = eina_stringshare_add(state);

   return EINA_TRUE;
}

EAPI const char *
edje_edit_program_state2_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(NULL);

   if (!epr->state2) return NULL;
   //printf("GET STATE2 for program: %s [%s %.2f]\n", prog, epr->state2, epr->value2);
   return eina_stringshare_add(epr->state2);
}

EAPI Eina_Bool
edje_edit_program_state2_set(Evas_Object *obj, const char *prog, const char *state2)
{
   GET_ED_OR_RETURN(EINA_FALSE);
   GET_EPR_OR_RETURN(EINA_FALSE);

   //printf("SET STATE2 for program: %s\n", prog);

   _edje_if_string_free(ed, epr->state2);
   epr->state2 = eina_stringshare_add(state2);

   return EINA_TRUE;
}

EAPI double
edje_edit_program_value_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(-1);

   //printf("GET VALUE for program: %s [%s %.2f]\n", prog, epr->state, epr->value);
   return epr->value;
}

EAPI Eina_Bool
edje_edit_program_value_set(Evas_Object *obj, const char *prog, double value)
{
   GET_EPR_OR_RETURN(EINA_FALSE);

   //printf("SET VALUE for program: %s [%.2f]\n", prog, value);
   epr->value = value;
   return EINA_TRUE;
}

EAPI double
edje_edit_program_value2_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(-1);

   //printf("GET VALUE2 for program: %s [%s %.2f]\n", prog, epr->state2, epr->value2);
   return epr->value2;
}

EAPI Eina_Bool
edje_edit_program_value2_set(Evas_Object *obj, const char *prog, double value)
{
   GET_EPR_OR_RETURN(EINA_FALSE);

   //printf("SET VALUE for program: %s [%.2f]\n", prog, value);
   epr->value2 = value;
   return EINA_TRUE;
}

EAPI double
edje_edit_program_in_from_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(0);

   //printf("GET IN.FROM for program: %s [%f]\n", prog, epr->in.from);
   return epr->in.from;
}

EAPI Eina_Bool
edje_edit_program_in_from_set(Evas_Object *obj, const char *prog, double seconds)
{
   GET_EPR_OR_RETURN(EINA_FALSE);

   //printf("SET IN.FROM for program: %s [%f]\n", prog, epr->in.from);
   epr->in.from = seconds;
   return EINA_TRUE;
}

EAPI double
edje_edit_program_in_range_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(0);

   //printf("GET IN.RANGE for program: %s [%f]\n", prog, epr->in.range);
   return epr->in.range;
}

EAPI Eina_Bool
edje_edit_program_in_range_set(Evas_Object *obj, const char *prog, double seconds)
{
   GET_EPR_OR_RETURN(EINA_FALSE);

   //printf("SET IN.RANGE for program: %s [%f]\n", prog, epr->in.range);
   epr->in.range = seconds;
   return EINA_TRUE;
}

EAPI Edje_Tween_Mode
edje_edit_program_transition_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(-1);

   //printf("GET TRANSITION for program: %s [%d]\n", prog, epr->tween.mode);
   return epr->tween.mode;
}

EAPI Eina_Bool
edje_edit_program_transition_set(Evas_Object *obj, const char *prog, Edje_Tween_Mode transition)
{
   GET_EPR_OR_RETURN(EINA_FALSE);

   //printf("GET TRANSITION for program: %s [%d]\n", prog, epr->tween.mode);
   epr->tween.mode = transition;
   return EINA_TRUE;
}

EAPI double
edje_edit_program_transition_time_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(-1);

   //printf("GET TRANSITION_TIME for program: %s [%.4f]\n", prog, epr->tween.time);
   return TO_DOUBLE(epr->tween.time);
}

EAPI Eina_Bool
edje_edit_program_transition_time_set(Evas_Object *obj, const char *prog, double seconds)
{
   GET_EPR_OR_RETURN(EINA_FALSE);

   //printf("GET TRANSITION_TIME for program: %s [%.4f]\n", prog, epr->tween.time);
   epr->tween.time = FROM_DOUBLE(seconds);
   return EINA_TRUE;
}

EAPI Edje_Action_Type
edje_edit_program_action_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(-1);

   //printf("GET ACTION for program: %s [%d]\n", prog, epr->action);
   return epr->action;
}

EAPI Eina_Bool
edje_edit_program_action_set(Evas_Object *obj, const char *prog, Edje_Action_Type action)
{
   GET_EPR_OR_RETURN(EINA_FALSE);

   //printf("SET ACTION for program: %s [%d]\n", prog, action);
   if (action >= EDJE_ACTION_TYPE_LAST) return EINA_FALSE;

   epr->action = action;
   return EINA_TRUE;
}

EAPI Eina_List *
edje_edit_program_targets_get(Evas_Object *obj, const char *prog)
{
   Eina_List *l, *targets = NULL;
   Edje_Program_Target *t;

   GET_ED_OR_RETURN(NULL);
   GET_EPR_OR_RETURN(NULL);

   //printf("GET TARGETS for program: %s [count: %d]\n", prog, eina_list_count(epr->targets));
   EINA_LIST_FOREACH(epr->targets, l, t)
     {
	if (epr->action == EDJE_ACTION_TYPE_STATE_SET)
	  {
	     /* the target is a part */
	     Edje_Real_Part *p = NULL;

	     p = ed->table_parts[t->id % ed->table_parts_size];
	     if (p && p->part && p->part->name)
	       targets = eina_list_append(targets,
		     eina_stringshare_add(p->part->name));
	  }
	else if (epr->action == EDJE_ACTION_TYPE_ACTION_STOP)
	  {
	     /* the target is a program */
	     Edje_Program *p;

	     p = ed->table_programs[t->id % ed->table_programs_size];
	     if (p && p->name)
	       targets = eina_list_append(targets,
		     eina_stringshare_add(p->name));
	  }
     }
   return targets;
}

EAPI Eina_Bool
edje_edit_program_targets_clear(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(EINA_FALSE);

   while (epr->targets)
     {
	Edje_Program_Target *prt;

	prt = eina_list_data_get(epr->targets);
	epr->targets = eina_list_remove_list(epr->targets, epr->targets);
	free(prt);
     }

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_program_target_add(Evas_Object *obj, const char *prog, const char *target)
{
   int id;
   Edje_Program_Target *t;

   GET_ED_OR_RETURN(EINA_FALSE);
   GET_EPR_OR_RETURN(EINA_FALSE);

   if (epr->action == EDJE_ACTION_TYPE_STATE_SET)
     {
	/* the target is a part */
	Edje_Real_Part *rp;

	rp = _edje_real_part_get(ed, target);
	if (!rp) return EINA_FALSE;
	id = rp->part->id;
     }
   else if (epr->action == EDJE_ACTION_TYPE_ACTION_STOP)
     {
	/* the target is a program */
	Edje_Program *tar;

	tar = _edje_program_get_byname(obj, target);
	if (!tar) return EINA_FALSE;
	id = tar->id;
     }
   else
     return EINA_FALSE;

   t = _alloc(sizeof(Edje_Program_Target));
   if (!t) return EINA_FALSE;

   t->id = id;
   epr->targets = eina_list_append(epr->targets, t);

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_program_target_del(Evas_Object *obj, const char *prog, const char *target)
{
   int id;
   Eina_List *l;
   Edje_Program_Target *t;

   GET_ED_OR_RETURN(EINA_FALSE);
   GET_EPR_OR_RETURN(EINA_FALSE);

   if (epr->action == EDJE_ACTION_TYPE_STATE_SET)
     {
	/* the target is a part */
	Edje_Real_Part *rp;

	rp = _edje_real_part_get(ed, target);
	if (!rp) return EINA_FALSE;
	id = rp->part->id;
     }
   else if (epr->action == EDJE_ACTION_TYPE_ACTION_STOP)
     {
	/* the target is a program */
	Edje_Program *tar;

	tar = _edje_program_get_byname(obj, target);
	if (!tar) return EINA_FALSE;
	id = tar->id;
     }
   else
     return EINA_FALSE;

   EINA_LIST_FOREACH(epr->targets, l, t)
      if (t->id == id)
	break;
   epr->targets = eina_list_remove_list(epr->targets, l);
   free(t);

   return EINA_TRUE;
}

EAPI Eina_List *
edje_edit_program_afters_get(Evas_Object *obj, const char *prog)
{
   Eina_List *l, *afters = NULL;
   Edje_Program_After *a;

   GET_ED_OR_RETURN(NULL);
   GET_EPR_OR_RETURN(NULL);

  // printf("GET AFTERS for program: %s [count: %d]\n", prog, eina_list_count(epr->after));
   EINA_LIST_FOREACH(epr->after, l, a)
     {
	Edje_Program *p = NULL;

	p = ed->table_programs[a->id % ed->table_programs_size];
	if (p && p->name)
	  {
	     //printf("   a: %d name: %s\n", a->id, p->name);
	     afters = eina_list_append(afters, eina_stringshare_add(p->name));
	  }
     }
   return afters;
}

EAPI Eina_Bool
edje_edit_program_afters_clear(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(EINA_FALSE);

   while (epr->after)
     {
	Edje_Program_After *pa;

	pa = eina_list_data_get(epr->after);
	epr->after = eina_list_remove_list(epr->after, epr->after);
	free(pa);
     }

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_program_after_add(Evas_Object *obj, const char *prog, const char *after)
{
   Edje_Program *af;
   Edje_Program_After *a;

   GET_EPR_OR_RETURN(EINA_FALSE);

   af = _edje_program_get_byname(obj, after);
   if (!af) return EINA_FALSE;

   a = _alloc(sizeof(Edje_Program_After));
   if (!a) return EINA_FALSE;

   a->id = af->id;

   epr->after = eina_list_append(epr->after, a);

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_program_after_del(Evas_Object *obj, const char *prog, const char *after)
{
   Edje_Program *af;
   Edje_Program_After *a;
   Eina_List *l;

   GET_EPR_OR_RETURN(EINA_FALSE);

   af = _edje_program_get_byname(obj, after);
   if (!af) return EINA_FALSE;

   EINA_LIST_FOREACH(epr->after, l, a)
      if (a->id == af->id)
	{
	   epr->after = eina_list_remove_list(epr->after, l);
	   break;
	}

   return EINA_TRUE;
}

EAPI const char *
edje_edit_program_api_name_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(NULL);

   return eina_stringshare_add(epr->api.name);
}

EAPI const char *
edje_edit_program_api_description_get(Evas_Object *obj, const char *prog)
{
   GET_EPR_OR_RETURN(NULL);

   return eina_stringshare_add(epr->api.description);
}

EAPI Eina_Bool
edje_edit_program_api_name_set(Evas_Object *obj, const char *prog, const char* name)
{
   GET_ED_OR_RETURN(EINA_FALSE);
   GET_EPR_OR_RETURN(EINA_FALSE);

   _edje_if_string_free(ed, epr->api.name);
   epr->api.name = eina_stringshare_add(name);

   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_program_api_description_set(Evas_Object *obj, const char *prog, const char *description)
{
   GET_ED_OR_RETURN(EINA_FALSE);
   GET_EPR_OR_RETURN(EINA_FALSE);

   _edje_if_string_free(ed, epr->api.description);
   epr->api.description = eina_stringshare_add(description);

   return EINA_TRUE;
}

/*************************/
/*  EMBRYO SCRIPTS  API  */
/*************************/
EAPI const char *
edje_edit_script_get(Evas_Object *obj)
{
   Embryo_Program   *script = NULL;

   GET_ED_OR_RETURN(NULL);

   if (!ed->collection) return NULL;
   if (!ed->collection->script) return NULL;

   script = ed->collection->script;

   printf("Get Script [%p] %d\n", script, embryo_program_recursion_get(script));

   return "Not yet complete...";
}


/***************************/
/*  EDC SOURCE GENERATION  */
/***************************/
#define I0 ""
#define I1 "   "
#define I2 "      "
#define I3 "         "
#define I4 "            "
#define I5 "               "
#define I6 "                  "
#define I7 "                     "

#define BUF_APPEND(STR) \
   ret &= eina_strbuf_append(buf, STR)

#define BUF_APPENDF(FMT, ...) \
   ret &= eina_strbuf_append_printf(buf, FMT, ##__VA_ARGS__)

static char *types[] = {"NONE", "RECT", "TEXT", "IMAGE", "SWALLOW", "TEXTBLOCK", "GRADIENT", "GROUP", "BOX", "TABLE", "EXTERNAL"};
static char *effects[] = {"NONE", "PLAIN", "OUTLINE", "SOFT_OUTLINE", "SHADOW", "SOFT_SHADOW", "OUTLINE_SHADOW", "OUTLINE_SOFT_SHADOW ", "FAR_SHADOW ", "FAR_SOFT_SHADOW", "GLOW"};
static char *prefers[] = {"NONE", "VERTICAL", "HORIZONTAL", "BOTH"};

 static Eina_Bool
_edje_generate_source_of_colorclass(Edje * ed, const char *name, Eina_Strbuf *buf)
{
   Eina_List *l;
   Edje_Color_Class *cc;
   Eina_Bool ret = EINA_TRUE;

   EINA_LIST_FOREACH(ed->file->color_classes, l, cc)
     if (!strcmp(cc->name, name))
       {
	 BUF_APPENDF(I1 "color_class { name: \"%s\";\n", cc->name);
	 BUF_APPENDF(I2 "color: %d %d %d %d;\n", cc->r, cc->g, cc->b, cc->a);
	 BUF_APPENDF(I2 "color2: %d %d %d %d;\n", cc->r2, cc->g2, cc->b2, cc->a2);
	 BUF_APPENDF(I2 "color3: %d %d %d %d;\n", cc->r3, cc->g3, cc->b3, cc->a3);
	 BUF_APPEND(I1 "}\n");
       }
   return ret;
}

 static Eina_Bool
_edje_generate_source_of_style(Edje * ed, const char *name, Eina_Strbuf *buf)
{
   Eina_List *l, *ll;
   Edje_Style *s;
   Edje_Style_Tag *t;
   Eina_Bool ret = EINA_TRUE;

   EINA_LIST_FOREACH(ed->file->styles, l, s)
     if (!strcmp(s->name, name))
       {
	 t = s->tags ? s->tags->data : NULL;
	 BUF_APPENDF(I1 "style { name:\"%s\";\n", s->name);
	 if (t && t->value)
	   BUF_APPENDF(I2 "base: \"%s\";\n", t->value);

	 EINA_LIST_FOREACH(s->tags, ll, t)
	   if (ll->prev && t && t->value)
	     BUF_APPENDF(I2 "tag: \"%s\" \"%s\";\n", t->key,
			        t->value);
	 BUF_APPEND(I1 "}\n");
	 return ret;
       }
   return EINA_FALSE;
}

static Eina_Bool
_edje_generate_source_of_program(Evas_Object *obj, const char *program, Eina_Strbuf *buf)
{
   Eina_List *l, *ll;
   const char *s, *s2;
   double db, db2;
   char *data;
   Eina_Bool ret = EINA_TRUE;
   const char *api_name, *api_description;

   GET_ED_OR_RETURN(EINA_FALSE);

   BUF_APPENDF(I3"program { name: \"%s\";\n", program);

   /* Signal */
   if ((s = edje_edit_program_signal_get(obj, program)))
     {
	BUF_APPENDF(I4"signal: \"%s\";\n", s);
	edje_edit_string_free(s);
     }

   /* Source */
   if ((s = edje_edit_program_source_get(obj, program)))
     {
	BUF_APPENDF(I4"source: \"%s\";\n", s);
	edje_edit_string_free(s);
     }

   /* Action */
   switch (edje_edit_program_action_get(obj, program))
     {
     case EDJE_ACTION_TYPE_ACTION_STOP:
	BUF_APPEND(I4"action: ACTION_STOP;\n");
	break;
     case EDJE_ACTION_TYPE_STATE_SET:
	if ((s = edje_edit_program_state_get(obj, program)))
	  {
		BUF_APPENDF(I4"action: STATE_SET \"%s\" %.2f;\n", s,
			edje_edit_program_value_get(obj, program));
		edje_edit_string_free(s);
	  }
	break;
     case EDJE_ACTION_TYPE_SIGNAL_EMIT:
	s = edje_edit_program_state_get(obj, program);
	s2 = edje_edit_program_state2_get(obj, program);
	if (s && s2)
	  {
		BUF_APPENDF(I4"action: SIGNAL_EMIT \"%s\" \"%s\";\n", s, s2);
		edje_edit_string_free(s);
		edje_edit_string_free(s2);
	  }
	break;
     //TODO Support Drag
     //~ case EDJE_ACTION_TYPE_DRAG_VAL_SET:
	//~ eina_strbuf_append(buf, I4"action: DRAG_VAL_SET TODO;\n");
	//~ break;
     //~ case EDJE_ACTION_TYPE_DRAG_VAL_STEP:
	//~ eina_strbuf_append(buf, I4"action: DRAG_VAL_STEP TODO;\n");
	//~ break;
     //~ case EDJE_ACTION_TYPE_DRAG_VAL_PAGE:
	//~ eina_strbuf_append(buf, I4"action: DRAG_VAL_PAGE TODO;\n");
	//~ break;
     default:
	break;
     }

   /* Transition */
   db = edje_edit_program_transition_time_get(obj, program);
   switch (edje_edit_program_transition_get(obj, program))
     {
     case EDJE_TWEEN_MODE_LINEAR:
	BUF_APPENDF(I4"transition: LINEAR %.5f;\n", db);
	break;
     case EDJE_TWEEN_MODE_ACCELERATE:
	BUF_APPENDF(I4"transition: ACCELERATE %.5f;\n", db);
	break;
     case EDJE_TWEEN_MODE_DECELERATE:
	BUF_APPENDF(I4"transition: DECELERATE %.5f;\n", db);
	break;
     case EDJE_TWEEN_MODE_SINUSOIDAL:
	BUF_APPENDF(I4"transition: SINUSOIDAL %.5f;\n", db);
	break;
     default:
	break;
     }

   /* In */
   db = edje_edit_program_in_from_get(obj, program);
   db2 = edje_edit_program_in_range_get(obj, program);
   if (db || db2)
     BUF_APPENDF(I4"in: %.5f %.5f;\n", db, db2);

   /* Targets */
   if ((ll = edje_edit_program_targets_get(obj, program)))
     {
	EINA_LIST_FOREACH(ll, l, data)
	  BUF_APPENDF(I4"target: \"%s\";\n", data);
	edje_edit_string_list_free(ll);
     }

   /* Afters */
   if ((ll = edje_edit_program_afters_get(obj, program)))
     {
        EINA_LIST_FOREACH(ll, l, data)
	  BUF_APPENDF(I4"after: \"%s\";\n", data);
	edje_edit_string_list_free(ll);
     }

   // TODO Support script {}
   /* api */
   api_name = edje_edit_program_api_name_get(obj, program);
   api_description = edje_edit_program_api_description_get(obj, program);

   if (api_name || api_description)
     {
	if (api_name && api_description)
	  {
	     BUF_APPENDF(I4"api: \"%s\" \"%s\";\n", api_name, api_description);
	     edje_edit_string_free(api_name);
	     edje_edit_string_free(api_description);
	  }
	else
	  if (api_name)
	    {
	       BUF_APPENDF(I4"api: \"%s\" \"\";\n", api_name);
	       edje_edit_string_free(api_name);
	    }
	  else
	    {
	       BUF_APPENDF(I4"api: \"\" \"%s\";\n", api_description);
	       edje_edit_string_free(api_description);
	    }
     }

   BUF_APPEND(I3 "}\n");
   return ret;
}

static Eina_Bool
_edje_generate_source_of_state(Evas_Object *obj, const char *part, const char *state, double value, Eina_Strbuf *buf)
{
   Eina_List *l, *ll;
   Eina_Bool ret = EINA_TRUE;

   GET_PD_OR_RETURN(EINA_FALSE);

   BUF_APPENDF(I4"description { state: \"%s\" %g;\n", pd->state.name, pd->state.value);
   //TODO Support inherit

   if (!pd->visible)
     BUF_APPEND(I5"visible: 0;\n");

   if (pd->align.x != 0.5 || pd->align.y != 0.5)
     BUF_APPENDF(I5"align: %g %g;\n", TO_DOUBLE(pd->align.x), TO_DOUBLE(pd->align.y));

   //TODO Support fixed

   if (pd->min.w || pd->min.h)
     BUF_APPENDF(I5"min: %d %d;\n", pd->min.w, pd->min.h);
   if (pd->max.w != -1 || pd->max.h != -1)
     BUF_APPENDF(I5"max: %d %d;\n", pd->max.w, pd->max.h);

   //TODO Support step

   if (pd->aspect.min || pd->aspect.max)
      BUF_APPENDF(I5"aspect: %g %g;\n", TO_DOUBLE(pd->aspect.min), TO_DOUBLE(pd->aspect.max));
   if (pd->aspect.prefer)
      BUF_APPENDF(I5"aspect_preference: %s;\n", prefers[pd->aspect.prefer]);

   if (pd->color_class)
     BUF_APPENDF(I5"color_class: \"%s\";\n", pd->color_class);

   if (pd->color.r != 255 || pd->color.g != 255 ||
       pd->color.b != 255 || pd->color.a != 255)
     BUF_APPENDF(I5"color: %d %d %d %d;\n",
		 pd->color.r, pd->color.g, pd->color.b, pd->color.a);
   if (pd->color2.r != 0 || pd->color2.g != 0 ||
       pd->color2.b != 0 || pd->color2.a != 255)
     BUF_APPENDF(I5"color2: %d %d %d %d;\n",
		 pd->color2.r, pd->color2.g, pd->color2.b, pd->color2.a);

   if (rp->part->type == EDJE_PART_TYPE_TEXT
       || rp->part->type == EDJE_PART_TYPE_TEXTBLOCK)
     {
	Edje_Part_Description_Text *txt;

	txt = (Edje_Part_Description_Text *) pd;

	if (txt->text.color3.r != 0 || txt->text.color3.g != 0 ||
	    txt->text.color3.b != 0 || txt->text.color3.a != 128)
	  BUF_APPENDF(I5"color3: %d %d %d %d;\n",
		      txt->text.color3.r, txt->text.color3.g, txt->text.color3.b, txt->text.color3.a);
     }

   //Rel1
   if (pd->rel1.relative_x || pd->rel1.relative_y || pd->rel1.offset_x ||
       pd->rel1.offset_y || pd->rel1.id_x != -1 || pd->rel1.id_y != -1)
     {
	BUF_APPEND(I5"rel1 {\n");
	if (pd->rel1.relative_x || pd->rel1.relative_y)
	  BUF_APPENDF(I6"relative: %g %g;\n", TO_DOUBLE(pd->rel1.relative_x), TO_DOUBLE(pd->rel1.relative_y));
	if (pd->rel1.offset_x || pd->rel1.offset_y)
	  BUF_APPENDF(I6"offset: %d %d;\n", pd->rel1.offset_x, pd->rel1.offset_y);
	if (pd->rel1.id_x != -1 && pd->rel1.id_x == pd->rel1.id_y)
	  BUF_APPENDF(I6"to: \"%s\";\n", ed->table_parts[pd->rel1.id_x]->part->name);
	else
	  {
		if (pd->rel1.id_x != -1)
		  BUF_APPENDF(I6"to_x: \"%s\";\n", ed->table_parts[pd->rel1.id_x]->part->name);
		if (pd->rel1.id_y != -1)
		  BUF_APPENDF(I6"to_y: \"%s\";\n", ed->table_parts[pd->rel1.id_y]->part->name);
	  }
	BUF_APPEND(I5"}\n");//rel1
     }

   //Rel2
   if (pd->rel2.relative_x != 1.0 || pd->rel2.relative_y != 1.0 ||
       pd->rel2.offset_x != -1 || pd->rel2.offset_y != -1 ||
       pd->rel2.id_x != -1 || pd->rel2.id_y != -1)
     {
	BUF_APPEND(I5"rel2 {\n");
	if (TO_DOUBLE(pd->rel2.relative_x) != 1.0 || TO_DOUBLE(pd->rel2.relative_y) != 1.0)
	  BUF_APPENDF(I6"relative: %g %g;\n", TO_DOUBLE(pd->rel2.relative_x), TO_DOUBLE(pd->rel2.relative_y));
	if (pd->rel2.offset_x != -1 || pd->rel2.offset_y != -1)
	  BUF_APPENDF(I6"offset: %d %d;\n", pd->rel2.offset_x, pd->rel2.offset_y);
	if (pd->rel2.id_x != -1 && pd->rel2.id_x == pd->rel2.id_y)
	  BUF_APPENDF(I6"to: \"%s\";\n", ed->table_parts[pd->rel2.id_x]->part->name);
	else
	  {
		if (pd->rel2.id_x != -1)
		  BUF_APPENDF(I6"to_x: \"%s\";\n", ed->table_parts[pd->rel2.id_x]->part->name);
		if (pd->rel2.id_y != -1)
		  BUF_APPENDF(I6"to_y: \"%s\";\n", ed->table_parts[pd->rel2.id_y]->part->name);
	  }
	BUF_APPEND(I5"}\n");//rel2
     }

   //Image
   if (rp->part->type == EDJE_PART_TYPE_IMAGE)
     {
        char *data;

	Edje_Part_Description_Image *img;

	img = (Edje_Part_Description_Image *) pd;

	BUF_APPEND(I5"image {\n");
	BUF_APPENDF(I6"normal: \"%s\";\n", _edje_image_name_find(obj, img->image.id));

	ll = edje_edit_state_tweens_list_get(obj, part, state, value);
	EINA_LIST_FOREACH(ll, l, data)
	  BUF_APPENDF(I6"tween: \"%s\";\n", data);
	edje_edit_string_list_free(ll);

	if (img->image.border.l || img->image.border.r || img->image.border.t || img->image.border.b)
	  BUF_APPENDF(I6"border: %d %d %d %d;\n", img->image.border.l, img->image.border.r, img->image.border.t, img->image.border.b);
	if (img->image.border.no_fill == 1)
	  BUF_APPEND(I6"middle: NONE;\n");
	else if (img->image.border.no_fill == 0)
	  BUF_APPEND(I6"middle: DEFAULT;\n");
	else if (img->image.border.no_fill == 2)
	  BUF_APPEND(I6"middle: SOLID;\n");

	BUF_APPEND(I5"}\n");//image

	//Fill

	BUF_APPEND(I5"fill {\n");
	if (rp->part->type == EDJE_PART_TYPE_IMAGE && !img->image.fill.smooth)
	  BUF_APPEND(I6"smooth: 0;\n");
        //TODO Support spread

	if (img->image.fill.pos_rel_x || img->image.fill.pos_rel_y ||
            img->image.fill.pos_abs_x || img->image.fill.pos_abs_y)
	  {
		BUF_APPEND(I6"origin {\n");
		if (img->image.fill.pos_rel_x || img->image.fill.pos_rel_y)
		  BUF_APPENDF(I7"relative: %g %g;\n", TO_DOUBLE(img->image.fill.pos_rel_x), TO_DOUBLE(img->image.fill.pos_rel_y));
		if (img->image.fill.pos_abs_x || img->image.fill.pos_abs_y)
		  BUF_APPENDF(I7"offset: %d %d;\n", img->image.fill.pos_abs_x, img->image.fill.pos_abs_y);
		BUF_APPEND(I6"}\n");
          }

	if (TO_DOUBLE(img->image.fill.rel_x) != 1.0 || TO_DOUBLE(img->image.fill.rel_y) != 1.0 ||
            img->image.fill.abs_x || img->image.fill.abs_y)
	  {
		BUF_APPEND(I6"size {\n");
		if (img->image.fill.rel_x != 1.0 || img->image.fill.rel_y != 1.0)
		  BUF_APPENDF(I7"relative: %g %g;\n", TO_DOUBLE(img->image.fill.rel_x), TO_DOUBLE(img->image.fill.rel_y));
		if (img->image.fill.abs_x || img->image.fill.abs_y)
		  BUF_APPENDF(I7"offset: %d %d;\n", img->image.fill.abs_x, img->image.fill.abs_y);
		BUF_APPEND(I6"}\n");
          }

	BUF_APPEND(I5"}\n");
     }

   //Text
   if (rp->part->type == EDJE_PART_TYPE_TEXT)
     {
	Edje_Part_Description_Text *txt;

	txt = (Edje_Part_Description_Text *) pd;

	BUF_APPEND(I5"text {\n");
	if (txt->text.text)
	  BUF_APPENDF(I6"text: \"%s\";\n", txt->text.text);
	BUF_APPENDF(I6"font: \"%s\";\n", txt->text.font);
	BUF_APPENDF(I6"size: %d;\n", txt->text.size);
	if (txt->text.text_class)
	  BUF_APPENDF(I6"text_class: \"%s\";\n", txt->text.text_class);
	if (txt->text.fit_x || txt->text.fit_y)
	  BUF_APPENDF(I6"fit: %d %d;\n", txt->text.fit_x, txt->text.fit_y);
        //TODO Support min & max
	if (TO_DOUBLE(txt->text.align.x) != 0.5 || TO_DOUBLE(txt->text.align.y) != 0.5)
	  BUF_APPENDF(I6"align: %g %g;\n", TO_DOUBLE(txt->text.align.x), TO_DOUBLE(txt->text.align.y));
        //TODO Support source
        //TODO Support text_source
	if (txt->text.elipsis)
	  BUF_APPENDF(I6"elipsis: %g;\n", txt->text.elipsis);
	BUF_APPEND(I5"}\n");
     }

   //External
   if (rp->part->type == EDJE_PART_TYPE_EXTERNAL)
     {
	if ((ll = (Eina_List *)edje_edit_state_external_params_list_get(obj, part, state, value)))
	  {
	     Edje_External_Param *p;

	     BUF_APPEND(I5"params {\n");
	     EINA_LIST_FOREACH(ll, l, p)
	       {
		  switch (p->type)
		    {
		     case EDJE_EXTERNAL_PARAM_TYPE_INT:
			BUF_APPENDF(I6"int: \"%s\" \"%d\";\n", p->name, p->i);
			break;
		     case EDJE_EXTERNAL_PARAM_TYPE_DOUBLE:
			BUF_APPENDF(I6"double: \"%s\" \"%g\";\n", p->name, p->d);
			break;
		     case EDJE_EXTERNAL_PARAM_TYPE_STRING:
			if (p->s)
			  BUF_APPENDF(I6"string: \"%s\" \"%s\";\n", p->name,
				      p->s);
			break;
		     case EDJE_EXTERNAL_PARAM_TYPE_BOOL:
			BUF_APPENDF(I6"bool: \"%s\" \"%d\";\n", p->name, p->i);
			break;
		     case EDJE_EXTERNAL_PARAM_TYPE_CHOICE:
			if (p->s)
			  BUF_APPENDF(I6"choice: \"%s\" \"%s\";\n", p->name,
				      p->s);
			break;
		     default:
			break;
		    }
	       }
	     BUF_APPEND(I5"}\n");
	  }
     }

   BUF_APPEND(I4"}\n");//description
   return ret;
}

static Eina_Bool
_edje_generate_source_of_part(Evas_Object *obj, Edje_Part *ep, Eina_Strbuf *buf)
{
   const char *part = ep->name;
   const char *str;
   Eina_List *l, *ll;
   char *data;
   Eina_Bool ret = EINA_TRUE;
   const char *api_name, *api_description;

   BUF_APPENDF(I3"part { name: \"%s\";\n", part);
   BUF_APPENDF(I4"type: %s;\n", types[edje_edit_part_type_get(obj, part)]);
   if (!edje_edit_part_mouse_events_get(obj, part))
      BUF_APPEND(I4"mouse_events: 0;\n");
   if (edje_edit_part_repeat_events_get(obj, part))
      BUF_APPEND(I4"repeat_events: 1;\n");
   if (edje_edit_part_scale_get(obj, part))
     BUF_APPEND(I4"scale: 1;\n");
   //TODO Support ignore_flags
   //TODO Support pointer_mode
   //TODO Support precise_is_inside
   //TODO Support use_alternate_font_metrics
   if ((str = edje_edit_part_clip_to_get(obj, part)))
     {
        BUF_APPENDF(I4"clip_to: \"%s\";\n", str);
        edje_edit_string_free(str);
     }
   if ((str = edje_edit_part_source_get(obj, part)))
     {
        BUF_APPENDF(I4"source: \"%s\";\n", str);
        edje_edit_string_free(str);
     }
   if (edje_edit_part_effect_get(obj, part))
     BUF_APPENDF(I4"effect: %s;\n",
	         effects[edje_edit_part_effect_get(obj, part)]);

   //Dragable
   if (edje_edit_part_drag_x_get(obj, part) ||
       edje_edit_part_drag_x_get(obj, part))
     {
	BUF_APPEND(I4"dragable {\n");
	BUF_APPENDF(I5"x: %d %d %d;\n",
				  edje_edit_part_drag_x_get(obj, part),
				  edje_edit_part_drag_step_x_get(obj, part),
				  edje_edit_part_drag_count_x_get(obj, part));
	BUF_APPENDF(I5"y: %d %d %d;\n",
				  edje_edit_part_drag_y_get(obj, part),
				  edje_edit_part_drag_step_y_get(obj, part),
				  edje_edit_part_drag_count_y_get(obj, part));
	if ((str = edje_edit_part_drag_confine_get(obj, part)))
	  {
		BUF_APPENDF(I5"confine: \"%s\";\n", str);
		edje_edit_string_free(str);
	  }
	if ((str = edje_edit_part_drag_event_get(obj, part)))
	  {
		BUF_APPENDF(I5"events: \"%s\";\n", str);
		edje_edit_string_free(str);
	  }
	BUF_APPEND(I4"}\n");
     }

   //Descriptions
   ll = edje_edit_part_states_list_get(obj, part);
   EINA_LIST_FOREACH(ll, l, data)
     {
	char state[512], *delim;
	double value;
	strncpy(state, data, sizeof(state) - 1); /* if we go over it, too bad.. the list of states may need to change to provide name and value separated */
	delim = strchr(state, ' ');
	*delim = '\0';
	delim++;
	value = strtod(delim, NULL);
	ret &= _edje_generate_source_of_state(obj, part, state, value, buf);
     }
   edje_edit_string_list_free(ll);

   api_name = edje_edit_part_api_name_get(obj, part);
   api_description = edje_edit_part_api_description_get(obj, part);

   if (api_name || api_description)
     {
	if (api_name && api_description)
	  {
	     BUF_APPENDF(I4"api: \"%s\" \"%s\";\n", api_name, api_description);
	     edje_edit_string_free(api_name);
	     edje_edit_string_free(api_description);
	  }
	else
	  if (api_name)
	    {
	       BUF_APPENDF(I4"api: \"%s\" \"\";\n", api_name);
	       edje_edit_string_free(api_name);
	    }
	  else
	    {
	       BUF_APPENDF(I4"api: \"\" \"%s\";\n", api_description);
	       edje_edit_string_free(api_description);
	    }
     }

   BUF_APPEND(I3"}\n");//part
   return ret;
}

static Eina_Bool
_edje_generate_source_of_group(Edje *ed, Edje_Part_Collection_Directory_Entry *pce, Eina_Strbuf *buf)
{
   Evas_Object *obj;
   Eina_List *l, *ll;
   int w, h, i;
   char *data;
   const char *group = pce->entry;
   Eina_Bool ret = EINA_TRUE;

   obj = edje_edit_object_add(ed->evas);
   if (!edje_object_file_set(obj, ed->file->path, group)) return EINA_FALSE;

   BUF_APPENDF(I1"group { name: \"%s\";\n", group);
   //TODO Support alias:
   w = edje_edit_group_min_w_get(obj);
   h = edje_edit_group_min_h_get(obj);
   if ((w > 0) || (h > 0))
      BUF_APPENDF(I2"min: %d %d;\n", w, h);
   w = edje_edit_group_max_w_get(obj);
   h = edje_edit_group_max_h_get(obj);
   if ((w > 0) || (h > 0))
      BUF_APPENDF(I2"max: %d %d;\n", w, h);

   /* Data */
   if (pce->ref->data)
     {
        Eina_Iterator *it;
        Eina_Hash_Tuple *tuple;
        BUF_APPEND(I2"data {\n");

        it = eina_hash_iterator_tuple_new(pce->ref->data);

        if (!it)
          {
             ERR("Generating EDC for Group[%s] data.", group);
             return EINA_FALSE;
          }

        EINA_ITERATOR_FOREACH(it, tuple)
           BUF_APPENDF(I3"item: \"%s\" \"%s\";\n", (char *)tuple->key,
                         (char *)tuple->data);

        eina_iterator_free(it);
        BUF_APPEND(I2"}\n\n");
     }

   //TODO Support script

   /* Parts */
   BUF_APPEND(I2"parts {\n");
   for (i = 0; i < pce->ref->parts_count; i++)
     {
        Edje_Part *ep;
        ep = pce->ref->parts[i];
        ret &= _edje_generate_source_of_part(obj, ep, buf);
     }
   BUF_APPEND(I2"}\n");//parts

   if (!ret)
     {
        ERR("Generating EDC for Group[%s] Parts.", group);
        return EINA_FALSE;
     }

   /* Programs */
   if ((ll = edje_edit_programs_list_get(obj)))
     {
	BUF_APPEND(I2 "programs {\n");
	EINA_LIST_FOREACH(ll, l, data)
	  ret &= _edje_generate_source_of_program(obj, data, buf);
	BUF_APPEND(I2 "}\n");
	edje_edit_string_list_free(ll);
     }
   BUF_APPEND("   }\n");//group

   if (!ret)
     {
        ERR("Generating EDC for Group[%s] Programs.", group);
        evas_object_del(obj);
        return EINA_FALSE;
     }

   evas_object_del(obj);
   return ret;
}

static Eina_Strbuf*
_edje_generate_source(Evas_Object *obj)
{
   Eina_Strbuf *buf;

   Eina_List *l, *ll;
   char *entry;
   Edje_Font_Directory_Entry *fnt;
   Eina_Bool ret = EINA_TRUE;

   GET_ED_OR_RETURN(NULL);

   /* Open a str buffer */

   buf = eina_strbuf_new();
   if (!buf) return NULL;

   /* Write edc into file */
   //TODO Probably we need to save the file before generation

   /* Images */
   if ((ll = edje_edit_images_list_get(obj)))
     {
	BUF_APPEND(I0"images {\n");

	EINA_LIST_FOREACH(ll, l, entry)
	  {
		int comp = edje_edit_image_compression_type_get(obj, entry);
		if (comp < 0) continue;

		BUF_APPENDF(I1"image: \"%s\" ", entry);

		if (comp == EDJE_EDIT_IMAGE_COMP_LOSSY)
		  BUF_APPENDF("LOSSY %d;\n",
		          edje_edit_image_compression_rate_get(obj, entry));
		else if (comp == EDJE_EDIT_IMAGE_COMP_RAW)
		  BUF_APPEND("RAW;\n");
		else if (comp == EDJE_EDIT_IMAGE_COMP_USER)
		  BUF_APPEND("USER;\n");
		else
		  BUF_APPEND("COMP;\n");
	  }
	BUF_APPEND(I0"}\n\n");
	edje_edit_string_list_free(ll);

	if (!ret)
	  {
	     ERR("Generating EDC for Images");
	     eina_strbuf_free(buf);
	     return NULL;
	  }
     }

   /* Fonts */
   if ((ll = edje_edit_fonts_list_get(obj)))
     {
	BUF_APPEND(I0"fonts {\n");

	EINA_LIST_FOREACH(ll, l, fnt)
          BUF_APPENDF(I1"font: \"%s\" \"%s\";\n", fnt->file,
			     fnt->entry);

	BUF_APPEND(I0"}\n\n");
	eina_list_free(ll);

	if (!ret)
	  {
	     ERR("Generating EDC for Fonts");
	     eina_strbuf_free(buf);
	     return NULL;
	  }
     }

   /* Data */
   if ((ll = edje_edit_data_list_get(obj)))
     {
	BUF_APPEND(I0 "data {\n");

	EINA_LIST_FOREACH(ll, l, entry)
          BUF_APPENDF(I1 "item: \"%s\" \"%s\";\n", entry,
			     edje_edit_data_value_get(obj, entry));

	BUF_APPEND(I0 "}\n\n");
	edje_edit_string_list_free(ll);

	if (!ret)
	  {
	     ERR("Generating EDC for Data");
	     eina_strbuf_free(buf);
	     return NULL;
	  }
     }

   /* Color Classes */
   if ((ll = edje_edit_color_classes_list_get(obj)))
     {
	BUF_APPEND(I0 "color_classes {\n");

	EINA_LIST_FOREACH(ll, l, entry)
	  _edje_generate_source_of_colorclass(ed, entry, buf);

	BUF_APPEND(I0 "}\n\n");
	edje_edit_string_list_free(ll);

	if (!ret)
	  {
	     ERR("Generating EDC for Color Classes");
	     eina_strbuf_free(buf);
	     return NULL;
	  }
     }

   /* Styles */
   if ((ll = edje_edit_styles_list_get(obj)))
     {
	BUF_APPEND(I0 "styles {\n");
	EINA_LIST_FOREACH(ll, l, entry)
	  _edje_generate_source_of_style(ed, entry, buf);
	BUF_APPEND(I0 "}\n\n");
	edje_edit_string_list_free(ll);

	if (!ret)
	  {
	     ERR("Generating EDC for Styles");
	     eina_strbuf_free(buf);
	     return NULL;
	  }
     }

   /* Externals */
   if ((ll = edje_edit_externals_list_get(obj)))
     {
        BUF_APPEND(I0 "externals {\n");
        EINA_LIST_FOREACH(ll, l, entry)
           BUF_APPENDF(I1 "external: \"%s\";\n", entry);

        BUF_APPEND(I0 "}\n\n");
        edje_edit_string_list_free(ll);

        if (!ret)
          {
             ERR("Generating EDC for Externals");
             eina_strbuf_free(buf);
             return NULL;
          }
     }

   /* Collections */
   if (ed->file->collection)
     {
        Eina_Iterator *it;
        Edje_Part_Collection_Directory_Entry *pce;
        BUF_APPEND("collections {\n");

        it = eina_hash_iterator_data_new(ed->file->collection);

        if (!it)
          {
             ERR("Generating EDC for Collections");
             eina_strbuf_free(buf);
             return NULL;
          }

        EINA_ITERATOR_FOREACH(it, pce)
          {
             ret &= _edje_generate_source_of_group(ed, pce, buf);
          }

        eina_iterator_free(it);
        BUF_APPEND("}\n\n");
     }

   return buf;
}



/*********************/
/*  SAVING ROUTINES  */
/*********************/
////////////////////////////////////////
typedef struct _SrcFile               SrcFile;
typedef struct _SrcFile_List          SrcFile_List;

struct _SrcFile
{
   char *name;
   const char *file;
};

struct _SrcFile_List
{
   Eina_List *list;
};

static Eet_Data_Descriptor *_srcfile_edd = NULL;
static Eet_Data_Descriptor *_srcfile_list_edd = NULL;

static void
source_edd(void)
{
   Eet_Data_Descriptor_Class eddc;

   if (_srcfile_edd) return;

   eet_eina_stream_data_descriptor_class_set(&eddc, "srcfile", sizeof(SrcFile));
   _srcfile_edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_srcfile_edd, SrcFile, "name", name, EET_T_INLINED_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_srcfile_edd, SrcFile, "file", file, EET_T_INLINED_STRING);

   eet_eina_stream_data_descriptor_class_set(&eddc, "srcfile_list", sizeof(SrcFile_List));
   _srcfile_list_edd = eet_data_descriptor_stream_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_LIST(_srcfile_list_edd, SrcFile_List, "list", list, _srcfile_edd);
}
/////////////////////////////////////////

static Eina_Bool
_edje_edit_edje_file_save(Eet_File *eetf, Edje_File *ef)
{
   /* Write Edje_File structure */
   INF("** Writing Edje_File* ed->file");
   if (eet_data_write(eetf, _edje_edd_edje_file, "edje/file", ef, 1) <= 0)
     {
	ERR("Error. unable to write \"edje_file\" entry to \"%s\"", ef->path);
	return EINA_FALSE;
     }
   return EINA_TRUE;
}

static Eina_Bool
_edje_edit_collection_save(Eet_File *eetf, Edje_Part_Collection *epc)
{
   char buf[256];

   snprintf(buf, sizeof(buf), "edje/collections/%i", epc->id);

   if (eet_data_write(eetf, _edje_edd_edje_part_collection, buf, epc, 1) > 0)
     return EINA_TRUE;

   ERR("Error. unable to write \"%s\" part entry", buf);
   return EINA_FALSE;
}

static Eina_Bool
_edje_edit_source_save(Eet_File *eetf, Evas_Object *obj)
{
   SrcFile *sf;
   SrcFile_List *sfl;
   Eina_Strbuf *source_file;
   Eina_Bool ret = EINA_TRUE;

   source_file = _edje_generate_source(obj);
   if (!source_file)
     {
	ERR("Can't create edc source");
	return EINA_FALSE;
     }

   //open the temp file and put the contents in SrcFile
   sf = _alloc(sizeof(SrcFile));
   if (!sf)
     {
	ERR("Unable to create source file struct");
	ret = EINA_FALSE;
        goto save_free_source;
     }
   sf->name = strdup("generated_source.edc");
   if (!sf->name)
     {
	ERR("Unable to alloc filename");
	ret = EINA_FALSE;
        goto save_free_sf;
     }

   sf->file = eina_strbuf_string_get(source_file);

   //create the needed list of source files (only one)
   sfl = _alloc(sizeof(SrcFile_List));
   if (!sfl)
     {
	ERR("Unable to create file list");
	ret = EINA_FALSE;
        goto save_free_filename;
     }
   sfl->list = NULL;
   sfl->list = eina_list_append(sfl->list, sf);
   if (!sfl->list)
     {
	ERR("Error. unable to append file in list");
	ret = EINA_FALSE;
        goto save_free_sfl;
     }

   // write the sources list to the eet file
   source_edd();
   if (eet_data_write(eetf, _srcfile_list_edd, "edje_sources", sfl, 1) <= 0)
    {
	ERR("Unable to write edc source");
	ret = EINA_FALSE;
    }

   /* Clear stuff */
   eina_list_free(sfl->list);
save_free_sfl:
   free(sfl);
save_free_filename:
   free(sf->name);
save_free_sf:
   free(sf);
save_free_source:
   eina_strbuf_free(source_file);
   return ret;
}

Eina_Bool
_edje_edit_internal_save(Evas_Object *obj, int current_only)
{
   Edje_File *ef;
   Eet_File *eetf;

   GET_ED_OR_RETURN(EINA_FALSE);

   ef = ed->file;
   if (!ef) return EINA_FALSE;

   INF("***********  Saving file ******************");
   INF("** path: %s", ef->path);

   /* Open the eet file */
   eetf = eet_open(ef->path, EET_FILE_MODE_READ_WRITE);
   if (!eetf)
     {
	ERR("Error. unable to open \"%s\" for writing output",
	    ef->path);
	return EINA_FALSE;
     }

   /* Set compiler name */
   if (strcmp(ef->compiler, "edje_edit"))
     {
	_edje_if_string_free(ed, ef->compiler);
	ef->compiler = (char *)eina_stringshare_add("edje_edit");
     }

   if (!_edje_edit_edje_file_save(eetf, ef))
     {
	eet_close(eetf);
	return EINA_FALSE;
     }

   if (current_only)
     {
	if (ed->collection)
	  {
	     INF("** Writing Edje_Part_Collection* ed->collection "
		   "[id: %d]", ed->collection->id);
	     if (!_edje_edit_collection_save(eetf, ed->collection))
	       {
		  eet_close(eetf);
		  return EINA_FALSE;
	       }
	  }
     }
   else
     {
	Eina_List *l;
	Edje_Part_Collection *edc;
	Edje_Part_Collection_Directory_Entry *ce;
        Eina_Iterator *it;

	INF("** Writing all collections");

	it = eina_hash_iterator_data_new(ef->collection);
        while (eina_iterator_next(it, (void **)&ce))
	  {
	     if (ce->ref)
	       {
		  INF("** Writing hash Edje_Part_Collection* ed->collection "
			"[id: %d]", ce->id);
		  if(!_edje_edit_collection_save(eetf, ce->ref))
		    {
		       eet_close(eetf);
		       return EINA_FALSE;
		    }
	       }
	  }
	eina_iterator_free(it);

	EINA_LIST_FOREACH(ef->collection_cache, l, edc)
	  {
	     INF("** Writing cache Edje_Part_Collection* ed->collection "
		   "[id: %d]", edc->id);
	     if(!_edje_edit_collection_save(eetf, edc))
	       {
		  eet_close(eetf);
		  return EINA_FALSE;
	       }
	  }
     }

   if (!_edje_edit_source_save(eetf, obj))
     {
	eet_close(eetf);
	return EINA_FALSE;
     }

   eet_close(eetf);

   /* Update mtime */
   {
     struct stat st;
     if (stat(ed->path, &st) != 0)
       return EINA_FALSE;
     ef->mtime = st.st_mtime;
   }

   INF("***********  Saving DONE ******************");
   return EINA_TRUE;
}

EAPI Eina_Bool
edje_edit_save(Evas_Object *obj)
{
   return _edje_edit_internal_save(obj, 1);
}

EAPI Eina_Bool
edje_edit_save_all(Evas_Object *obj)
{
   return _edje_edit_internal_save(obj, 0);
}

EAPI void
edje_edit_print_internal_status(Evas_Object *obj)
{
   Edje_Program *epr;
   unsigned int i;
   int j;

   GET_ED_OR_RETURN();

   _edje_generate_source(obj);
   return;

   INF("\n****** CHECKIN' INTERNAL STRUCTS STATUS *********");

   INF("*** Edje\n");
   INF("    path: '%s'", ed->path);
   INF("    group: '%s'", ed->group);
   INF("    parent: '%s'", ed->parent);

   INF("*** Parts [table:%d list:%d]", ed->table_parts_size,
       ed->collection->parts_count);
   for (i = 0; i < ed->collection->parts_count; ++i)
     {
	Edje_Real_Part *rp;
	Edje_Part *p;

	p = ed->collection->parts[i];

	rp = ed->table_parts[p->id % ed->table_parts_size];
	printf("    [%d]%s ", p->id, p->name);
	if (p == rp->part)
	  printf(" OK!\n");
	else
	  WRN(" WRONG (table[%id]->name = '%s')", p->id, rp->part->name);
     }

   INF("*** Programs [table:%d list:%d,%d,%d,%d,%d]", ed->table_programs_size,
       ed->collection->programs.fnmatch_count,
       ed->collection->programs.strcmp_count,
       ed->collection->programs.strncmp_count,
       ed->collection->programs.strrncmp_count,
       ed->collection->programs.nocmp_count);
   for(j = 0; j < ed->table_programs_size; ++j)
     {
	epr = ed->table_programs[i % ed->table_programs_size];
	printf("     [%d]%s\n", epr->id, epr->name);
     }

   printf("\n");

   INF("******************  END  ************************\n");
}
