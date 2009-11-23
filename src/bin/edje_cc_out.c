/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
# ifdef  __cplusplus
extern "C"
# endif
void *alloca (size_t);
#endif

#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/stat.h>

#include <Ecore_Evas.h>

#include "edje_cc.h"
#include "edje_prefix.h"

#include <lua.h>
#include <lauxlib.h>

typedef struct _External_Lookup External_Lookup;
typedef struct _Part_Lookup Part_Lookup;
typedef struct _Program_Lookup Program_Lookup;
typedef struct _Group_Lookup Group_Lookup;
typedef struct _String_Lookup Image_Lookup;
typedef struct _String_Lookup Spectrum_Lookup;
typedef struct _Slave_Lookup Slave_Lookup;
typedef struct _Code_Lookup Code_Lookup;


struct _External_Lookup
{
   char *name;
};

struct _Part_Lookup
{
   Edje_Part_Collection *pc;
   char *name;
   int *dest;
};

struct _Program_Lookup
{
   Edje_Part_Collection *pc;
   char *name;
   int *dest;
};

struct _Group_Lookup
{
   char *name;
};

struct _String_Lookup
{
   char *name;
   int *dest;
};

struct _Slave_Lookup
{
   int *master;
   int *slave;
};

struct _Code_Lookup
{
   char *ptr;
   int   len;
   int   val;
};

static void data_process_string(Edje_Part_Collection *pc, const char *prefix, char *s, void (*func)(Edje_Part_Collection *pc, char *name, char *ptr, int len));

Edje_File *edje_file = NULL;
Eina_List *edje_collections = NULL;
Eina_List *externals = NULL;
Eina_List *fonts = NULL;
Eina_List *codes = NULL;
Eina_List *code_lookups = NULL;
Eina_List *aliases = NULL;

static Eet_Data_Descriptor *edd_edje_file = NULL;
static Eet_Data_Descriptor *edd_edje_image_directory = NULL;
static Eet_Data_Descriptor *edd_edje_image_directory_entry = NULL;
static Eet_Data_Descriptor *edd_edje_spectrum_directory = NULL;
static Eet_Data_Descriptor *edd_edje_spectrum_directory_entry = NULL;
static Eet_Data_Descriptor *edd_edje_program = NULL;
static Eet_Data_Descriptor *edd_edje_program_target = NULL;
static Eet_Data_Descriptor *edd_edje_part_collection_directory = NULL;
static Eet_Data_Descriptor *edd_edje_part_collection_directory_entry = NULL;
static Eet_Data_Descriptor *edd_edje_part_collection = NULL;
static Eet_Data_Descriptor *edd_edje_part = NULL;
static Eet_Data_Descriptor *edd_edje_part_description = NULL;
static Eet_Data_Descriptor *edd_edje_part_image_id = NULL;
static Eet_Data_Descriptor *edd_edje_spectrum_color = NULL;

static Eina_List *part_lookups = NULL;
static Eina_List *program_lookups = NULL;
static Eina_List *group_lookups = NULL;
static Eina_List *image_lookups = NULL;
static Eina_List *spectrum_lookups = NULL;
static Eina_List *part_slave_lookups = NULL;
static Eina_List *image_slave_lookups= NULL;
static Eina_List *spectrum_slave_lookups= NULL;

#define ABORT_WRITE(eet_file, file) \
   eet_close(eet_file); \
   unlink(file); \
   exit(-1);

static void
error_and_abort(Eet_File *ef, const char *fmt, ...)
{
   va_list ap;

   fprintf(stderr, "%s: Error. ", progname);

   va_start(ap, fmt);
   vfprintf(stderr, fmt, ap);
   va_end(ap);
   ABORT_WRITE(ef, file_out);
}

void
data_setup(void)
{
   edd_edje_file = _edje_edd_edje_file;
   edd_edje_image_directory = _edje_edd_edje_image_directory;
   edd_edje_image_directory_entry = _edje_edd_edje_image_directory_entry;
   edd_edje_spectrum_directory = _edje_edd_edje_spectrum_directory;
   edd_edje_spectrum_directory_entry = _edje_edd_edje_spectrum_directory_entry;
   edd_edje_program = _edje_edd_edje_program;
   edd_edje_program_target = _edje_edd_edje_program_target;
   edd_edje_part_collection_directory = _edje_edd_edje_part_collection_directory;
   edd_edje_part_collection_directory_entry = _edje_edd_edje_part_collection_directory_entry;
   edd_edje_part_collection = _edje_edd_edje_part_collection;
   edd_edje_part = _edje_edd_edje_part;
   edd_edje_part_description = _edje_edd_edje_part_description;
   edd_edje_part_image_id = _edje_edd_edje_part_image_id;
   edd_edje_spectrum_color = _edje_edd_edje_spectrum_color;
}

static void
check_image_part_desc (Edje_Part_Collection *pc, Edje_Part *ep,
                       Edje_Part_Description *epd, Eet_File *ef)
{
   Eina_List *l;
   Edje_Part_Image_Id *iid;

   return;
   if (epd->image.id == -1)
     error_and_abort(ef, "Collection %i: image attributes missing for "
		     "part \"%s\", description \"%s\" %f\n",
		     pc->id, ep->name, epd->state.name, epd->state.value);

   EINA_LIST_FOREACH(epd->image.tween_list, l, iid)
     {
	if (iid->id == -1)
	  error_and_abort(ef, "Collection %i: tween image id missing for "
			  "part \"%s\", description \"%s\" %f\n",
			  pc->id, ep->name, epd->state.name, epd->state.value);
    }
}

