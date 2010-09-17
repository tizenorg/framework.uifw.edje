#include <string.h>

#include "edje_private.h"

static int _injob = 0;
static Ecore_Job *_job = NULL;
static Ecore_Timer *_job_loss_timer = NULL;

static Eina_List *msgq = NULL;
static Eina_List *tmp_msgq = NULL;

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @addtogroup Edje_message_queue_Group Message_Queue
 *
 * @brief These functions provide an abstraction layer between the
 * application code and the interface, while allowing extremely
 * flexible dynamic layouts and animations.
 *
 * @{
 */

/**
 * @brief Send message to object.
 *
 * @param obj The edje object reference.
 * @param type The type of message to send.
 * @param id A identification number for the message.
 * @param msg The message to be send.
 *
 *
 * This function sends messages to this object and to all of its child
 * objects, if applicable. The function that handles messages arriving
 * at this edje object is is set with
 * edje_object_message_handler_set().
 *
 * @see edje_object_message_handler_set()
 *
 */

EAPI void
edje_object_message_send(Evas_Object *obj, Edje_Message_Type type, int id, void *msg)
{
   Edje *ed;
   unsigned int i;

   ed = _edje_fetch(obj);
   if (!ed) return;
   _edje_message_send(ed, EDJE_QUEUE_SCRIPT, type, id, msg);

   for (i = 0; i < ed->table_parts_size; i++)
     {
	Edje_Real_Part *rp = ed->table_parts[i];
	if ((rp->part->type == EDJE_PART_TYPE_GROUP) && (rp->swallowed_object))
	  edje_object_message_send(rp->swallowed_object, type, id, msg);
     }
}

/**
 * @brief Set the message handler function for this an object.
 *
 * @param obj The edje object reference.
 * @param func The function to handle messages.
 * @param data The data to be associated to the message handler.
 *
 *
 * This function associates a message handler function and data to the
 * edje object.
 *
 */

EAPI void
edje_object_message_handler_set(Evas_Object *obj, void (*func) (void *data, Evas_Object *obj, Edje_Message_Type type, int id, void *msg), void *data)
{
   Edje *ed;

   ed = _edje_fetch(obj);
   if (!ed) return;
   _edje_message_cb_set(ed, func, data);
}

/**
 * @brief Process an object's message queue.
 *
 * @param obj The edje object reference.
 *
 * This function goes through the object message queue processing the
 * pending messages for *this* specific edje object. Normally they'd
 * be processed only at idle time.
 *
 */

EAPI void
edje_object_message_signal_process(Evas_Object *obj)
{
   Eina_List *l, *ln, *tmpq = NULL;
   Edje *ed;
   Edje_Message *em;

   ed = _edje_fetch(obj);
   if (!ed) return;

   for (l = msgq; l; )
     {
        ln = l->next;
        em = l->data;
        if (em->edje == ed)
          {
             tmpq = eina_list_append(tmpq, em);
             msgq = eina_list_remove_list(msgq, l);
          }
        l = ln;
     }
   /* a temporary message queue */
   if (tmp_msgq)
     {
	while (tmpq)
	  {
	     tmp_msgq = eina_list_append(tmp_msgq, tmpq->data);
	     tmpq = eina_list_remove_list(tmpq, tmpq);
	  }
     }
   else
     {
	tmp_msgq = tmpq;
	tmpq = NULL;
     }

   while (tmp_msgq)
     {
	Edje_Message *em;

	em = tmp_msgq->data;
	tmp_msgq = eina_list_remove_list(tmp_msgq, tmp_msgq);
	em->edje->message.num--;
	_edje_message_process(em);
	_edje_message_free(em);
     }
}

/**
 * @brief Process all queued up edje messages.
 *
 * This function triggers the processing of messages addressed to any
 * (alive) edje objects.
 *
 */

EAPI void
edje_message_signal_process(void)
{
   _edje_message_queue_process();
}


static Eina_Bool
_edje_dummy_timer(void *data __UNUSED__)
{
   return ECORE_CALLBACK_CANCEL;
}

static void
_edje_job(void *data __UNUSED__)
{
   if (_job_loss_timer)
     {
	ecore_timer_del(_job_loss_timer);
	_job_loss_timer = NULL;
     }
   _job = NULL;
   _injob++;
   _edje_message_queue_process();
   _injob--;
}

static Eina_Bool
_edje_job_loss_timer(void *data __UNUSED__)
{
   _job_loss_timer = NULL;
   if (!_job)
     {
        _job = ecore_job_add(_edje_job, NULL);
     }
   return ECORE_CALLBACK_CANCEL;
}

void
_edje_message_init(void)
{
}

void
_edje_message_shutdown(void)
{
   _edje_message_queue_clear();
   if (_job_loss_timer)
     {
        ecore_timer_del(_job_loss_timer);
        _job_loss_timer = NULL;
     }
   if (_job)
     {
        ecore_job_del(_job);
        _job = NULL;
     }
}

void
_edje_message_cb_set(Edje *ed, void (*func) (void *data, Evas_Object *obj, Edje_Message_Type type, int id, void *msg), void *data)
{
   unsigned int i;

   ed->message.func = func;
   ed->message.data = data;
   for (i = 0 ; i < ed->table_parts_size ; i++) {
      Edje_Real_Part *rp;
      rp = ed->table_parts[i];
      if (rp->part->type == EDJE_PART_TYPE_GROUP && rp->swallowed_object) {
         Edje *edj2 = _edje_fetch(rp->swallowed_object);
         if (!edj2) continue;
	 _edje_message_cb_set(edj2, func, data);
      }
   }
}

Edje_Message *
_edje_message_new(Edje *ed, Edje_Queue queue, Edje_Message_Type type, int id)
{
   Edje_Message *em;

   em = calloc(1, sizeof(Edje_Message));
   if (!em) return NULL;
   em->edje = ed;
   em->queue = queue;
   em->type = type;
   em->id = id;
   em->edje->message.num++;
   return em;
}

void
_edje_message_free(Edje_Message *em)
{
   if (em->msg)
     {
	int i;

	switch (em->type)
	  {
	   case EDJE_MESSAGE_STRING:
	       {
		  Edje_Message_String *emsg;

		  emsg = (Edje_Message_String *)em->msg;
		  free(emsg->str);
		  free(emsg);
	       }
	     break;
	   case EDJE_MESSAGE_INT:
	       {
		  Edje_Message_Int *emsg;

		  emsg = (Edje_Message_Int *)em->msg;
		  free(emsg);
	       }
	     break;
	   case EDJE_MESSAGE_FLOAT:
	       {
		  Edje_Message_Float *emsg;

		  emsg = (Edje_Message_Float *)em->msg;
		  free(emsg);
	       }
	     break;
	   case EDJE_MESSAGE_INT_SET:
	       {
		  Edje_Message_Int_Set *emsg;

		  emsg = (Edje_Message_Int_Set *)em->msg;
		  free(emsg);
	       }
	     break;
	   case EDJE_MESSAGE_FLOAT_SET:
	       {
		  Edje_Message_Float_Set *emsg;

		  emsg = (Edje_Message_Float_Set *)em->msg;
		  free(emsg);
	       }
	     break;
	   case EDJE_MESSAGE_STRING_FLOAT:
	       {
		  Edje_Message_String_Float *emsg;

		  emsg = (Edje_Message_String_Float *)em->msg;
		  free(emsg->str);
		  free(emsg);
	       }
	     break;
	   case EDJE_MESSAGE_STRING_INT:
	       {
		  Edje_Message_String_Int *emsg;

		  emsg = (Edje_Message_String_Int *)em->msg;
		  free(emsg->str);
		  free(emsg);
	       }
	     break;
	   case EDJE_MESSAGE_STRING_FLOAT_SET:
	       {
		  Edje_Message_String_Float_Set *emsg;

		  emsg = (Edje_Message_String_Float_Set *)em->msg;
		  free(emsg->str);
		  free(emsg);
	       }
	     break;
	   case EDJE_MESSAGE_STRING_INT_SET:
	       {
		  Edje_Message_String_Int_Set *emsg;

		  emsg = (Edje_Message_String_Int_Set *)em->msg;
		  free(emsg->str);
		  free(emsg);
	       }
	     break;
	   case EDJE_MESSAGE_SIGNAL:
	       {
		  Edje_Message_Signal *emsg;

		  emsg = (Edje_Message_Signal *)em->msg;
		  if (emsg->sig) eina_stringshare_del(emsg->sig);
		  if (emsg->src) eina_stringshare_del(emsg->src);
		  free(emsg);
	       }
	     break;
	   case EDJE_MESSAGE_STRING_SET:
	       {
		  Edje_Message_String_Set *emsg;

		  emsg = (Edje_Message_String_Set *)em->msg;
		  for (i = 0; i < emsg->count; i++)
		    free(emsg->str[i]);
		  free(emsg);
	       }
	     break;
	   case EDJE_MESSAGE_NONE:
	   default:
	     break;
	  }
     }
   free(em);
}

void
_edje_message_send(Edje *ed, Edje_Queue queue, Edje_Message_Type type, int id, void *emsg)
{
   /* FIXME: check all malloc & strdup fails and gracefully unroll and exit */
   Edje_Message *em;
   int i;
   unsigned char *msg = NULL;

   em = _edje_message_new(ed, queue, type, id);
   if (!em) return;
   if (_job)
     {
        ecore_job_del(_job);
        _job = NULL;
     }
   if (_injob > 0)
     {
        _job_loss_timer = ecore_timer_add(0.01, _edje_job_loss_timer, NULL);
     }
   else
     {
        if (!_job)
          {
             _job = ecore_job_add(_edje_job, NULL);
          }
        if (_job_loss_timer)
          {
             ecore_timer_del(_job_loss_timer);
             _job_loss_timer = NULL;
          }
     }
   switch (em->type)
     {
      case EDJE_MESSAGE_NONE:
	break;
      case EDJE_MESSAGE_SIGNAL:
	  {
	     Edje_Message_Signal *emsg2, *emsg3;

	     emsg2 = (Edje_Message_Signal *)emsg;
	     emsg3 = calloc(1, sizeof(Edje_Message_Signal));
	     if (emsg2->sig) emsg3->sig = eina_stringshare_add(emsg2->sig);
	     if (emsg2->src) emsg3->src = eina_stringshare_add(emsg2->src);
	     msg = (unsigned char *)emsg3;
	  }
	break;
      case EDJE_MESSAGE_STRING:
	  {
	     Edje_Message_String *emsg2, *emsg3;

	     emsg2 = (Edje_Message_String *)emsg;

	     emsg3 = malloc(sizeof(Edje_Message_String));
	     emsg3->str = strdup(emsg2->str);
	     msg = (unsigned char *)emsg3;
	  }
	break;
      case EDJE_MESSAGE_INT:
	  {
	     Edje_Message_Int *emsg2, *emsg3;

	     emsg2 = (Edje_Message_Int *)emsg;
	     emsg3 = malloc(sizeof(Edje_Message_Int));
	     emsg3->val = emsg2->val;
	     msg = (unsigned char *)emsg3;
	  }
	break;
      case EDJE_MESSAGE_FLOAT:
	  {
	     Edje_Message_Float *emsg2, *emsg3;

	     emsg2 = (Edje_Message_Float *)emsg;
	     emsg3 = malloc(sizeof(Edje_Message_Float));
	     emsg3->val = emsg2->val;
	     msg = (unsigned char *)emsg3;
	  }
	break;
      case EDJE_MESSAGE_STRING_SET:
	  {
	     Edje_Message_String_Set *emsg2, *emsg3;

	     emsg2 = (Edje_Message_String_Set *)emsg;
	     emsg3 = malloc(sizeof(Edje_Message_String_Set) + ((emsg2->count - 1) * sizeof(char *)));
	     emsg3->count = emsg2->count;
	     for (i = 0; i < emsg3->count; i++)
	       emsg3->str[i] = strdup(emsg2->str[i]);
	     msg = (unsigned char *)emsg3;
	  }
	break;
      case EDJE_MESSAGE_INT_SET:
	  {
	     Edje_Message_Int_Set *emsg2, *emsg3;

	     emsg2 = (Edje_Message_Int_Set *)emsg;
	     emsg3 = malloc(sizeof(Edje_Message_Int_Set) + ((emsg2->count - 1) * sizeof(int)));
	     emsg3->count = emsg2->count;
	     for (i = 0; i < emsg3->count; i++)
	       emsg3->val[i] = emsg2->val[i];
	     msg = (unsigned char *)emsg3;
	  }
	break;
      case EDJE_MESSAGE_FLOAT_SET:
	  {
	     Edje_Message_Float_Set *emsg2, *emsg3;

	     emsg2 = (Edje_Message_Float_Set *)emsg;
	     emsg3 = malloc(sizeof(Edje_Message_Float_Set) + ((emsg2->count - 1) * sizeof(double)));
	     emsg3->count = emsg2->count;
	     for (i = 0; i < emsg3->count; i++)
	       emsg3->val[i] = emsg2->val[i];
	     msg = (unsigned char *)emsg3;
	  }
	break;
      case EDJE_MESSAGE_STRING_INT:
	  {
	     Edje_Message_String_Int *emsg2, *emsg3;

	     emsg2 = (Edje_Message_String_Int *)emsg;
	     emsg3 = malloc(sizeof(Edje_Message_String_Int));
	     emsg3->str = strdup(emsg2->str);
	     emsg3->val = emsg2->val;
	     msg = (unsigned char *)emsg3;
	  }
	break;
      case EDJE_MESSAGE_STRING_FLOAT:
	  {
	     Edje_Message_String_Float *emsg2, *emsg3;

	     emsg2 = (Edje_Message_String_Float *)emsg;
	     emsg3 = malloc(sizeof(Edje_Message_String_Float));
	     emsg3->str = strdup(emsg2->str);
	     emsg3->val = emsg2->val;
	     msg = (unsigned char *)emsg3;
	  }
	break;
      case EDJE_MESSAGE_STRING_INT_SET:
	  {
	     Edje_Message_String_Int_Set *emsg2, *emsg3;

	     emsg2 = (Edje_Message_String_Int_Set *)emsg;
	     emsg3 = malloc(sizeof(Edje_Message_String_Int_Set) + ((emsg2->count - 1) * sizeof(int)));
	     emsg3->str = strdup(emsg2->str);
	     emsg3->count = emsg2->count;
	     for (i = 0; i < emsg3->count; i++)
	       emsg3->val[i] = emsg2->val[i];
	     msg = (unsigned char *)emsg3;
	  }
	break;
      case EDJE_MESSAGE_STRING_FLOAT_SET:
	  {
	     Edje_Message_String_Float_Set *emsg2, *emsg3;

	     emsg2 = (Edje_Message_String_Float_Set *)emsg;
	     emsg3 = malloc(sizeof(Edje_Message_String_Float_Set) + ((emsg2->count - 1) * sizeof(double)));
	     emsg3->str = strdup(emsg2->str);
	     emsg3->count = emsg2->count;
	     for (i = 0; i < emsg3->count; i++)
	       emsg3->val[i] = emsg2->val[i];
	     msg = (unsigned char *)emsg3;
	  }
	break;
      default:
	break;
     }

   em->msg = msg;
   msgq = eina_list_append(msgq, em);
}

void
_edje_message_parameters_push(Edje_Message *em)
{
   int i;
   
   /* these params ALWAYS go on */
   /* first param is the message type - always */
   embryo_parameter_cell_push(em->edje->collection->script,
			      (Embryo_Cell)em->type);
   /* 2nd param is the integer of the event id - always there */
   embryo_parameter_cell_push(em->edje->collection->script,
			      (Embryo_Cell)em->id);
   /* the rest is varags of whatever is in the msg */
   switch (em->type)
     {
      case EDJE_MESSAGE_NONE:
	break;
      case EDJE_MESSAGE_STRING:
	embryo_parameter_string_push(em->edje->collection->script,
				     ((Edje_Message_String *)em->msg)->str);
	break;
      case EDJE_MESSAGE_INT:
	  {
	     Embryo_Cell v;

	     v = (Embryo_Cell)((Edje_Message_Int *)em->msg)->val;
	     embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
	  }
	break;
      case EDJE_MESSAGE_FLOAT:
	  {
	     Embryo_Cell v;
	     float fv;

	     fv = ((Edje_Message_Float *)em->msg)->val;
	     v = EMBRYO_FLOAT_TO_CELL(fv);
	     embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
	  }
	break;
      case EDJE_MESSAGE_STRING_SET:
	for (i = 0; i < ((Edje_Message_String_Set *)em->msg)->count; i++)
	  embryo_parameter_string_push(em->edje->collection->script,
				       ((Edje_Message_String_Set *)em->msg)->str[i]);
	break;
      case EDJE_MESSAGE_INT_SET:
	for (i = 0; i < ((Edje_Message_Int_Set *)em->msg)->count; i++)
	  {
	     Embryo_Cell v;

	     v = (Embryo_Cell)((Edje_Message_Int_Set *)em->msg)->val[i];
	     embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
	  }
	break;
      case EDJE_MESSAGE_FLOAT_SET:
	for (i = 0; i < ((Edje_Message_Float_Set *)em->msg)->count; i++)
	  {
	     Embryo_Cell v;
	     float fv;

	     fv = ((Edje_Message_Float_Set *)em->msg)->val[i];
	     v = EMBRYO_FLOAT_TO_CELL(fv);
	     embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
	  }
	break;
      case EDJE_MESSAGE_STRING_INT:
	embryo_parameter_string_push(em->edje->collection->script,
				     ((Edje_Message_String_Int *)em->msg)->str);
	  {
	     Embryo_Cell v;

	     v = (Embryo_Cell)((Edje_Message_String_Int *)em->msg)->val;
	     embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
	  }
	break;
      case EDJE_MESSAGE_STRING_FLOAT:
	embryo_parameter_string_push(em->edje->collection->script,
				     ((Edje_Message_String_Float *)em->msg)->str);
	  {
	     Embryo_Cell v;
	     float fv;

	     fv = ((Edje_Message_String_Float *)em->msg)->val;
	     v = EMBRYO_FLOAT_TO_CELL(fv);
	     embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
	  }
	break;
      case EDJE_MESSAGE_STRING_INT_SET:
	embryo_parameter_string_push(em->edje->collection->script,
				     ((Edje_Message_String_Int_Set *)em->msg)->str);
	for (i = 0; i < ((Edje_Message_String_Int_Set *)em->msg)->count; i++)
	  {
	     Embryo_Cell v;

	     v = (Embryo_Cell)((Edje_Message_String_Int_Set *)em->msg)->val[i];
	     embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
	  }
	break;
      case EDJE_MESSAGE_STRING_FLOAT_SET:
	embryo_parameter_string_push(em->edje->collection->script,
				     ((Edje_Message_String_Float_Set *)em->msg)->str);
	for (i = 0; i < ((Edje_Message_String_Float_Set *)em->msg)->count; i++)
	  {
	     Embryo_Cell v;
	     float fv;

	     fv = ((Edje_Message_String_Float_Set *)em->msg)->val[i];
	     v = EMBRYO_FLOAT_TO_CELL(fv);
	     embryo_parameter_cell_array_push(em->edje->collection->script, &v, 1);
	  }
	break;
      default:
	break;
     }
}

void
_edje_message_process(Edje_Message *em)
{
   Embryo_Function fn;
   void *pdata;

   /* signals are only handled one way */
   if (em->type == EDJE_MESSAGE_SIGNAL)
     {
	_edje_emit_handle(em->edje,
			  ((Edje_Message_Signal *)em->msg)->sig,
			  ((Edje_Message_Signal *)em->msg)->src);
	return;
     }
   /* if this has been queued up for the app then just call the callback */
   if (em->queue == EDJE_QUEUE_APP)
     {
	if (em->edje->message.func)
	  em->edje->message.func(em->edje->message.data, em->edje->obj,
				 em->type, em->id, em->msg);
	return;
     }
   /* now this message is destined for the script message handler fn */
   if (!(em->edje->collection)) return;
   if ((em->edje->collection->script) && _edje_script_only (em->edje))
     {
	_edje_script_only_message(em->edje, em);
	return;
     }
   if (em->edje->L)
     {
	_edje_lua_script_only_message(em->edje, em);
	return;
     }
   fn = embryo_program_function_find(em->edje->collection->script, "message");
   if (fn == EMBRYO_FUNCTION_NONE) return;
   /* reset the engine */
   _edje_embryo_script_reset(em->edje);
   
   _edje_message_parameters_push(em);
   
   embryo_program_vm_push(em->edje->collection->script);
   _edje_embryo_globals_init(em->edje);
   pdata = embryo_program_data_get(em->edje->collection->script);
   embryo_program_data_set(em->edje->collection->script, em->edje);
   embryo_program_max_cycle_run_set(em->edje->collection->script, 5000000);
   embryo_program_run(em->edje->collection->script, fn);
   embryo_program_data_set(em->edje->collection->script, pdata);
   embryo_program_vm_pop(em->edje->collection->script);
}

void
_edje_message_queue_process(void)
{
   int i;

   if (!msgq) return;

   /* allow the message queue to feed itself up to 8 times before forcing */
   /* us to go back to normal processing and let a 0 timeout deal with it */
   for (i = 0; (i < 8) && (msgq); i++)
     {
	/* a temporary message queue */
	if (tmp_msgq)
	  {
	     while (msgq)
	       {
		  tmp_msgq = eina_list_append(tmp_msgq, msgq->data);
		  msgq = eina_list_remove_list(msgq, msgq);
	       }
	  }
	else
	  {
	     tmp_msgq = msgq;
	     msgq = NULL;
	  }

	while (tmp_msgq)
	  {
	     Edje_Message *em;
	     Edje *ed;

	     em = tmp_msgq->data;
	     ed = em->edje;
	     tmp_msgq = eina_list_remove_list(tmp_msgq, tmp_msgq);
	     em->edje->message.num--;
	     if (!ed->delete_me)
	       {
		  ed->processing_messages++;
		  _edje_message_process(em);
		  _edje_message_free(em);
		  ed->processing_messages--;
	       }
	     else
	       _edje_message_free(em);
	     if (ed->processing_messages == 0)
	       {
		  if (ed->delete_me) _edje_del(ed);
	       }
	  }
     }

   /* if the message queue filled again set a timer to expire in 0.0 sec */
   /* to get the idle enterer to be run again */
   if (msgq)
     {
	ecore_timer_add(0.0, _edje_dummy_timer, NULL);
     }
}

void
_edje_message_queue_clear(void)
{
   while (msgq)
     {
	Edje_Message *em;

	em = msgq->data;
	msgq = eina_list_remove_list(msgq, msgq);
	em->edje->message.num--;
	_edje_message_free(em);
     }
   while (tmp_msgq)
     {
	Edje_Message *em;

	em = tmp_msgq->data;
	tmp_msgq = eina_list_remove_list(tmp_msgq, tmp_msgq);
	em->edje->message.num--;
	_edje_message_free(em);
     }
}

void
_edje_message_del(Edje *ed)
{
   Eina_List *l;

   if (ed->message.num <= 0) return;
   /* delete any messages on the main queue for this edje object */
   for (l = msgq; l; )
     {
	Edje_Message *em;
	Eina_List *lp;

	em = eina_list_data_get(l);
	lp = l;
	l = eina_list_next(l);
	if (em->edje == ed)
	  {
	     msgq = eina_list_remove_list(msgq, lp);
	     em->edje->message.num--;
	     _edje_message_free(em);
	  }
	if (ed->message.num <= 0) return;
     }
   /* delete any on the processing queue */
   for (l = tmp_msgq; l; )
     {
	Edje_Message *em;
	Eina_List *lp;

	em = eina_list_data_get(l);
	lp = l;
	l = eina_list_next(l);
	if (em->edje == ed)
	  {
	     tmp_msgq = eina_list_remove_list(tmp_msgq, lp);
	     em->edje->message.num--;
	     _edje_message_free(em);
	  }
	if (ed->message.num <= 0) return;
     }
}

/**
 *
 * @}
 */
