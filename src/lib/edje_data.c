#include "edje_private.h"

EAPI Eet_Data_Descriptor *_edje_edd_edje_file = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_style = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_style_tag = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_color_class = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_external_directory = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_external_directory_entry = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_font_directory_entry = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_image_directory = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_image_directory_entry = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_image_directory_set = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_image_directory_set_entry = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_program = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_program_pointer = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_program_target = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_program_after = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_collection_directory_entry = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_pack_element = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_collection = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_pointer = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_variant = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_common = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_image = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_text = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_box = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_table = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_external = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_variant_list = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_common_pointer = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_image_pointer = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_text_pointer = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_box_pointer = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_table_pointer = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_description_external_pointer = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_image_id = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_part_image_id_pointer = NULL;
EAPI Eet_Data_Descriptor *_edje_edd_edje_external_param = NULL;

#define FREED(eed) \
   if (eed) \
   { \
      eet_data_descriptor_free((eed)); \
      (eed) = NULL; \
   }

struct {
   Edje_Part_Type type;
   const char *name;
} variant_convertion[] = {
  { EDJE_PART_TYPE_RECTANGLE, "rectangle" },
  { EDJE_PART_TYPE_SWALLOW,   "swallow" },
  { EDJE_PART_TYPE_GROUP,     "group" },
  { EDJE_PART_TYPE_IMAGE,     "image" },
  { EDJE_PART_TYPE_TEXT,      "text" },
  { EDJE_PART_TYPE_TEXTBLOCK, "textblock" },
  { EDJE_PART_TYPE_BOX,       "box" },
  { EDJE_PART_TYPE_TABLE,     "table" },
  { EDJE_PART_TYPE_EXTERNAL,  "external" },
};

static const char *
_edje_description_variant_type_get(const void *data, Eina_Bool *unknow __UNUSED__)
{
   const unsigned char *type;
   unsigned int i;

   type = data;

   for (i = 0; i < (sizeof (variant_convertion) / sizeof (variant_convertion[0])); ++i)
     if (*type == variant_convertion[i].type)
       return variant_convertion[i].name;

   return NULL;
}

static Eina_Bool
_edje_description_variant_type_set(const char *type, void *data, Eina_Bool unknow __UNUSED__)
{
   unsigned char *dt;
   unsigned int i;

   dt = data;

   for (i = 0; i < (sizeof (variant_convertion) / sizeof (variant_convertion[0])); ++i)
     if (!strcmp(variant_convertion[i].name, type))
       {
	  *dt = variant_convertion[i].type;
	  return EINA_TRUE;
       }

   return EINA_FALSE;
}

void
_edje_edd_shutdown(void)
{
   FREED(_edje_edd_edje_file);
   FREED(_edje_edd_edje_style);
   FREED(_edje_edd_edje_style_tag);
   FREED(_edje_edd_edje_color_class);
   FREED(_edje_edd_edje_external_directory);
   FREED(_edje_edd_edje_external_directory_entry);
   FREED(_edje_edd_edje_font_directory_entry);
   FREED(_edje_edd_edje_image_directory);
   FREED(_edje_edd_edje_image_directory_entry);
   FREED(_edje_edd_edje_program);
   FREED(_edje_edd_edje_program_pointer);
   FREED(_edje_edd_edje_program_target);
   FREED(_edje_edd_edje_program_after);
   FREED(_edje_edd_edje_part_collection_directory_entry);
   FREED(_edje_edd_edje_pack_element);
   FREED(_edje_edd_edje_part_collection);
   FREED(_edje_edd_edje_part);
   FREED(_edje_edd_edje_part_pointer);
   FREED(_edje_edd_edje_part_description_variant);
   FREED(_edje_edd_edje_part_description_common);
   FREED(_edje_edd_edje_part_description_image);
   FREED(_edje_edd_edje_part_description_text);
   FREED(_edje_edd_edje_part_description_box);
   FREED(_edje_edd_edje_part_description_table);
   FREED(_edje_edd_edje_part_description_external);
   FREED(_edje_edd_edje_part_description_variant_list);
   FREED(_edje_edd_edje_part_description_common_pointer);
   FREED(_edje_edd_edje_part_description_image_pointer);
   FREED(_edje_edd_edje_part_description_text_pointer);
   FREED(_edje_edd_edje_part_description_box_pointer);
   FREED(_edje_edd_edje_part_description_table_pointer);
   FREED(_edje_edd_edje_part_description_external_pointer);
   FREED(_edje_edd_edje_part_image_id);
   FREED(_edje_edd_edje_part_image_id_pointer);
   FREED(_edje_edd_edje_external_param);
   FREED(_edje_edd_edje_image_directory_set);
   FREED(_edje_edd_edje_image_directory_set_entry);
}