static void
check_packed_items(Edje_Part_Collection *pc, Edje_Part *ep, Eet_File *ef)
{
   Eina_List *l;
   Edje_Pack_Element *it;

   EINA_LIST_FOREACH(ep->items, l, it)
     {
	if (it->type == EDJE_PART_TYPE_GROUP && !it->source)
	  error_and_abort(ef, "Collection %i: missing source on packed item "
			  "of type GROUP in part \"%s\"\n",
			  pc->id, ep->name);
	if (ep->type == EDJE_PART_TYPE_TABLE && (it->col < 0 || it->row < 0))
	  error_and_abort(ef, "Collection %i: missing col/row on packed item "
			  "for part \"%s\" of type TABLE\n",
			  pc->id, ep->name);
     }
}

static void
check_part (Edje_Part_Collection *pc, Edje_Part *ep, Eet_File *ef)
{
   Edje_Part_Description *epd = ep->default_desc;
   Eina_List *l;
   Edje_Part_Description *data;

   if (!epd)
     error_and_abort(ef, "Collection %i: default description missing "
		     "for part \"%s\"\n", pc->id, ep->name);

   if (ep->type == EDJE_PART_TYPE_IMAGE)
     {
	check_image_part_desc (pc, ep, epd, ef);

	EINA_LIST_FOREACH(ep->other_desc, l, data)
	  check_image_part_desc (pc, ep, data, ef);
     }
   else if ((ep->type == EDJE_PART_TYPE_BOX) ||
	    (ep->type == EDJE_PART_TYPE_TABLE))
     check_packed_items(pc, ep, ef);
}

static void
check_program (Edje_Part_Collection *pc, Edje_Program *ep, Eet_File *ef)
{
   switch (ep->action)
     {
      case EDJE_ACTION_TYPE_STATE_SET:
      case EDJE_ACTION_TYPE_ACTION_STOP:
      case EDJE_ACTION_TYPE_DRAG_VAL_SET:
      case EDJE_ACTION_TYPE_DRAG_VAL_STEP:
      case EDJE_ACTION_TYPE_DRAG_VAL_PAGE:
	 if (!ep->targets)
	   error_and_abort(ef, "Collection %i: target missing in program "
			   "\"%s\"\n", pc->id, ep->name);
	 break;
      default:
	 break;
     }
}

static void
check_spectrum (Edje_Spectrum_Directory_Entry *se, Eet_File *ef)
{
   if (!se->entry)
     error_and_abort(ef, "Spectrum missing a name.\n");
   else if (!se->color_list)
     error_and_abort(ef, "Spectrum %s is empty. At least one color must be "
		     "given.", se->entry);
}

static int
data_write_header(Eet_File *ef)
{
   int bytes = 0;

   if (edje_file)
     {

	if (edje_file->collection_dir)
	  {
	     /* copy aliases into collection directory */
	     while (aliases)
	       {
		 edje_file->collection_dir->entries = eina_list_append(edje_file->collection_dir->entries, eina_list_data_get(aliases));
		  aliases = eina_list_remove_list(aliases, aliases);
	       }
	  }
	bytes = eet_data_write(ef, edd_edje_file, "edje_file", edje_file, 1);
	if (bytes <= 0)
	  error_and_abort(ef, "Unable to write \"edje_file\" entry to \"%s\" \n",
			  file_out);
     }

   if (verbose)
     {
	printf("%s: Wrote %9i bytes (%4iKb) for \"edje_file\" header\n",
	       progname, bytes, (bytes + 512) / 1024);
     }

   return bytes;
}

static int
data_write_fonts(Eet_File *ef, int *font_num, int *input_bytes, int *input_raw_bytes)
{
   Eina_List *l;;
   int bytes = 0;
   int total_bytes = 0;
   Font *fn;

   EINA_LIST_FOREACH(fonts, l, fn)
     {
	void *fdata = NULL;
	int fsize = 0;
	Eina_List *ll;
	FILE *f;

	f = fopen(fn->file, "rb");
	if (f)
	  {
	     long pos;

	     fseek(f, 0, SEEK_END);
	     pos = ftell(f);
	     rewind(f);
	     fdata = malloc(pos);
	     if (fdata)
	       {
		  if (fread(fdata, pos, 1, f) != 1)
		    error_and_abort(ef, "Unable to read all of font "
				    "file \"%s\"\n", fn->file);
		  fsize = pos;
	       }
	     fclose(f);
	  }
	else
	  {
	     char *data;

	     EINA_LIST_FOREACH(fnt_dirs, ll, data)
	       {
		  char buf[4096];

		  snprintf(buf, sizeof(buf), "%s/%s", data, fn->file);
		  f = fopen(buf, "rb");
		  if (f)
		    {
		       long pos;

		       fseek(f, 0, SEEK_END);
		       pos = ftell(f);
		       rewind(f);
		       fdata = malloc(pos);
		       if (fdata)
			 {
			    if (fread(fdata, pos, 1, f) != 1)
			      error_and_abort(ef, "Unable to read all of font "
					      "file \"%s\"\n", buf);
			    fsize = pos;
			 }
		       fclose(f);
		       if (fdata) break;
		    }
	       }
	  }
	if (!fdata)
	  {
	     error_and_abort(ef, "Unable to load font part \"%s\" entry "
			     "to %s \n", fn->file, file_out);
	  }
	else
	  {
	     char buf[4096];

	     snprintf(buf, sizeof(buf), "fonts/%s", fn->name);
	     bytes = eet_write(ef, buf, fdata, fsize, 1);
	     if (bytes <= 0)
	       error_and_abort(ef, "Unable to write font part \"%s\" as \"%s\" "
			       "part entry to %s \n", fn->file, buf, file_out);

	     *font_num += 1;
	     total_bytes += bytes;
	     *input_bytes += fsize;
	     *input_raw_bytes += fsize;

	     if (verbose)
	       {
		  printf("%s: Wrote %9i bytes (%4iKb) for \"%s\" font entry \"%s\" compress: [real: %2.1f%%]\n",
			 progname, bytes, (bytes + 512) / 1024, buf, fn->file,
			 100 - (100 * (double)bytes) / ((double)(fsize))
			 );
	       }
	     free(fdata);
	  }
     }

   return total_bytes;
}

