/*
 * vim:ts=8:sw=3:sts=8:noexpandtab:cino=>5n-3f0^-2{2
 */

#include "edje_private.h"

static Eina_Hash *type_registry = NULL;
static int init_count = 0;

EAPI Eina_Bool
edje_external_type_register(const char *type_name, Edje_External_Type *type_info)
{
   if (eina_hash_find(type_registry, type_name))
     {
	printf("EDJE ERROR: external type '%s' already registered\n", type_name);
	return EINA_FALSE;
     }
   return eina_hash_add(type_registry, type_name, type_info);
}

EAPI Eina_Bool
edje_external_type_unregister(const char *type_name)
{
   return eina_hash_del_by_key(type_registry, type_name);
}

EAPI Eina_Iterator *
edje_external_iterator_get(void)
{
   return eina_hash_iterator_tuple_new(type_registry);
}

EAPI Edje_External_Param *
edje_external_param_find(const Eina_List *params, const char *key)
{
   const Eina_List *l;
   Edje_External_Param *param;

   EINA_LIST_FOREACH(params, l, param)
      if (!strcmp(param->name, key)) return param;

   return NULL;
}

EAPI Eina_Bool
edje_external_param_int_get(const Eina_List *params, const char *key, int *ret)
{
   Edje_External_Param *param;

   if (!params) return EINA_FALSE;
   param = edje_external_param_find(params, key);

   if (param && param->type == EDJE_EXTERNAL_PARAM_TYPE_INT && ret)
     {
	*ret = param->i;
	return EINA_TRUE;
     }

   return EINA_FALSE;
}

EAPI Eina_Bool
edje_external_param_double_get(const Eina_List *params, const char *key, double *ret)
{
   Edje_External_Param *param;

   if (!params) return EINA_FALSE;
   param = edje_external_param_find(params, key);

   if (param && param->type == EDJE_EXTERNAL_PARAM_TYPE_DOUBLE && ret)
     {
	*ret = param->d;
	return EINA_TRUE;
     }

   return EINA_FALSE;
}

EAPI Eina_Bool
edje_external_param_string_get(const Eina_List *params, const char *key, const char **ret)
{
   Edje_External_Param *param;

   if (!params) return EINA_FALSE;
   param = edje_external_param_find(params, key);

   if (param && param->type == EDJE_EXTERNAL_PARAM_TYPE_STRING && ret)
     {
	*ret = param->s;
	return EINA_TRUE;
     }

   return EINA_FALSE;
}


void
_edje_external_init()
{
   if (!type_registry)
     type_registry = eina_hash_string_superfast_new(NULL);

   init_count++;
}

void
_edje_external_shutdown()
{
   if (--init_count == 0)
     {
	eina_hash_free(type_registry);
	type_registry = NULL;
     }
}

Evas_Object *
_edje_external_type_add(const char *type_name, Evas *evas, Evas_Object *parent, const Eina_List *params)
{
   Edje_External_Type *type;
   Evas_Object *obj;

   type = eina_hash_find(type_registry, type_name);
   if (!type)
     {
	printf("EDJE ERROR: external type '%s' not registered\n", type_name);
	return NULL;
     }

   obj = type->add(type->data, evas, parent, params);
   if (!obj)
     {
	printf("EDJE ERROR: external type '%s' returned NULL from constructor\n", type_name);
	return NULL;
     }

   evas_object_data_set(obj, "Edje_External_Type", type);

   printf("evas object: %p, external type: %p, data_get: %p\n", obj, type, evas_object_data_get(obj, "Edje_External_Type"));
   return obj;
}

void
_edje_external_signal_emit(Evas_Object *obj, const char *emission, const char *source)
{
   Edje_External_Type *type;

   type = evas_object_data_get(obj, "Edje_External_Type");
   if (!type)
     {
	printf("EDJE ERROR: external type data not found.\n");
	return;
     }

   type->signal_emit(type->data, obj, emission, source);
}

void
_edje_external_params_free(Eina_List *external_params, unsigned int free_strings)
{
   Edje_External_Param *param;

   EINA_LIST_FREE(external_params, param)
     {
	if (free_strings)
	  {
	     if (param->name) eina_stringshare_del(param->name);
	     if (param->s) eina_stringshare_del(param->s);
	  }
	free(param);
     }
}

void
_edje_external_recalc_apply(Edje *ed, Edje_Real_Part *ep,
      Edje_Calc_Params *params,
      Edje_Part_Description *chosen_desc)
{
   Edje_External_Type *type;
   void *params1, *params2 = NULL;
   if (!ep->swallowed_object) return;

   type = evas_object_data_get(ep->swallowed_object, "Edje_External_Type");

   if (!type) return;

   if (!type->state_set) return;

   params1 = ep->param1.external_params ? ep->param1.external_params : ep->param1.description->external_params;
   if (ep->param2 && ep->param2->description)
     params2 = ep->param2->external_params ? ep->param2->external_params : ep->param2->description->external_params;

   type->state_set(type->data, ep->swallowed_object,
	 params1, params2, ep->description_pos);
}

void *
_edje_external_params_parse(Evas_Object *obj, const Eina_List *params)
{
   Edje_External_Type *type;

   type = evas_object_data_get(obj, "Edje_External_Type");
   if (!type) return NULL;

   if (!type->params_parse) return NULL;

   return type->params_parse(type->data, params);
}

void
_edje_external_parsed_params_free(Evas_Object *obj, void *params)
{
   Edje_External_Type *type;

   if (!params) return;

   type = evas_object_data_get(obj, "Edje_External_Type");
   if (!type) return;

   if (!type->params_free) return;

   type->params_free(params);
}