#define EDJE_DEFINE_POINTER_TYPE(Type, Name)				\
   {									\
     typedef struct _Edje_##Type##_Pointer Edje_##Type##_Pointer;	\
     struct _Edje_##Type##_Pointer					\
     {									\
	Edje_##Type *pointer;						\
     };									\
									\
     EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_##Type##_Pointer); \
     _edje_edd_edje_##Name##_pointer =					\
       eet_data_descriptor_file_new(&eddc);				\
     EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_##Name##_pointer, Edje_##Type##_Pointer, "pointer", pointer, _edje_edd_edje_##Name); \
   }

void
_edje_edd_init(void)
{
   Eet_Data_Descriptor_Class eddc;

   /* external directory */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_External_Directory_Entry);
   _edje_edd_edje_external_directory_entry =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_external_directory_entry, Edje_External_Directory_Entry, "entry", entry, EET_T_STRING);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_External_Directory);
   _edje_edd_edje_external_directory =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_external_directory, Edje_External_Directory, "entries", entries, _edje_edd_edje_external_directory_entry);

   /* font directory */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Font_Directory_Entry);
   _edje_edd_edje_font_directory_entry =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_font_directory_entry, Edje_Font_Directory_Entry, "entry", entry, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_font_directory_entry, Edje_Font_Directory_Entry, "file", file, EET_T_STRING);

   /* image directory */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Image_Directory_Entry);
   _edje_edd_edje_image_directory_entry =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_entry, Edje_Image_Directory_Entry, "entry", entry, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_entry, Edje_Image_Directory_Entry, "source_type", source_type, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_entry, Edje_Image_Directory_Entry, "source_param", source_param, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_entry, Edje_Image_Directory_Entry, "id", id, EET_T_INT);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Image_Directory_Set_Entry);
   _edje_edd_edje_image_directory_set_entry =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "min.w", size.min.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "min.h", size.min.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "max.w", size.max.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set_entry, Edje_Image_Directory_Set_Entry, "max.h", size.max.h, EET_T_INT);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Image_Directory_Set);
   _edje_edd_edje_image_directory_set =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set, Edje_Image_Directory_Set, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_image_directory_set, Edje_Image_Directory_Set,  "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_image_directory_set, Edje_Image_Directory_Set, "entries", entries, _edje_edd_edje_image_directory_set_entry);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Image_Directory);
   _edje_edd_edje_image_directory =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_image_directory, Edje_Image_Directory, "entries", entries, _edje_edd_edje_image_directory_entry);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_image_directory, Edje_Image_Directory, "sets", sets, _edje_edd_edje_image_directory_set);

   /* collection directory */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Collection_Directory_Entry);
   _edje_edd_edje_part_collection_directory_entry =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "entry", entry, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.RECTANGLE", count.RECTANGLE, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.TEXT", count.RECTANGLE, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.IMAGE", count.RECTANGLE, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.SWALLOW", count.RECTANGLE, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.TEXTBLOCK", count.RECTANGLE, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.GROUP", count.RECTANGLE, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.BOX", count.RECTANGLE, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.TABLE", count.RECTANGLE, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.EXTERNAL", count.RECTANGLE, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection_directory_entry, Edje_Part_Collection_Directory_Entry, "count.part", count.RECTANGLE, EET_T_INT);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Style_Tag);
   _edje_edd_edje_style_tag =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_style_tag, Edje_Style_Tag, "key", key, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_style_tag, Edje_Style_Tag, "value", value, EET_T_STRING);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Style);
   _edje_edd_edje_style =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_style, Edje_Style, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_style, Edje_Style, "tags", tags, _edje_edd_edje_style_tag);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Color_Class);
   _edje_edd_edje_color_class =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "r", r, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "g", g, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "b", b, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "a", a, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "r2", r2, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "g2", g2, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "b2", b2, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "a2", a2, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "r3", r3, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "g3", g3, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "b3", b3, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_color_class, Edje_Color_Class, "a3", a3, EET_T_UCHAR);

   /* the main file directory */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_File);
   _edje_edd_edje_file =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_file, Edje_File, "compiler", compiler, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_file, Edje_File, "version", version, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_file, Edje_File, "feature_ver", feature_ver, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_file, Edje_File, "external_dir", external_dir, _edje_edd_edje_external_directory);
   EET_DATA_DESCRIPTOR_ADD_SUB(_edje_edd_edje_file, Edje_File, "image_dir", image_dir, _edje_edd_edje_image_directory);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_file, Edje_File, "styles", styles, _edje_edd_edje_style);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_file, Edje_File, "color_classes", color_classes, _edje_edd_edje_color_class);
   EET_DATA_DESCRIPTOR_ADD_HASH_STRING(_edje_edd_edje_file, Edje_File, "data", data);
   EET_DATA_DESCRIPTOR_ADD_HASH(_edje_edd_edje_file, Edje_File, "fonts", fonts, _edje_edd_edje_font_directory_entry);
   EET_DATA_DESCRIPTOR_ADD_HASH(_edje_edd_edje_file, Edje_File, "collection", collection, _edje_edd_edje_part_collection_directory_entry);

   /* parts & programs - loaded induvidually */
   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Program_Target);
   _edje_edd_edje_program_target =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program_target, Edje_Program_Target, "id", id, EET_T_INT);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Program_After);
   _edje_edd_edje_program_after =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program_after,
                                 Edje_Program_After, "id", id, EET_T_INT);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Program);
   _edje_edd_edje_program =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "signal", signal, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "source", source, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "filter_part", filter.part, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "filter_state", filter.state, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "in.from", in.from, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "in.range", in.range, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "action", action, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "state", state, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "state2", state2, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "value", value, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "value2", value2, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "tween.mode", tween.mode, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "tween.time", tween.time, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_program, Edje_Program, "targets", targets, _edje_edd_edje_program_target);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_program, Edje_Program, "after", after, _edje_edd_edje_program_after);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "api.name", api.name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "api.description", api.description, EET_T_STRING);

   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "param.src", param.src, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_program, Edje_Program, "param.dst", param.dst, EET_T_INT);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Image_Id);
   _edje_edd_edje_part_image_id =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_image_id, Edje_Part_Image_Id, "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_image_id, Edje_Part_Image_Id, "set", set, EET_T_UCHAR);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_External_Param);
   _edje_edd_edje_external_param =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_external_param, Edje_External_Param, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_external_param, Edje_External_Param, "type", type, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_external_param, Edje_External_Param, "i", i, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_external_param, Edje_External_Param, "d", d, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_external_param, Edje_External_Param, "s", s, EET_T_STRING);