static int
data_write_images(Eet_File *ef, int *image_num, int *input_bytes, int *input_raw_bytes)
{
   Eina_List *l;
   int bytes = 0;
   int total_bytes = 0;

   if ((edje_file) && (edje_file->image_dir))
     {
	Ecore_Evas *ee;
	Evas *evas;
	Edje_Image_Directory_Entry *img;

	ecore_init();
	ecore_evas_init();

	ee = ecore_evas_buffer_new(1, 1);
	if (!ee)
	  error_and_abort(ef, "Cannot create buffer engine canvas for image "
			  "load.\n");

	evas = ecore_evas_get(ee);
	EINA_LIST_FOREACH(edje_file->image_dir->entries, l, img)
	  {
	     if (img->source_type == EDJE_IMAGE_SOURCE_TYPE_EXTERNAL)
	       {
	       }
	     else
	       {
		  Evas_Object *im;
		  Eina_List *ll;
		  char *data;

		  im = NULL;
		  EINA_LIST_FOREACH(img_dirs, ll, data)
		    {
		       char buf[4096];

		       snprintf(buf, sizeof(buf), "%s/%s",
				data, img->entry);
		       im = evas_object_image_add(evas);
		       if (im)
			 {
			    evas_object_image_file_set(im, buf, NULL);
			    if (evas_object_image_load_error_get(im) ==
				EVAS_LOAD_ERROR_NONE)
			      {
				 break;
			      }
			    evas_object_del(im);
			    im = NULL;
			 }
		    }
		  if (!im)
		    {
		       im = evas_object_image_add(evas);
		       if (im)
			 {
			    evas_object_image_file_set(im, img->entry, NULL);
			    if (evas_object_image_load_error_get(im) !=
				EVAS_LOAD_ERROR_NONE)
			      {
				 evas_object_del(im);
				 im = NULL;
			      }
			 }
		    }
		  if (im)
		    {
		       void *im_data;
		       int  im_w, im_h;
		       int  im_alpha;
		       char buf[256];

		       evas_object_image_size_get(im, &im_w, &im_h);
		       im_alpha = evas_object_image_alpha_get(im);
		       im_data = evas_object_image_data_get(im, 0);
		       if ((im_data) && (im_w > 0) && (im_h > 0))
			 {
			    int mode, qual;

			    snprintf(buf, sizeof(buf), "images/%i", img->id);
			    mode = 2;
			    qual = 80;
			    if ((img->source_type == EDJE_IMAGE_SOURCE_TYPE_INLINE_PERFECT) &&
				(img->source_param == 0))
			      mode = 0; /* RAW */
			    else if ((img->source_type == EDJE_IMAGE_SOURCE_TYPE_INLINE_PERFECT) &&
				     (img->source_param == 1))
			      mode = 1; /* COMPRESS */
			    else
			      mode = 2; /* LOSSY */
			    if ((mode == 0) && (no_raw))
			      {
				 mode = 1; /* promote compression */
				 img->source_param = 95;
			      }
			    if ((mode == 2) && (no_lossy)) mode = 1; /* demote compression */
			    if ((mode == 1) && (no_comp))
			      {
				 if (no_lossy) mode = 0; /* demote compression */
				 else if (no_raw)
				   {
				      img->source_param = 90;
				      mode = 2; /* no choice. lossy */
				   }
			      }
			    if (mode == 2)
			      {
				 qual = img->source_param;
				 if (qual < min_quality) qual = min_quality;
				 if (qual > max_quality) qual = max_quality;
			      }
			    if (mode == 0)
			      bytes = eet_data_image_write(ef, buf,
							   im_data, im_w, im_h,
							   im_alpha,
							   0, 0, 0);
			    else if (mode == 1)
			      bytes = eet_data_image_write(ef, buf,
							   im_data, im_w, im_h,
							   im_alpha,
							   1, 0, 0);
			    else if (mode == 2)
			      bytes = eet_data_image_write(ef, buf,
							   im_data, im_w, im_h,
							   im_alpha,
							   0, qual, 1);
			    if (bytes <= 0)
			      error_and_abort(ef, "Unable to write image part "
					      "\"%s\" as \"%s\" part entry to "
					      "%s\n", img->entry, buf,
					      file_out);

			    *image_num += 1;
			    total_bytes += bytes;
			 }
		       else
			 {
			    error_and_abort(ef, "Unable to load image for "
					    "image part \"%s\" as \"%s\" part "
					    "entry to %s\n", img->entry, buf,
					    file_out);
			 }

		       if (verbose)
			 {
			    struct stat st;
			    const char *file = NULL;

			    evas_object_image_file_get(im, &file, NULL);
			    if (!file || (stat(file, &st) != 0))
			      st.st_size = 0;
			    *input_bytes += st.st_size;
			    *input_raw_bytes += im_w * im_h * 4;
			    printf("%s: Wrote %9i bytes (%4iKb) for \"%s\" image entry \"%s\" compress: [raw: %2.1f%%] [real: %2.1f%%]\n",
				   progname, bytes, (bytes + 512) / 1024, buf, img->entry,
				   100 - (100 * (double)bytes) / ((double)(im_w * im_h * 4)),
				   100 - (100 * (double)bytes) / ((double)(st.st_size))
				   );
			 }
		       evas_object_del(im);
		    }
		  else
		    {
		       error_and_abort(ef, "Unable to load image for image \"%s\" part entry to %s. Missing PNG or JPEG loader modules for Evas or file does not exist, or is not readable.\n",
				       img->entry, file_out);
		    }
	       }
	  }
	ecore_evas_free(ee);
	ecore_evas_shutdown();
	ecore_shutdown();
     }

   return total_bytes;
}

