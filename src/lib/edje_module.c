#ifdef HAVE_CONFIG_H
#include "config.h"
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <Eina.h>
#include <Ecore_File.h>
#include "Edje.h"

#if defined(_WIN32) || defined(__CYGWIN__)
# define MODULE_EXTENSION ".dll"
#else
# define MODULE_EXTENSION ".so"
#endif /* !defined(_WIN32) && !defined(__CYGWIN__) */

#define LOG_COLOR "\033[36m"

Eina_Hash *_registered_modules = NULL;
Eina_Array *_available_modules = NULL;
Eina_List *_modules_name = NULL;
static int _edje_modules_log_dom = -1;

const char *
_edje_module_name_get(Eina_Module *m)
{
   const char *name;
   ssize_t len;

   name = ecore_file_file_get(eina_module_file_get(m));
   len = strlen(name);
   len -= sizeof(MODULE_EXTENSION) - 1;
   if (len <= 0) return NULL;
   return eina_stringshare_add_length(name, len);
}

EAPI Eina_Bool
edje_module_load(const char *module)
{
   Eina_Module *m;

   EINA_SAFETY_ON_NULL_RETURN_VAL(module, EINA_FALSE);

   if (eina_hash_find(_registered_modules, module))
     return EINA_TRUE;

   m = eina_module_find(_available_modules, module);

   if (!m)
     {
       EINA_LOG_ERR("Could not find the module %s", module);
       return EINA_FALSE;
     }

   if (!eina_module_load(m))
     {
       EINA_LOG_ERR("Could not load the module %s", module);
       return EINA_TRUE;
     }

   return !!eina_hash_add(_registered_modules, module, m);
}

void
_edje_module_init(void)
{
   _edje_modules_log_dom = eina_log_domain_register("edje_module", LOG_COLOR);

   if (_edje_modules_log_dom < 0)
     {
       EINA_LOG_ERR("Could not register log domain: edje_module");
       return;
     }

   _registered_modules = eina_hash_string_small_new(NULL);
   _available_modules = eina_module_list_get(_available_modules,
           PACKAGE_LIB_DIR "/lib/edje", 0, NULL, NULL);

   if (!_available_modules)
     {
       eina_hash_free(_registered_modules);
       return;
     }

   unsigned int i;
   Eina_Array_Iterator it;
   Eina_Module *m;

   EINA_ARRAY_ITER_NEXT(_available_modules, i, m, it)
     {
       const char *name;

       name = _edje_module_name_get(m);
       _modules_name = eina_list_append(_modules_name, name);
     }

}

void
_edje_module_shutdown(void)
{
   Eina_List *l;
   const char *data;

   eina_module_list_flush(_available_modules);
   if (_available_modules)
     {
       eina_array_free(_available_modules);
       _available_modules = NULL;
     }

   if (_registered_modules)
     {
       eina_hash_free(_registered_modules);
       _registered_modules = NULL;
     }

   if (_modules_name)
     {
       EINA_LIST_FOREACH(_modules_name, l, data)
         {
           eina_stringshare_del(data);
         }
       _modules_name = NULL;
     }

   eina_log_domain_unregister(_edje_modules_log_dom);
   _edje_modules_log_dom = -1;
}

EAPI const Eina_List *
edje_available_modules_get(void)
{
   return _modules_name;
}