#define EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON(Edd, Type, Dec)	\
   {								\
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "state.name", Dec.state.name, EET_T_STRING); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "state.value", Dec.state.value, EET_T_DOUBLE); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "visible", Dec.visible, EET_T_CHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "align.x", Dec.align.x, EDJE_T_FLOAT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "align.y", Dec.align.y, EDJE_T_FLOAT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "fixed.w", Dec.fixed.w, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "fixed.h", Dec.fixed.h, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "min.w", Dec.min.w, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "min.h", Dec.min.h, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "max.w", Dec.max.w, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "max.h", Dec.max.h, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "step.x", Dec.step.x, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "step.y", Dec.step.y, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "aspect.min", Dec.aspect.min, EDJE_T_FLOAT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "aspect.max", Dec.aspect.max, EDJE_T_FLOAT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "aspect.prefer", Dec.aspect.prefer, EET_T_CHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.relative_x", Dec.rel1.relative_x, EDJE_T_FLOAT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.relative_y", Dec.rel1.relative_y, EDJE_T_FLOAT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.offset_x", Dec.rel1.offset_x, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.offset_y", Dec.rel1.offset_y, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.id_x", Dec.rel1.id_x, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel1.id_y", Dec.rel1.id_y, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.relative_x", Dec.rel2.relative_x, EDJE_T_FLOAT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.relative_y", Dec.rel2.relative_y, EDJE_T_FLOAT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.offset_x", Dec.rel2.offset_x, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.offset_y", Dec.rel2.offset_y, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.id_x", Dec.rel2.id_x, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "rel2.id_y", Dec.rel2.id_y, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color_class", Dec.color_class, EET_T_STRING); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color.r", Dec.color.r, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color.g", Dec.color.g, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color.b", Dec.color.b, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color.a", Dec.color.a, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color2.r", Dec.color2.r, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color2.g", Dec.color2.g, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color2.b", Dec.color2.b, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "color2.a", Dec.color2.a, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.id_persp", Dec.map.id_persp, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.id_light", Dec.map.id_light, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.rot.id_center", Dec.map.rot.id_center, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.rot.x", Dec.map.rot.x, EDJE_T_FLOAT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.rot.y", Dec.map.rot.y, EDJE_T_FLOAT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.rot.z", Dec.map.rot.z, EDJE_T_FLOAT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.on", Dec.map.on, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.smooth", Dec.map.smooth, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.alpha", Dec.map.alpha, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.persp_on", Dec.map.persp_on, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "map.backcull", Dec.map.backcull, EET_T_UCHAR); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "persp.zplane", Dec.persp.zplane, EET_T_INT); \
      EET_DATA_DESCRIPTOR_ADD_BASIC(Edd, Type, "persp.focal", Dec.persp.focal, EET_T_INT); \
   }

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Common);
   _edje_edd_edje_part_description_common =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "state.name", state.name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "state.value", state.value, EET_T_DOUBLE);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "visible", visible, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "align.x", align.x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "align.y", align.y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "fixed.w", fixed.w, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "fixed.h", fixed.h, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "min.w", min.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "min.h", min.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "max.w", max.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "max.h", max.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "step.x", step.x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "step.y", step.y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "aspect.min", aspect.min, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "aspect.max", aspect.max, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "aspect.prefer", aspect.prefer, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "rel1.relative_x", rel1.relative_x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "rel1.relative_y", rel1.relative_y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "rel1.offset_x", rel1.offset_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "rel1.offset_y", rel1.offset_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "rel1.id_x", rel1.id_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "rel1.id_y", rel1.id_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "rel2.relative_x", rel2.relative_x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "rel2.relative_y", rel2.relative_y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "rel2.offset_x", rel2.offset_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "rel2.offset_y", rel2.offset_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "rel2.id_x", rel2.id_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "rel2.id_y", rel2.id_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "color_class", color_class, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "color.r", color.r, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "color.g", color.g, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "color.b", color.b, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "color.a", color.a, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "color2.r", color2.r, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "color2.g", color2.g, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "color2.b", color2.b, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "color2.a", color2.a, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "map.id_persp", map.id_persp, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "map.id_light", map.id_light, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "map.rot.id_center", map.rot.id_center, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "map.rot.x", map.rot.x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "map.rot.y", map.rot.y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "map.rot.z", map.rot.z, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "map.on", map.on, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "map.smooth", map.smooth, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "map.alpha", map.alpha, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "map.persp_on", map.persp_on, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "map.backcull", map.backcull, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "persp.zplane", persp.zplane, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_common, Edje_Part_Description_Common, "persp.focal", persp.focal, EET_T_INT);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Image);
   _edje_edd_edje_part_description_image =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, common);

   EDJE_DEFINE_POINTER_TYPE(Part_Image_Id, part_image_id);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.tweens", image.tweens, _edje_edd_edje_part_image_id_pointer);

   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.id", image.id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.set", image.set, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.border.l", image.border.l, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.border.r", image.border.r, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.border.t", image.border.t, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.border.b", image.border.b, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.border.no_fill", image.border.no_fill, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.border.scale", image.border.scale, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.smooth", image.fill.smooth, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.pos_rel_x", image.fill.pos_rel_x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.pos_abs_x", image.fill.pos_abs_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.rel_x", image.fill.rel_x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.abs_x", image.fill.abs_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.pos_rel_y", image.fill.pos_rel_y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.pos_abs_y", image.fill.pos_abs_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.rel_y", image.fill.rel_y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.abs_y", image.fill.abs_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.angle", image.fill.angle, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.spread", image.fill.spread, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_image, Edje_Part_Description_Image, "image.fill.type", image.fill.type, EET_T_CHAR);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Text);
   _edje_edd_edje_part_description_text =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, common);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.color3.r", text.color3.r, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.color3.g", text.color3.g, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.color3.b", text.color3.b, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.color3.a", text.color3.a, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.text", text.text, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.text_class", text.text_class, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.style", text.style, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.font", text.font, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.repch", text.repch, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.size", text.size, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.fit_x", text.fit_x, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.fit_y", text.fit_y, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.min_x", text.min_x, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.min_y", text.min_y, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.max_x", text.max_x, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.max_y", text.max_y, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.align.x", text.align.x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.align.y", text.align.y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.id_source", text.id_source, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.id_text_source", text.id_text_source, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_text, Edje_Part_Description_Text, "text.elipsis", text.elipsis, EET_T_DOUBLE);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Box);
   _edje_edd_edje_part_description_box =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, common);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.layout", box.layout, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.alt_layout", box.alt_layout, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.align.x", box.align.x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.align.y", box.align.y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.padding.x", box.padding.x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.padding.y", box.padding.y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.min.h", box.min.h, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_box, Edje_Part_Description_Box, "box.min.v", box.min.v, EET_T_UCHAR);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_Table);
   _edje_edd_edje_part_description_table =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, common);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, "table.homogeneous", table.homogeneous, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, "table.align.x", table.align.x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, "table.align.y", table.align.y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, "table.padding.x", table.padding.x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_description_table, Edje_Part_Description_Table, "table.padding.y", table.padding.y, EET_T_INT);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Part_Description_External);
   _edje_edd_edje_part_description_external =
     eet_data_descriptor_file_new(&eddc);
   EDJE_DATA_DESCRIPTOR_DESCRIPTION_COMMON(_edje_edd_edje_part_description_external, Edje_Part_Description_External, common);
   EET_DATA_DESCRIPTOR_ADD_LIST(_edje_edd_edje_part_description_external, Edje_Part_Description_External, "external_params", external_params, _edje_edd_edje_external_param);

   EDJE_DEFINE_POINTER_TYPE(Part_Description_Common, part_description_common);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Image, part_description_image);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Text, part_description_text);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Box, part_description_box);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_Table, part_description_table);
   EDJE_DEFINE_POINTER_TYPE(Part_Description_External, part_description_external);

   eddc.version = EET_DATA_DESCRIPTOR_CLASS_VERSION;
   eddc.func.type_get = _edje_description_variant_type_get;
   eddc.func.type_set = _edje_description_variant_type_set;
   _edje_edd_edje_part_description_variant = eet_data_descriptor_file_new(&eddc);

   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "rectangle", _edje_edd_edje_part_description_common);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "swallow",   _edje_edd_edje_part_description_common);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "group",     _edje_edd_edje_part_description_common);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "image",     _edje_edd_edje_part_description_image);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "text",      _edje_edd_edje_part_description_text);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "textblock", _edje_edd_edje_part_description_text);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "box",       _edje_edd_edje_part_description_box);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "table",     _edje_edd_edje_part_description_table);
   EET_DATA_DESCRIPTOR_ADD_MAPPING(_edje_edd_edje_part_description_variant, "external",  _edje_edd_edje_part_description_external);