static void
check_groups_names(Eet_File *ef)
{
   Eina_List *l;
   Edje_Part_Collection_Directory_Entry *de;

   if (!edje_file->collection_dir)
     return;

   /* check that all groups have names */
   EINA_LIST_FOREACH(edje_file->collection_dir->entries, l, de)
	if (!de->entry)
	  error_and_abort(ef, "Collection %i: name missing.\n", de->id);
}

static void
check_spectra(Eet_File *ef)
{
   Eina_List *l;
   Edje_Spectrum_Directory_Entry *se;

   if (!edje_file->spectrum_dir)
     return;

   /* check that all spectra are valid */
   EINA_LIST_FOREACH(edje_file->spectrum_dir->entries, l, se)
     check_spectrum(se, ef);
}

static void
check_groups(Eet_File *ef)
{
   Eina_List *l;
   Edje_Part_Collection *pc;

   /* sanity checks for parts and programs */
   EINA_LIST_FOREACH(edje_collections, l, pc)
     {
	Eina_List *ll;
	Edje_Part *part;
	Edje_Program *prog;

	EINA_LIST_FOREACH(pc->parts, ll, part)
	  check_part(pc, part, ef);
	EINA_LIST_FOREACH(pc->programs, ll, prog)
	  check_program(pc, prog, ef);
     }
}

static int
data_write_groups(Eet_File *ef, int *collection_num)
{
   Eina_List *l;
   Edje_Part_Collection *pc;
   int bytes = 0;
   int total_bytes = 0;

   EINA_LIST_FOREACH(edje_collections, l, pc)
     {
	char buf[4096];

	snprintf(buf, sizeof(buf), "collections/%i", pc->id);
	bytes = eet_data_write(ef, edd_edje_part_collection, buf, pc, 1);
	if (bytes <= 0)
	  error_and_abort(ef, "Error. Unable to write \"%s\" part entry "
			  "to %s\n", buf, file_out);

	*collection_num += 1;
	total_bytes += bytes;

	if (verbose)
	  {
	     printf("%s: Wrote %9i bytes (%4iKb) for \"%s\" collection entry\n",
		    progname, bytes, (bytes + 512) / 1024, buf);
	  }
     }

   return total_bytes;
}

static void
create_script_file(Eet_File *ef, const char *filename, const Code *cd)
{
   FILE *f = fopen(filename, "wb");
   if (!f)
     error_and_abort(ef, "Unable to open temp file \"%s\" for script "
		     "compilation.\n", filename);

   Eina_List *ll;
   Code_Program *cp;

   fprintf(f, "#include <edje>\n");
   int ln = 2;

   if (cd->shared)
     {
	while (ln < (cd->l1 - 1))
	  {
	     fprintf(f, " \n");
	     ln++;
	  }
	{
	   char *sp;
	   int hash = 0;
	   int newlined = 0;

	   for (sp = cd->shared; *sp; sp++)
	     {
		if ((sp[0] == '#') && (newlined))
		  {
		     hash = 1;
		  }
		newlined = 0;
		if (sp[0] == '\n') newlined = 1;
		if (!hash) fputc(sp[0], f);
		else if (sp[0] == '\n') hash = 0;
	     }
	   fputc('\n', f);
	}
	ln += cd->l2 - cd->l1 + 1;
     }
   EINA_LIST_FOREACH(cd->programs, ll, cp)
     {
	if (cp->script)
	  {
	     while (ln < (cp->l1 - 1))
	       {
		  fprintf(f, " \n");
		  ln++;
	       }
	     /* FIXME: this prototype needs to be */
	     /* formalised and set in stone */
	     fprintf(f, "public _p%i(sig[], src[]) {", cp->id);
	     {
		char *sp;
		int hash = 0;
		int newlined = 0;

		for (sp = cp->script; *sp; sp++)
		  {
		     if ((sp[0] == '#') && (newlined))
		       {
			  hash = 1;
		       }
		     newlined = 0;
		     if (sp[0] == '\n') newlined = 1;
		     if (!hash) fputc(sp[0], f);
		     else if (sp[0] == '\n') hash = 0;
		  }
	     }
	     fprintf(f, "}");
	     ln += cp->l2 - cp->l1 + 1;
	  }
     }

   fclose(f);
}

static void
compile_script_file(Eet_File *ef, const char *source, const char *output,
		    int script_num)
{
   FILE *f;
   char buf[4096];
   int ret;

   snprintf(buf, sizeof(buf),
	    "embryo_cc -i %s/include -o %s %s",
	    e_prefix_data_get(), output, source);
   ret = system(buf);

   /* accept warnings in the embryo code */
   if (ret < 0 || ret > 1)
     error_and_abort(ef, "Compiling script code not clean.\n");

   f = fopen(output, "rb");
   if (!f)
     error_and_abort(ef, "Unable to open script object \"%s\" for reading.\n",
		     output);

   fseek(f, 0, SEEK_END);
   int size = ftell(f);
   rewind(f);

   if (size > 0)
     {
	int bt;
	void *data = malloc(size);

	if (data)
	  {
	     if (fread(data, size, 1, f) != 1)
	       error_and_abort(ef, "Unable to read all of script object "
			       "\"%s\"\n", output);

	     snprintf(buf, sizeof(buf), "scripts/%i", script_num);
	     bt = eet_write(ef, buf, data, size, 1);
	     free(data);
	  }
     }

   fclose(f);
}

static void
data_write_scripts(Eet_File *ef)
{
   Eina_List *l;
   int i;

   if (!tmp_dir)
#ifdef HAVE_EVIL
     tmp_dir = (char *)evil_tmpdir_get();
#else
     tmp_dir = "/tmp";
#endif

   for (i = 0, l = codes; l; l = eina_list_next(l), i++)
     {
	char tmpn[4096];
	char tmpo[4096];
	int fd;
	Code *cd = eina_list_data_get(l);
	
	if (cd->is_lua)
	  continue;
	if ((!cd->shared) && (!cd->programs))
	  continue;

	snprintf(tmpn, PATH_MAX, "%s/edje_cc.sma-tmp-XXXXXX", tmp_dir);
	fd = mkstemp(tmpn);
	if (fd < 0)
	  error_and_abort(ef, "Unable to open temp file \"%s\" for script "
			  "compilation.\n", tmpn);

	create_script_file(ef, tmpn, cd);
	close(fd);

	snprintf(tmpo, PATH_MAX, "%s/edje_cc.amx-tmp-XXXXXX", tmp_dir);
	fd = mkstemp(tmpo);
	if (fd < 0)
	  {
	     unlink(tmpn);
	     error_and_abort(ef, "Unable to open temp file \"%s\" for script "
			     "compilation.\n", tmpn);
	  }

	compile_script_file(ef, tmpn, tmpo, i);
	close(fd);

	unlink(tmpn);
	unlink(tmpo);
     }
}

typedef struct _Edje_Lua_Script_Writer_Struct Edje_Lua_Script_Writer_Struct;

struct _Edje_Lua_Script_Writer_Struct {
   char *buf;
   int size;
};

#ifdef LUA_BINARY
static int
_edje_lua_script_writer (lua_State *L __UNUSED__, const void* chunk_buf, size_t chunk_size, void* _data)
{
   Edje_Lua_Script_Writer_Struct *data;
   void *old;

   data = (Edje_Lua_Script_Writer_Struct *)_data;
   old = data->buf;
   data->buf = malloc (data->size + chunk_size);
   memcpy (data->buf, old, data->size);
   memcpy (&((data->buf)[data->size]), chunk_buf, chunk_size);
   if (old)
     free (old);
   data->size += chunk_size;

   return 0;
}
#endif

void
_edje_lua_error_and_abort(lua_State * L, int err_code, Eet_File *ef)
{
   char *err_type;

   switch (err_code)
     {
     case LUA_ERRRUN:
	err_type = "runtime";
	break;
     case LUA_ERRSYNTAX:
	err_type = "syntax";
	break;
     case LUA_ERRMEM:
	err_type = "memory allocation";
	break;
     case LUA_ERRERR:
	err_type = "error handler";
	break;
     default:
	err_type = "unknown";
	break;
     }
   error_and_abort(ef, "Lua %s error: %s\n", err_type, lua_tostring(L, -1));
}


static void
data_write_lua_scripts(Eet_File *ef)
{
   Eina_List *l;
   Eina_List *ll;
   Code_Program *cp;
   int i;

   for (i = 0, l = codes; l; l = eina_list_next(l), i++)
     {
	char buf[4096];
	Code *cd;
	lua_State *L;
	int ln = 1;
	luaL_Buffer b;
	Edje_Lua_Script_Writer_Struct data;
#ifdef LUA_BINARY
	int err_code;
#endif

	cd = (Code *)eina_list_data_get(l);
	if (!cd->is_lua)
	  continue;
	if ((!cd->shared) && (!cd->programs))
	  continue;
	
	L = luaL_newstate();
	if (!L)
	  error_and_abort(ef, "Lua error: Lua state could not be initialized\n");

	luaL_buffinit(L, &b);

	data.buf = NULL;
	data.size = 0;
	if (cd->shared)
	  {
	     while (ln < (cd->l1 - 1))
	       {
		  luaL_addchar(&b, '\n');
		  ln++;
	       }
	     luaL_addstring(&b, cd->shared);
	     ln += cd->l2 - cd->l1;
	  }

	EINA_LIST_FOREACH(cd->programs, ll, cp)
	  {
	     if (cp->script)
	       {
		  while (ln < (cp->l1 - 1))
		    {
		       luaL_addchar(&b, '\n');
		       ln++;
		    }
		  luaL_addstring(&b, "_G[");
		  lua_pushnumber(L, cp->id);
		  luaL_addvalue(&b);
		  luaL_addstring(&b, "] = function (ed, signal, source)");
		  luaL_addstring(&b, cp->script);
		  luaL_addstring(&b, "end\n");
		  ln += cp->l2 - cp->l1 + 1;
	       }
	  }
	luaL_pushresult(&b);
#ifdef LUA_BINARY
	if (err_code = luaL_loadstring(L, lua_tostring (L, -1)))
	  _edje_lua_error_and_abort(L, err_code, ef);
	lua_dump(L, _edje_lua_script_writer, &data);
#else // LUA_PLAIN_TEXT
	data.buf = lua_tostring(L, -1);
	data.size = strlen(data.buf);
#endif
	//printf("lua chunk size: %d\n", data.size);

	/* 
	 * TODO load and test Lua chunk
	 */

	/*
	   if (luaL_loadbuffer(L, globbuf, globbufsize, "edje_lua_script"))
	   printf("lua load error: %s\n", lua_tostring (L, -1));
	   if (lua_pcall(L, 0, 0, 0))
	   printf("lua call error: %s\n", lua_tostring (L, -1));
	 */
	
	snprintf(buf, sizeof(buf), "lua_scripts/%i", i);
	eet_write(ef, buf, data.buf, data.size, 1);
#ifdef LUA_BINARY
	free(data.buf);
#endif
	lua_close(L);
     }
}