#define EDJE_ADD_ARRAY_MAPPING(Variant, Type, Minus)			\
   {									\
     Edje_Part_Description_List tmp;					\
									\
     eet_data_descriptor_element_add(Variant, Type,			\
				     EET_T_UNKNOW, EET_G_VAR_ARRAY,	\
				     0, (char*)(&tmp.desc_count) - (char*)(&tmp), \
				     NULL,				\
				     _edje_edd_edje_part_description_##Minus##_pointer); \
   }

   _edje_edd_edje_part_description_variant_list = eet_data_descriptor_file_new(&eddc);

   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "rectangle", common);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "swallow",   common);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "group",     common);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "image",     image);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "text",      text);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "textblock", text);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "box",       box);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "table",     table);
   EDJE_ADD_ARRAY_MAPPING(_edje_edd_edje_part_description_variant_list, "external",  external);

   EET_EINA_FILE_DATA_DESCRIPTOR_CLASS_SET(&eddc, Edje_Pack_Element);
   _edje_edd_edje_pack_element =
      eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "type", type, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "source", source, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "min.w", min.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "min.h", min.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "prefer.w", prefer.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "prefer.h", prefer.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "max.w", max.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "max.h", max.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "padding.l", padding.l, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "padding.r", padding.r, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "padding.t", padding.t, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "padding.b", padding.b, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "align.x", align.x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "align.y", align.y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "weight.x", weight.x, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "weight.y", weight.y, EDJE_T_FLOAT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "aspect.w", aspect.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "aspect.h", aspect.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "aspect.mode", aspect.mode, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "options", options, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "col", col, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "row", row, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "colspan", colspan, EET_T_USHORT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_pack_element, Edje_Pack_Element, "rowspan", rowspan, EET_T_USHORT);

   eet_eina_file_data_descriptor_class_set(&eddc, "Edje_Part", sizeof (Edje_Part));
   _edje_edd_edje_part =
     eet_data_descriptor_file_new(&eddc);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "name", name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_VARIANT(_edje_edd_edje_part, Edje_Part, "default_desc", default_desc, type, _edje_edd_edje_part_description_variant);
   EET_DATA_DESCRIPTOR_ADD_VARIANT(_edje_edd_edje_part, Edje_Part, "other", other, type, _edje_edd_edje_part_description_variant_list);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "source", source, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "source2", source2, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "source3", source3, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "source4", source4, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "source5", source5, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "source6", source6, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "clip_to_id", clip_to_id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.x", dragable.x, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.step_x", dragable.step_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.count_x", dragable.count_x, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.y", dragable.y, EET_T_CHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.step_y", dragable.step_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.count_y", dragable.count_y, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.counfine_id", dragable.confine_id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "dragable.events_id", dragable.event_id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part, Edje_Part, "items", items, _edje_edd_edje_pack_element);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "type", type, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "effect", effect, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "mouse_events", mouse_events, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "repeat_events", repeat_events, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "ignore_flags", ignore_flags, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "scale", scale, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "precise_is_inside", precise_is_inside, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "use_alternate_font_metrics", use_alternate_font_metrics, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "pointer_mode", pointer_mode, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "entry_mode", entry_mode, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "select_mode", select_mode, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "multiline", multiline, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "api.name", api.name, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part, Edje_Part, "api.description", api.description, EET_T_STRING);

   eet_eina_file_data_descriptor_class_set(&eddc, "Edje_Part_Collection", sizeof (Edje_Part_Collection));
   _edje_edd_edje_part_collection  =
     eet_data_descriptor_file_new(&eddc);

   EDJE_DEFINE_POINTER_TYPE(Program, program);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "programs.fnmatch", programs.fnmatch, _edje_edd_edje_program_pointer);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "programs.strcmp", programs.strcmp, _edje_edd_edje_program_pointer);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "programs.strncmp", programs.strncmp, _edje_edd_edje_program_pointer);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "programs.strrncmp", programs.strrncmp, _edje_edd_edje_program_pointer);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "programs.nocmp", programs.nocmp, _edje_edd_edje_program_pointer);

   EDJE_DEFINE_POINTER_TYPE(Part, part);
   EET_DATA_DESCRIPTOR_ADD_VAR_ARRAY(_edje_edd_edje_part_collection, Edje_Part_Collection, "parts", parts, _edje_edd_edje_part_pointer);

   EET_DATA_DESCRIPTOR_ADD_HASH_STRING(_edje_edd_edje_part_collection, Edje_Part_Collection, "data", data);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "id", id, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_HASH_STRING(_edje_edd_edje_part_collection, Edje_Part_Collection, "alias", alias);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "prop.min.w", prop.min.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "prop.min.h", prop.min.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "prop.max.w", prop.max.w, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "prop.max.h", prop.max.h, EET_T_INT);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "part", part, EET_T_STRING);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "script_only", script_only, EET_T_UCHAR);
   EET_DATA_DESCRIPTOR_ADD_BASIC(_edje_edd_edje_part_collection, Edje_Part_Collection, "lua_script_only", lua_script_only, EET_T_UCHAR);
}