void
data_write(void)
{
   Eet_File *ef;
   int input_bytes = 0;
   int total_bytes = 0;
   int src_bytes = 0;
   int fmap_bytes = 0;
   int input_raw_bytes = 0;
   int image_num = 0;
   int font_num = 0;
   int collection_num = 0;

   ef = eet_open(file_out, EET_FILE_MODE_WRITE);
   if (!ef)
     {
	fprintf(stderr, "%s: Error. Unable to open \"%s\" for writing output\n",
		progname, file_out);
	exit(-1);
     }

   total_bytes += data_write_header(ef);
   total_bytes += data_write_fonts(ef, &font_num, &input_bytes,
				   &input_raw_bytes);
   total_bytes += data_write_images(ef, &image_num, &input_bytes,
				    &input_raw_bytes);

   check_groups_names(ef);
   check_spectra(ef);
   check_groups(ef);

   total_bytes += data_write_groups(ef, &collection_num);
   data_write_scripts(ef);
   data_write_lua_scripts(ef);

   src_bytes = source_append(ef);
   total_bytes += src_bytes;
   fmap_bytes = source_fontmap_save(ef, fonts);
   total_bytes += fmap_bytes;

   eet_close(ef);

   if (verbose)
     {
	struct stat st;

	if (stat(file_in, &st) != 0)
	  st.st_size = 0;
	input_bytes += st.st_size;
	input_raw_bytes += st.st_size;
	printf("Summary:\n"
	       "  Wrote %i collections\n"
	       "  Wrote %i images\n"
	       "  Wrote %i fonts\n"
	       "  Wrote %i bytes (%iKb) of original source data\n"
	       "  Wrote %i bytes (%iKb) of original source font map\n"
	       "Conservative compression summary:\n"
	       "  Wrote total %i bytes (%iKb) from %i (%iKb) input data\n"
	       "  Output file is %3.1f%% the size of the input data\n"
	       "  Saved %i bytes (%iKb)\n"
	       "Raw compression summary:\n"
	       "  Wrote total %i bytes (%iKb) from %i (%iKb) raw input data\n"
	       "  Output file is %3.1f%% the size of the raw input data\n"
	       "  Saved %i bytes (%iKb)\n"
	       ,
	       collection_num,
	       image_num,
	       font_num,
	       src_bytes, (src_bytes + 512) / 1024,
	       fmap_bytes, (fmap_bytes + 512) / 1024,
	       total_bytes, (total_bytes + 512) / 1024,
	       input_bytes, (input_bytes + 512) / 1024,
	       (100.0 * (double)total_bytes) / (double)input_bytes,
	       input_bytes - total_bytes,
	       (input_bytes - total_bytes + 512) / 1024,
	       total_bytes, (total_bytes + 512) / 1024,
	       input_raw_bytes, (input_raw_bytes + 512) / 1024,
	       (100.0 * (double)total_bytes) / (double)input_raw_bytes,
	       input_raw_bytes - total_bytes,
	       (input_raw_bytes - total_bytes + 512) / 1024);
     }
}

void
data_queue_group_lookup(char *name)
{
   Group_Lookup *gl;

   gl = mem_alloc(SZ(Group_Lookup));
   group_lookups = eina_list_append(group_lookups, gl);
   gl->name = mem_strdup(name);
}

void
data_queue_part_lookup(Edje_Part_Collection *pc, char *name, int *dest)
{
   Part_Lookup *pl;

   pl = mem_alloc(SZ(Part_Lookup));
   part_lookups = eina_list_append(part_lookups, pl);
   pl->pc = pc;
   pl->name = mem_strdup(name);
   pl->dest = dest;
}

void
data_queue_program_lookup(Edje_Part_Collection *pc, char *name, int *dest)
{
   Program_Lookup *pl;

   pl = mem_alloc(SZ(Program_Lookup));
   program_lookups = eina_list_append(program_lookups, pl);
   pl->pc = pc;
   pl->name = mem_strdup(name);
   pl->dest = dest;
}

void
data_queue_image_lookup(char *name, int *dest)
{
   Image_Lookup *il;

   il = mem_alloc(SZ(Image_Lookup));
   image_lookups = eina_list_append(image_lookups, il);
   il->name = mem_strdup(name);
   il->dest = dest;
}

void
data_queue_spectrum_lookup(char *name, int *dest)
{
   Spectrum_Lookup *sl;

   sl = mem_alloc(SZ(Spectrum_Lookup));
   spectrum_lookups = eina_list_append(spectrum_lookups, sl);
   sl->name = mem_strdup(name);
   sl->dest = dest;
}

void
data_queue_part_slave_lookup(int *master, int *slave)
{
   Slave_Lookup *sl;

   sl = mem_alloc(SZ(Slave_Lookup));
   part_slave_lookups = eina_list_append(part_slave_lookups, sl);
   sl->master = master;
   sl->slave = slave;
}

void
data_queue_image_slave_lookup(int *master, int *slave)
{
   Slave_Lookup *sl;

   sl = mem_alloc(SZ(Slave_Lookup));
   image_slave_lookups = eina_list_append(image_slave_lookups, sl);
   sl->master = master;
   sl->slave = slave;
}

void
data_queue_spectrum_slave_lookup(int *master, int *slave)
{
   Slave_Lookup *sl;

   sl = mem_alloc(SZ(Slave_Lookup));
   spectrum_slave_lookups = eina_list_append(spectrum_slave_lookups, sl);
   sl->master = master;
   sl->slave = slave;
}

void
handle_slave_lookup(Eina_List *list, int *master, int value)
{
   Eina_List *l;
   Slave_Lookup *sl;

   EINA_LIST_FOREACH(list, l, sl)
     if (sl->master == master)
       *sl->slave = value;
}

void
data_process_lookups(void)
{
   Eina_List *l;

   while (part_lookups)
     {
	Part_Lookup *pl;
	Edje_Part *ep;

	pl = eina_list_data_get(part_lookups);

	EINA_LIST_FOREACH(pl->pc->parts, l, ep)
	  {
	     if ((ep->name) && (!strcmp(ep->name, pl->name)))
	       {
		  handle_slave_lookup(part_slave_lookups, pl->dest, ep->id);
		  *(pl->dest) = ep->id;
		  break;
	       }
	  }
	if (!l)
	  {
	     fprintf(stderr, "%s: Error. Unable to find part name \"%s\".\n",
		     progname, pl->name);
	     exit(-1);
	  }
	part_lookups = eina_list_remove(part_lookups, pl);
	free(pl->name);
	free(pl);
     }

   while (program_lookups)
     {
	Program_Lookup *pl;
	Edje_Program *ep;

	pl = eina_list_data_get(program_lookups);

	EINA_LIST_FOREACH(pl->pc->programs, l, ep)
	  {
	     if ((ep->name) && (!strcmp(ep->name, pl->name)))
	       {
		  *(pl->dest) = ep->id;
		  break;
	       }
	  }
	if (!l)
	  {
	     fprintf(stderr, "%s: Error. Unable to find program name \"%s\".\n",
		     progname, pl->name);
	     exit(-1);
	  }
	program_lookups = eina_list_remove(program_lookups, pl);
	free(pl->name);
	free(pl);
     }

   while (group_lookups)
     {
        Group_Lookup *gl;
	Edje_Part_Collection_Directory_Entry *de;

        gl = eina_list_data_get(group_lookups);

	EINA_LIST_FOREACH(edje_file->collection_dir->entries, l, de)
          {
             if (!strcmp(de->entry, gl->name))
               {
                  break;
               }
          }
        if (!l)
          {
             fprintf(stderr, "%s: Error. Unable to find group name \"%s\".\n",
                     progname, gl->name);
             exit(-1);
          }
        group_lookups = eina_list_remove(group_lookups, gl);
        free(gl->name);
        free(gl);
     }

   while (image_lookups)
     {
	Image_Lookup *il;
	Edje_Image_Directory_Entry *de;

	il = eina_list_data_get(image_lookups);

	if (!edje_file->image_dir)
	  l = NULL;
	else
	  {
	     EINA_LIST_FOREACH(edje_file->image_dir->entries, l, de)
	       {
		  if ((de->entry) && (!strcmp(de->entry, il->name)))
		    {
		       handle_slave_lookup(image_slave_lookups, il->dest, de->id);
		       if (de->source_type == EDJE_IMAGE_SOURCE_TYPE_EXTERNAL)
			 *(il->dest) = -de->id - 1;
		       else
			 *(il->dest) = de->id;
		       break;
		    }
	       }
	  }

	if (!l)
	  {
	     fprintf(stderr, "%s: Error. Unable to find image name \"%s\".\n",
		     progname, il->name);
	     exit(-1);
	  }
	image_lookups = eina_list_remove(image_lookups, il);
	free(il->name);
	free(il);
     }

   while (spectrum_lookups)
     {
	Spectrum_Lookup *il;
	Edje_Spectrum_Directory_Entry *de;

	il = eina_list_data_get(spectrum_lookups);

	if (!edje_file->spectrum_dir)
	  l = NULL;
	else
	  {
	     EINA_LIST_FOREACH(edje_file->spectrum_dir->entries, l, de)
	       {
		  *(il->dest) = 1;
		  if ((de->entry) && (!strcmp(de->entry, il->name)))
		    {
		       handle_slave_lookup(spectrum_slave_lookups, il->dest, de->id);
		       *(il->dest) = de->id;
		       break;
		    }
	       }
	  }

	if (!l)
	  {
	     fprintf(stderr, "%s: Error. unable to find spectrum name %s\n",
		     progname, il->name);
	     exit(-1);
	  }
	spectrum_lookups = eina_list_remove(spectrum_lookups, il);
	free(il->name);
	free(il);
     }

   while (part_slave_lookups)
     {
        free(eina_list_data_get(part_slave_lookups));
	part_slave_lookups = eina_list_remove_list(part_slave_lookups, part_slave_lookups);
     }

   while (image_slave_lookups)
     {
        free(eina_list_data_get(image_slave_lookups));
	image_slave_lookups = eina_list_remove_list(image_slave_lookups, image_slave_lookups);
     }

   while (spectrum_slave_lookups)
     {
        free(eina_list_data_get(spectrum_slave_lookups));
	spectrum_slave_lookups = eina_list_remove_list(spectrum_slave_lookups, spectrum_slave_lookups);
     }
}

static void
data_process_string(Edje_Part_Collection *pc, const char *prefix, char *s, void (*func)(Edje_Part_Collection *pc, char *name, char* ptr, int len))
{
   char *p;
   char *key;
   int keyl;
   int quote, escape;

   key = alloca(strlen(prefix) + 2 + 1);
   if (!key) return;
   strcpy(key, prefix);
   strcat(key, ":\"");
   keyl = strlen(key);
   quote = 0;
   escape = 0;
   for (p = s; (p) && (*p); p++)
     {
	if (!quote)
	  {
	     if (*p == '\"')
	       {
		  quote = 1;
		  p++;
	       }
	  }
	if (!quote)
	  {
	     if (!strncmp(p, key, keyl))
	       {
                  char *ptr;
                  int len;
                  int inesc = 0;
		  char *name;

                  ptr = p;
                  p += keyl;
		  while ((*p))
		    {
		       if (!inesc)
		         {
		  	    if (*p == '\\') inesc = 1;
			    else if (*p == '\"')
			      {
			         /* string concatenation, see below */
				 if (*(p + 1) != '\"')
				   break;
				 else
				   p++;
			      }
			 }
                       else
                            inesc = 0;
                       p++;
		    }
		  len = p - ptr + 1;
		  name = alloca(len);
		  if (name)
		    {
		       char *pp;
		       int i;

		       name[0] = 0;
		       pp = ptr + keyl;
		       inesc = 0;
		       i = 0;
		       while (*pp)
		         {
		    	    if (!inesc)
			      {
			         if (*pp == '\\') inesc = 1;
			         else if (*pp == '\"')
			    	   {
				      /* concat strings like "foo""bar" to "foobar" */
				      if (*(pp + 1) == '\"')
				        pp++;
				      else
				        {
				   	   name[i] = 0;
					   break;
					}
				   }
				 else
				   {
				      name[i] = *pp;
				      name[i + 1] = 0;
				      i++;
				   }
			      }
			    else
                              inesc = 0;
			    pp++;
			}
		      func(pc, name, ptr, len);
		   }
              }
	  }
	else
	  {
	     if (!escape)
	       {
		  if (*p == '\"') quote = 0;
		  else if (*p == '\\') escape = 1;
	       }
	     else if (escape)
	       {
		  escape = 0;
	       }
	  }
     }
}

static void
_data_queue_part_lookup(Edje_Part_Collection *pc, char *name, char *ptr, int len)
{
   Code_Lookup *cl;
   cl = mem_alloc(SZ(Code_Lookup));
   cl->ptr = ptr;
   cl->len = len;

   data_queue_part_lookup(pc, name, &(cl->val));

   code_lookups = eina_list_append(code_lookups, cl);
}
static void
_data_queue_program_lookup(Edje_Part_Collection *pc, char *name, char *ptr, int len)
{
   Code_Lookup *cl;

   cl = mem_alloc(SZ(Code_Lookup));
   cl->ptr = ptr;
   cl->len = len;

   data_queue_program_lookup(pc, name, &(cl->val));

   code_lookups = eina_list_append(code_lookups, cl);
}
static void
_data_queue_group_lookup(Edje_Part_Collection *pc __UNUSED__, char *name, char *ptr __UNUSED__, int len __UNUSED__)
{
   data_queue_group_lookup(name);	
}
static void
_data_queue_image_pc_lookup(Edje_Part_Collection *pc __UNUSED__, char *name, char *ptr, int len)
{
   Code_Lookup *cl;

   cl = mem_alloc(SZ(Code_Lookup));
   cl->ptr = ptr;
   cl->len = len;

   data_queue_image_lookup(name, &(cl->val));

   code_lookups = eina_list_append(code_lookups, cl);
}

void
data_process_scripts(void)
{
   Eina_List *l, *l2;

   for (l = codes, l2 = edje_collections; (l) && (l2); l = eina_list_next(l), l2 = eina_list_next(l2))
     {
	Code *cd;
	Edje_Part_Collection *pc;

	cd = eina_list_data_get(l);
	pc = eina_list_data_get(l2);
	if ((cd->shared) || (cd->programs))
	  {
	     Eina_List *ll;
	     Code_Program *cp;

	     if ((cd->shared) && (!cd->is_lua))
	       {
		  data_process_string(pc, "PART",    cd->shared, _data_queue_part_lookup);
		  data_process_string(pc, "PROGRAM", cd->shared, _data_queue_program_lookup);
		  data_process_string(pc, "IMAGE",   cd->shared, _data_queue_image_pc_lookup);
		  data_process_string(pc, "GROUP",   cd->shared, _data_queue_group_lookup);
	       }
	     EINA_LIST_FOREACH(cd->programs, ll, cp)
	       {
		  if (cp->script)
		    {
		       data_process_string(pc, "PART",    cp->script, _data_queue_part_lookup);
		       data_process_string(pc, "PROGRAM", cp->script, _data_queue_program_lookup);
		       data_process_string(pc, "IMAGE",   cp->script, _data_queue_image_pc_lookup);
		       data_process_string(pc, "GROUP",   cp->script, _data_queue_group_lookup);
		    }
	       }
	  }
     }
}

void
data_process_script_lookups(void)
{
   Eina_List *l;
   Code_Lookup *cl;

   EINA_LIST_FOREACH(code_lookups, l, cl)
     {
	char buf[12];
	int n;

	n = eina_convert_itoa(cl->val, buf);
	if (n > cl->len)
	  {
	     fprintf(stderr, "%s: Error. The unexpected happened. A numeric replacement string was larger than the original!\n",
		     progname);
	     exit(-1);
	  }
	memset(cl->ptr, ' ', cl->len);
	strncpy(cl->ptr, buf, n);
     }
}
