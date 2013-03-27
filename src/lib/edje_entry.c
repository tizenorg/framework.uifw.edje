#include "edje_private.h"
#include <ctype.h>

#ifdef HAVE_ECORE_IMF
static Eina_Bool _edje_entry_imf_retrieve_surrounding_cb(void *data, Ecore_IMF_Context *ctx, char **text, int *cursor_pos);
static void      _edje_entry_imf_event_commit_cb(void *data, Ecore_IMF_Context *ctx, void *event_info);
static void      _edje_entry_imf_event_preedit_changed_cb(void *data, Ecore_IMF_Context *ctx, void *event_info);
static void      _edje_entry_imf_event_delete_surrounding_cb(void *data, Ecore_IMF_Context *ctx, void *event);
#endif

// TIZEN ONLY - START
static void _edje_entry_start_handler_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _edje_entry_start_handler_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _edje_entry_start_handler_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _edje_entry_end_handler_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _edje_entry_end_handler_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _edje_entry_end_handler_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);

static void _edje_entry_cursor_handler_mouse_down_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _edje_entry_cursor_handler_mouse_move_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
static void _edje_entry_cursor_handler_mouse_up_cb(void *data, Evas *e, Evas_Object *obj, void *event_info);
// TIZEN ONLY - END

typedef struct _Entry Entry;
typedef struct _Sel Sel;
typedef struct _Anchor Anchor;

// TIZEN ONLY - START
typedef enum _Entry_Long_Press_State
{
   _ENTRY_LONG_PRESSING,
   _ENTRY_LONG_PRESSED,
   _ENTRY_LONG_PRESS_RELEASED
} Entry_Long_Press_State;
// TIZEN ONLY - END

static void _edje_entry_imf_cursor_location_set(Entry *en);
static void _edje_entry_imf_cursor_info_set(Entry *en);

struct _Entry
{
   Edje_Real_Part *rp;
   Evas_Coord ox, oy; // TIZEN ONLY
   Evas_Coord sx, sy; // TIZEN ONLY
   Evas_Coord rx, ry; // TIZEN ONLY
   Evas_Coord dx, dy; // TIZEN ONLY
   Evas_Coord_Rectangle layout_region; // TIZEN ONLY
   Evas_Coord_Rectangle viewport_region; // TIZEN ONLY
   Evas_Object *cursor_bg;
   Evas_Object *cursor_fg;
   Evas_Object *sel_handler_start; // TIZEN ONLY
   Evas_Object *sel_handler_end; // TIZEN ONLY
   Evas_Object *sel_handler_edge_start; // TIZEN ONLY
   Evas_Object *sel_handler_edge_end; // TIZEN ONLY
   Evas_Object *cursor_handler; // TIZEN ONLY
   Evas_Textblock_Cursor *cursor;
   Evas_Textblock_Cursor *sel_start, *sel_end;
   Evas_Textblock_Cursor *cursor_user, *cursor_user_extra;
   Evas_Textblock_Cursor *preedit_start, *preedit_end;
   Ecore_Timer *pw_timer;
   Eina_List *sel;
   Eina_List *anchors;
   Eina_List *anchorlist;
   Eina_List *itemlist;
   Eina_List *seq;
   char *selection;
   Edje_Input_Panel_Lang input_panel_lang;
   Eina_Bool composing : 1;
   Eina_Bool selecting : 1;
   Eina_Bool have_selection : 1;
   Eina_Bool select_allow : 1;
   Eina_Bool select_mod_start : 1;
   Eina_Bool select_mod_end : 1;
   Eina_Bool handler_bypassing: 1; // TIZEN ONLY
   Eina_Bool cursor_handler_disabled: 1; // TIZEN ONLY
   Eina_Bool had_sel : 1;
   Eina_Bool input_panel_enable : 1;
   Eina_Bool prediction_allow : 1;
   Eina_Bool focused : 1; // TIZEN ONLY
   // TIZEN ONLY(130129) : Currently, for freezing cursor movement.
   Eina_Bool freeze : 1;
   //
   int select_dragging_state;

#ifdef HAVE_ECORE_IMF
   Eina_Bool have_preedit : 1;
   Eina_Bool commit_cancel : 1; // For skipping useless commit
   Ecore_IMF_Context *imf_context;
#endif

   Ecore_Timer *long_press_timer; // TIZEN ONLY
   Ecore_Timer *cursor_handler_click_timer; // TIZEN ONLY
   Entry_Long_Press_State long_press_state; // TIZEN ONLY
};

struct _Sel
{
   Evas_Textblock_Rectangle rect;
   Evas_Object *obj_fg, *obj_bg, *obj, *sobj;
};

struct _Anchor
{
   Entry *en;
   char *name;
   Evas_Textblock_Cursor *start, *end;
   Eina_List *sel;
   Eina_Bool item : 1;
};

#ifdef HAVE_ECORE_IMF
static void
_preedit_clear(Entry *en)
{
   if (en->preedit_start)
     {
        evas_textblock_cursor_free(en->preedit_start);
        en->preedit_start = NULL;
     }

   if (en->preedit_end)
     {
        evas_textblock_cursor_free(en->preedit_end);
        en->preedit_end = NULL;
     }

   en->have_preedit = EINA_FALSE;
}

static void
_preedit_del(Entry *en)
{
   if (!en || !en->have_preedit) return;
   if (!en->preedit_start || !en->preedit_end) return;
   if (!evas_textblock_cursor_compare(en->preedit_start, en->preedit_end)) return;

   /* delete the preedit characters */
   evas_textblock_cursor_range_delete(en->preedit_start, en->preedit_end);
}

static void
_edje_entry_focus_in_cb(void *data, Evas_Object *o __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Edje_Real_Part *rp;
   Entry *en;

   rp = data;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   if (!rp || !rp->typedata.text->entry_data || !rp->edje || !rp->edje->obj) return;

   en = rp->typedata.text->entry_data;
   if (!en || !en->imf_context) return;

   if (evas_object_focus_get(rp->edje->obj))
     {
        // TIZEN ONLY - START
        if ((!en->sel_handler_start) && (!en->sel_handler_end) &&
            (en->rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_BLOCK_HANDLE))
          {
             if (rp->part->source8)
               {
                  en->sel_handler_end = edje_object_add(en->rp->edje->base.evas);
                  edje_object_file_set(en->sel_handler_end, en->rp->edje->path, en->rp->part->source8);

                  evas_object_layer_set(en->sel_handler_end, EVAS_LAYER_MAX - 2);
                  en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, en->sel_handler_end);
                  evas_object_event_callback_add(en->sel_handler_end, EVAS_CALLBACK_MOUSE_DOWN, _edje_entry_end_handler_mouse_down_cb, en->rp);
                  evas_object_event_callback_add(en->sel_handler_end, EVAS_CALLBACK_MOUSE_UP, _edje_entry_end_handler_mouse_up_cb, en->rp);
                  evas_object_event_callback_add(en->sel_handler_end, EVAS_CALLBACK_MOUSE_MOVE, _edje_entry_end_handler_mouse_move_cb, en->rp);
               }

             if (rp->part->source7)
               {
                  en->sel_handler_start = edje_object_add(en->rp->edje->base.evas);
                  edje_object_file_set(en->sel_handler_start, en->rp->edje->path, en->rp->part->source7);
                  evas_object_layer_set(en->sel_handler_start, EVAS_LAYER_MAX - 2);
                  en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, en->sel_handler_start);
                  evas_object_event_callback_add(en->sel_handler_start, EVAS_CALLBACK_MOUSE_DOWN, _edje_entry_start_handler_mouse_down_cb, en->rp);
                  evas_object_event_callback_add(en->sel_handler_start, EVAS_CALLBACK_MOUSE_UP, _edje_entry_start_handler_mouse_up_cb, en->rp);
                  evas_object_event_callback_add(en->sel_handler_start, EVAS_CALLBACK_MOUSE_MOVE, _edje_entry_start_handler_mouse_move_cb, en->rp);
               }

             //start edge handler
             if (rp->part->source10)
               {
                  en->sel_handler_edge_start = edje_object_add(en->rp->edje->base.evas);
                  edje_object_file_set(en->sel_handler_edge_start, en->rp->edje->path, en->rp->part->source10);
                  evas_object_layer_set(en->sel_handler_edge_start, EVAS_LAYER_MAX - 2);
                  en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, en->sel_handler_edge_start);
                  evas_object_clip_set(en->sel_handler_edge_start, evas_object_clip_get(rp->object));
               }

             //end edge handler
             if (rp->part->source11)
               {
                  en->sel_handler_edge_end = edje_object_add(en->rp->edje->base.evas);
                  edje_object_file_set(en->sel_handler_edge_end, en->rp->edje->path, en->rp->part->source11);
                  evas_object_layer_set(en->sel_handler_edge_end, EVAS_LAYER_MAX - 2);
                  en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, en->sel_handler_edge_end);
                  evas_object_clip_set(en->sel_handler_edge_end, evas_object_clip_get(rp->object));
               }
          }
        if (en->sel_handler_start)
           edje_object_signal_emit(en->sel_handler_start, "edje,focus,in", "edje");
        if (en->sel_handler_end)
           edje_object_signal_emit(en->sel_handler_end, "edje,focus,in", "edje");
        if (en->cursor_handler)
           edje_object_signal_emit(en->cursor_handler, "edje,focus,in", "edje");
        if (en->sel_handler_edge_start)
           edje_object_signal_emit(en->sel_handler_edge_start, "edje,focus,in", "edje");
        if (en->sel_handler_edge_end)
           edje_object_signal_emit(en->sel_handler_edge_end, "edje,focus,in", "edje");
        // TIZEN ONLY - END

        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_focus_in(en->imf_context);
        _edje_entry_imf_cursor_info_set(en);
        en->focused = EINA_TRUE;
     }
}

static void
_edje_entry_focus_out_cb(void *data, Evas_Object *o __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Edje_Real_Part *rp;
   Entry *en;

   rp = data;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   if (!rp || !rp->typedata.text->entry_data) return;

   en = rp->typedata.text->entry_data;
   if (!en || !en->imf_context) return;

   // TIZEN ONLY - START
   if (en->sel_handler_start)
     edje_object_signal_emit(en->sel_handler_start, "edje,focus,out", "edje");
   if (en->sel_handler_end)
     edje_object_signal_emit(en->sel_handler_end, "edje,focus,out", "edje");
   if (en->cursor_handler)
      edje_object_signal_emit(en->cursor_handler, "edje,focus,out", "edje");
   if (en->sel_handler_edge_start)
     edje_object_signal_emit(en->sel_handler_edge_start, "edje,focus,out", "edje");
   if (en->sel_handler_edge_end)
     edje_object_signal_emit(en->sel_handler_edge_end, "edje,focus,out", "edje");
   // TIZEN ONLY - END

   ecore_imf_context_reset(en->imf_context);
   ecore_imf_context_focus_out(en->imf_context);
}
#endif

static void
_edje_focus_in_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Edje *ed = data;
#ifdef HAVE_ECORE_IMF
   Edje_Real_Part *rp;
   Entry *en;
#endif

   _edje_emit(ed, "focus,in", "");

   rp = ed->focused_part;
   if (!rp) return;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))   //  TIZEN ONLY
     return;

   // TIZEN ONLY - START
   if ((!en->sel_handler_start) && (!en->sel_handler_end) &&
       (en->rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_BLOCK_HANDLE))
     {
        if (rp->part->source8)
          {
             en->sel_handler_end = edje_object_add(en->rp->edje->base.evas);
             edje_object_file_set(en->sel_handler_end, en->rp->edje->path, en->rp->part->source8);

             evas_object_layer_set(en->sel_handler_end, EVAS_LAYER_MAX - 2);
             en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, en->sel_handler_end);
             evas_object_event_callback_add(en->sel_handler_end, EVAS_CALLBACK_MOUSE_DOWN, _edje_entry_end_handler_mouse_down_cb, en->rp);
             evas_object_event_callback_add(en->sel_handler_end, EVAS_CALLBACK_MOUSE_UP, _edje_entry_end_handler_mouse_up_cb, en->rp);
             evas_object_event_callback_add(en->sel_handler_end, EVAS_CALLBACK_MOUSE_MOVE, _edje_entry_end_handler_mouse_move_cb, en->rp);
          }

        if (rp->part->source7)
          {
             en->sel_handler_start = edje_object_add(en->rp->edje->base.evas);
             edje_object_file_set(en->sel_handler_start, en->rp->edje->path, en->rp->part->source7);
             evas_object_layer_set(en->sel_handler_start, EVAS_LAYER_MAX - 2);
             en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, en->sel_handler_start);
             evas_object_event_callback_add(en->sel_handler_start, EVAS_CALLBACK_MOUSE_DOWN, _edje_entry_start_handler_mouse_down_cb, en->rp);
             evas_object_event_callback_add(en->sel_handler_start, EVAS_CALLBACK_MOUSE_UP, _edje_entry_start_handler_mouse_up_cb, en->rp);
             evas_object_event_callback_add(en->sel_handler_start, EVAS_CALLBACK_MOUSE_MOVE, _edje_entry_start_handler_mouse_move_cb, en->rp);
          }

        //start edge handler
        if (rp->part->source10)
          {
             en->sel_handler_edge_start = edje_object_add(en->rp->edje->base.evas);
             edje_object_file_set(en->sel_handler_edge_start, en->rp->edje->path, en->rp->part->source10);
             evas_object_layer_set(en->sel_handler_edge_start, EVAS_LAYER_MAX - 2);
             en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, en->sel_handler_edge_start);
             evas_object_clip_set(en->sel_handler_edge_start, evas_object_clip_get(rp->object));
          }

        //end edge handler
        if (rp->part->source11)
          {
             en->sel_handler_edge_end = edje_object_add(en->rp->edje->base.evas);
             edje_object_file_set(en->sel_handler_edge_end, en->rp->edje->path, en->rp->part->source11);
             evas_object_layer_set(en->sel_handler_edge_end, EVAS_LAYER_MAX - 2);
             en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, en->sel_handler_edge_end);
             evas_object_clip_set(en->sel_handler_edge_end, evas_object_clip_get(rp->object));
          }
     }
   if (en->sel_handler_start)
      edje_object_signal_emit(en->sel_handler_start, "edje,focus,in", "edje");
   if (en->sel_handler_end)
      edje_object_signal_emit(en->sel_handler_end, "edje,focus,in", "edje");
   if (en->cursor_handler)
      edje_object_signal_emit(en->cursor_handler, "edje,focus,in", "edje");
   if (en->sel_handler_edge_start)
      edje_object_signal_emit(en->sel_handler_edge_start, "edje,focus,in", "edje");
   if (en->sel_handler_edge_end)
      edje_object_signal_emit(en->sel_handler_edge_end, "edje,focus,in", "edje");

   en->focused = EINA_TRUE;
   if (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_EDITABLE) return;
   // TIZEN ONLY - END

#ifdef HAVE_ECORE_IMF
   if (!en->imf_context) return;

   ecore_imf_context_reset(en->imf_context);
   ecore_imf_context_focus_in(en->imf_context);
   _edje_entry_imf_cursor_info_set(en);
#endif
}

static void
_edje_focus_out_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Edje *ed = data;
#ifdef HAVE_ECORE_IMF
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en;
#endif

   _edje_emit(ed, "focus,out", "");

#ifdef HAVE_ECORE_IMF
   if (!rp) return;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))  // TIZEN ONLY
     return;

   // TIZEN ONLY - START
   if (en->sel_handler_start)
     edje_object_signal_emit(en->sel_handler_start, "edje,focus,out", "edje");
   if (en->sel_handler_end)
     edje_object_signal_emit(en->sel_handler_end, "edje,focus,out", "edje");
   if (en->cursor_handler)
      edje_object_signal_emit(en->cursor_handler, "edje,focus,out", "edje");
   if (en->sel_handler_edge_start)
     edje_object_signal_emit(en->sel_handler_edge_start, "edje,focus,out", "edje");
   if (en->sel_handler_edge_end)
     edje_object_signal_emit(en->sel_handler_edge_end, "edje,focus,out", "edje");
   en->focused = EINA_FALSE;
   if (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_EDITABLE) return;
   // TIZEN ONLY - END

   if (!en->imf_context) return;

   ecore_imf_context_reset(en->imf_context);
   ecore_imf_context_focus_out(en->imf_context);
#endif
}

static void
_text_filter_markup_prepend_internal(Entry *en, Evas_Textblock_Cursor *c, char *text)
{
   Edje_Markup_Filter_Callback *cb;
   Eina_List *l;

   EINA_LIST_FOREACH(en->rp->edje->markup_filter_callbacks, l, cb)
     {
        if (!strcmp(cb->part, en->rp->part->name))
          {
             cb->func(cb->data, en->rp->edje->obj, cb->part, &text);
             if (!text) break;
          }
     }
#ifdef HAVE_ECORE_IMF
   // For skipping useless commit
   if (en->have_preedit && (!text || !strcmp(text, "")))
      en->commit_cancel = EINA_TRUE;
#endif
   if (text)
     {
        evas_object_textblock_text_markup_prepend(c, text);
        free(text);
     }
}

static void
_text_filter_text_prepend(Entry *en, Evas_Textblock_Cursor *c, const char *text)
{
   char *text2;
   Edje_Text_Insert_Filter_Callback *cb;
   Eina_List *l;

   text2 = strdup(text);
   EINA_LIST_FOREACH(en->rp->edje->text_insert_filter_callbacks, l, cb)
     {
        if (!strcmp(cb->part, en->rp->part->name))
          {
             cb->func(cb->data, en->rp->edje->obj, cb->part, EDJE_TEXT_FILTER_TEXT, &text2);
             if (!text2) break;
          }
     }
   if (text2)
     {
        char *markup_text;
        markup_text = evas_textblock_text_utf8_to_markup(NULL, text2);
        free(text2);
        if (markup_text)
          _text_filter_markup_prepend_internal(en, c, markup_text);
     }
}

static void
_text_filter_format_prepend(Entry *en, Evas_Textblock_Cursor *c, const char *text)
{
   char *text2;
   Edje_Text_Insert_Filter_Callback *cb;
   Eina_List *l;

   text2 = strdup(text);
   EINA_LIST_FOREACH(en->rp->edje->text_insert_filter_callbacks, l, cb)
     {
        if (!strcmp(cb->part, en->rp->part->name))
          {
             cb->func(cb->data, en->rp->edje->obj, cb->part, EDJE_TEXT_FILTER_FORMAT, &text2);
             if (!text2) break;
          }
     }
   if (text2)
     {
        char *s, *markup_text;

        s = text2;
        if (*s == '+')
          {
             s++;
             while (*s == ' ') s++;
             if (!s)
               {
                  free(text2);
                  return;
               }
             markup_text = (char*) malloc(strlen(s) + 3);
             if (markup_text)
               {
                  *(markup_text) = '<';
                  strncpy((markup_text + 1), s, strlen(s));
                  *(markup_text + strlen(s) + 1) = '>';
                  *(markup_text + strlen(s) + 2) = '\0';
               }
          }
        else if (s[0] == '-')
          {
             s++;
             while (*s == ' ') s++;
             if (!s)
               {
                  free(text2);
                  return;
               }
             markup_text = (char*) malloc(strlen(s) + 4);
             if (markup_text)
               {
                  *(markup_text) = '<';
                  *(markup_text + 1) = '/';
                  strncpy((markup_text + 2), s, strlen(s));
                  *(markup_text + strlen(s) + 2) = '>';
                  *(markup_text + strlen(s) + 3) = '\0';
               }
          }
        else
          {
             markup_text = (char*) malloc(strlen(s) + 4);
             if (markup_text)
               {
                  *(markup_text) = '<';
                  strncpy((markup_text + 1), s, strlen(s));
                  *(markup_text + strlen(s) + 1) = '/';
                  *(markup_text + strlen(s) + 2) = '>';
                  *(markup_text + strlen(s) + 3) = '\0';
               }
          }
        free(text2);
        if (markup_text)
          _text_filter_markup_prepend_internal(en, c, markup_text);
     }
}

static void
_text_filter_markup_prepend(Entry *en, Evas_Textblock_Cursor *c, const char *text)
{
   char *text2;
   Edje_Text_Insert_Filter_Callback *cb;
   Eina_List *l;

   text2 = strdup(text);
   EINA_LIST_FOREACH(en->rp->edje->text_insert_filter_callbacks, l, cb)
     {
        if (!strcmp(cb->part, en->rp->part->name))
          {
             cb->func(cb->data, en->rp->edje->obj, cb->part, EDJE_TEXT_FILTER_MARKUP, &text2);
             if (!text2) break;
          }
     }
   if (text2)
     _text_filter_markup_prepend_internal(en, c, text2);
}

static void
_curs_update_from_curs(Evas_Textblock_Cursor *c, Evas_Object *o __UNUSED__, Entry *en, Evas_Coord *cx, Evas_Coord *cy)
{
   Evas_Coord cw, ch;
   Evas_Textblock_Cursor_Type cur_type;
   if (c != en->cursor) return;
   switch (en->rp->part->cursor_mode)
     {
      case EDJE_ENTRY_CURSOR_MODE_BEFORE:
         cur_type = EVAS_TEXTBLOCK_CURSOR_BEFORE;
         break;
      case EDJE_ENTRY_CURSOR_MODE_UNDER:
         /* no break for a resaon */
      default:
         cur_type = EVAS_TEXTBLOCK_CURSOR_UNDER;
     }
   evas_textblock_cursor_geometry_get(c, cx, cy, &cw, &ch, NULL, cur_type);
   *cx += (cw / 2);
   *cy += (ch / 2);
}

static int
_curs_line_last_get(Evas_Textblock_Cursor *c __UNUSED__, Evas_Object *o, Entry *en __UNUSED__)
{
   Evas_Textblock_Cursor *cc;
   int ln;

   cc = evas_object_textblock_cursor_new(o);
   evas_textblock_cursor_paragraph_last(cc);
   ln = evas_textblock_cursor_line_geometry_get(cc, NULL, NULL, NULL, NULL);
   evas_textblock_cursor_free(cc);
   return ln;
}

static void
_curs_lin_start(Evas_Textblock_Cursor *c, Evas_Object *o __UNUSED__,
                Entry *en __UNUSED__)
{
   evas_textblock_cursor_line_char_first(c);
}

static void
_curs_lin_end(Evas_Textblock_Cursor *c, Evas_Object *o __UNUSED__,
              Entry *en __UNUSED__)
{
   evas_textblock_cursor_line_char_last(c);
}

static void
_curs_start(Evas_Textblock_Cursor *c, Evas_Object *o __UNUSED__,
            Entry *en __UNUSED__)
{
   evas_textblock_cursor_paragraph_first(c);
}

static void
_curs_end(Evas_Textblock_Cursor *c, Evas_Object *o __UNUSED__, Entry *en __UNUSED__)
{
   evas_textblock_cursor_paragraph_last(c);
}

static void
_curs_jump_line(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en, int ln)
{
   Evas_Coord cx, cy;
   Evas_Coord lx, ly, lw, lh;
   int last = _curs_line_last_get(c, o, en);

   if (ln < 0) ln = 0;
   else
     {
        if (ln > last) ln = last;
     }

   _curs_update_from_curs(c, o, en, &cx, &cy);

   if (!evas_object_textblock_line_number_geometry_get(o, ln, &lx, &ly, &lw, &lh))
     return;
   if (evas_textblock_cursor_char_coord_set(c, cx, ly + (lh / 2)))
     return;
   evas_textblock_cursor_line_set(c, ln);
   if (cx < (lx + (lw / 2)))
     {
        if (ln == last) _curs_end(c, o, en);
        _curs_lin_start(c, o, en);
     }
   else
     {
        if (ln == last)
          _curs_end(c, o, en);
        else
          _curs_lin_end(c, o, en);
     }
}

static void
_curs_jump_line_by(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en, int by)
{
   int ln;

   ln = evas_textblock_cursor_line_geometry_get(c, NULL, NULL, NULL, NULL) + by;
   _curs_jump_line(c, o, en, ln);
}

static void
_curs_up(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   _curs_jump_line_by(c, o, en, -1);
}

static void
_curs_down(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   _curs_jump_line_by(c, o, en, 1);
}

static void
_sel_start(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   if (en->sel_start) return;
   en->sel_start = evas_object_textblock_cursor_new(o);
   evas_textblock_cursor_copy(c, en->sel_start);
   en->sel_end = evas_object_textblock_cursor_new(o);
   evas_textblock_cursor_copy(c, en->sel_end);

   en->have_selection = EINA_FALSE;
   if (en->selection)
     {
        free(en->selection);
        en->selection = NULL;
     }
}

static void
_sel_enable(Evas_Textblock_Cursor *c __UNUSED__, Evas_Object *o __UNUSED__, Entry *en)
{
   if (en->have_selection) return;
   en->have_selection = EINA_TRUE;
   if (en->selection)
     {
        free(en->selection);
        en->selection = NULL;
     }
   _edje_emit(en->rp->edje, "selection,start", en->rp->part->name);
}

static void
_sel_extend(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   if (!en->sel_end) return;
   _edje_entry_imf_context_reset(en->rp);
   _sel_enable(c, o, en);
   if (!evas_textblock_cursor_compare(c, en->sel_end)) return;

   evas_textblock_cursor_copy(c, en->sel_end);

   _edje_entry_imf_cursor_info_set(en);

   if (en->selection)
     {
        free(en->selection);
        en->selection = NULL;
     }
   _edje_emit(en->rp->edje, "selection,changed", en->rp->part->name);
   // TIZEN ONLY - start
   if (en->cursor_handler)
      evas_object_hide(en->cursor_handler);
   // TIZEN ONLY - end
}

static void
_sel_preextend(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   if (!en->sel_end) return;
   _edje_entry_imf_context_reset(en->rp);
   _sel_enable(c, o, en);
   if (!evas_textblock_cursor_compare(c, en->sel_start)) return;

   evas_textblock_cursor_copy(c, en->sel_start);

   _edje_entry_imf_cursor_info_set(en);

   if (en->selection)
     {
        free(en->selection);
        en->selection = NULL;
     }
   _edje_emit(en->rp->edje, "selection,changed", en->rp->part->name);
   // TIZEN ONLY - start
   if (en->cursor_handler)
      evas_object_hide(en->cursor_handler);
   // TIZEN ONLY - end
}

static void
_sel_clear(Evas_Textblock_Cursor *c __UNUSED__, Evas_Object *o __UNUSED__, Entry *en)
{
   en->had_sel = EINA_FALSE;
   if (en->sel_start)
     {
        evas_textblock_cursor_free(en->sel_start);
        evas_textblock_cursor_free(en->sel_end);
        en->sel_start = NULL;
        en->sel_end = NULL;
     }
   if (en->selection)
     {
        free(en->selection);
        en->selection = NULL;
     }
   while (en->sel)
     {
        Sel *sel;

        sel = en->sel->data;
        if (sel->obj_bg) evas_object_del(sel->obj_bg);
        if (sel->obj_fg) evas_object_del(sel->obj_fg);

        // TIZEN ONLY - START
        if (en->rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_BLOCK_HANDLE)
          {
             evas_object_hide(en->sel_handler_start);
             evas_object_hide(en->sel_handler_end);

             evas_object_hide(en->sel_handler_edge_start);
             evas_object_hide(en->sel_handler_edge_end);
          }
        // TIZEN ONLY - END
        free(sel);
        en->sel = eina_list_remove_list(en->sel, en->sel);
     }
   if (en->have_selection)
     {
        en->have_selection = EINA_FALSE;
        evas_object_show(en->cursor_fg);
        _edje_emit(en->rp->edje, "selection,cleared", en->rp->part->name);
     }
}

static void
_sel_update(Evas_Textblock_Cursor *c __UNUSED__, Evas_Object *o, Entry *en)
{
   Eina_List *range = NULL, *l;
   Sel *sel;
   Evas_Coord x, y, w, h;
   Evas_Object *smart, *clip;

   smart = evas_object_smart_parent_get(o);
   clip = evas_object_clip_get(o);
   if (en->sel_start)
     {
        range = evas_textblock_cursor_range_geometry_get(en->sel_start, en->sel_end);
        if (!range) return;
     }
   else
     return;
   if (eina_list_count(range) != eina_list_count(en->sel))
     {
        while (en->sel)
          {
             sel = en->sel->data;
             if (sel->obj_bg) evas_object_del(sel->obj_bg);
             if (sel->obj_fg) evas_object_del(sel->obj_fg);
             free(sel);
             en->sel = eina_list_remove_list(en->sel, en->sel);
          }
        if (en->have_selection)
          {
             for (l = range; l; l = eina_list_next(l))
               {
                  Evas_Object *ob;

                  sel = calloc(1, sizeof(Sel));
                  en->sel = eina_list_append(en->sel, sel);
                  ob = edje_object_add(en->rp->edje->base.evas);
                  edje_object_file_set(ob, en->rp->edje->path, en->rp->part->source);
                  evas_object_smart_member_add(ob, smart);
                  evas_object_stack_below(ob, o);
                  evas_object_clip_set(ob, clip);
                  evas_object_pass_events_set(ob, EINA_TRUE);
                  evas_object_show(ob);
                  sel->obj_bg = ob;
                  _edje_subobj_register(en->rp->edje, sel->obj_bg);

                  ob = edje_object_add(en->rp->edje->base.evas);
                  edje_object_file_set(ob, en->rp->edje->path, en->rp->part->source2);
                  evas_object_smart_member_add(ob, smart);
                  evas_object_stack_above(ob, o);
                  evas_object_clip_set(ob, clip);
                  evas_object_pass_events_set(ob, EINA_TRUE);
                  evas_object_show(ob);
                  sel->obj_fg = ob;
                  _edje_subobj_register(en->rp->edje, sel->obj_fg);
               }
          }
     }
   x = y = w = h = -1;
   evas_object_geometry_get(o, &x, &y, &w, &h);
   if (en->have_selection)
     {
        // TIZEN ONLY - START
        int list_cnt, list_idx;

        list_cnt = eina_list_count(en->sel);
        list_idx = 0;
        evas_object_hide(en->cursor_fg);
        evas_object_hide(en->cursor_bg);
        // TIZEN ONLY - END

        EINA_LIST_FOREACH(en->sel, l, sel)
          {
             Evas_Textblock_Rectangle *r;
             list_idx++; // TIZEN ONLY

             r = range->data;
             if (sel->obj_bg)
               {
                  evas_object_move(sel->obj_bg, x + r->x, y + r->y);
                  evas_object_resize(sel->obj_bg, r->w, r->h);
               }
             if (sel->obj_fg)
               {
                  evas_object_move(sel->obj_fg, x + r->x, y + r->y);
                  evas_object_resize(sel->obj_fg, r->w, r->h);
               }
             // TIZEN ONLY - START
             if (en->rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_BLOCK_HANDLE)
               {
                  int fh_idx, eh_idx;
                  const char *gc = edje_object_data_get(en->sel_handler_start, "gap");
                  int bh_gap = 0;
                  if (gc) bh_gap = atoi(gc);

                  if (en->handler_bypassing)
                    {
                       fh_idx = list_cnt;
                       eh_idx = 1;
                    }
                  else
                    {
                       fh_idx = 1;
                       eh_idx = list_cnt;
                    }

                  if (list_idx == fh_idx)
                    {
                       Evas_Coord nx, ny, handler_height = 0;
                       Evas_Coord edge_w;
                       edje_object_part_geometry_get(en->sel_handler_start, "handle", NULL, NULL, NULL, &handler_height);

                       //keep same touch pos when by passing
                       if (en->handler_bypassing)
                         {
                            nx = x + r->x + r->w;
                            ny = y + r->y;
                         }
                       else
                         {
                            nx = x + r->x;
                            ny = y + r->y;
                         }
                       evas_object_hide(en->sel_handler_start);

                       edje_object_size_min_calc(en->sel_handler_edge_start, &edge_w, NULL);
                       evas_object_move(en->sel_handler_edge_start, nx, ny);
                       evas_object_resize(en->sel_handler_edge_start, edge_w, r->h);

                       evas_object_hide(en->sel_handler_edge_start);

                       // if viewport region is not set, show handlers
                       if (en->viewport_region.x == -1 && en->viewport_region.y == -1 &&
                           en->viewport_region.w == -1 && en->viewport_region.h == -1)
                         {
                            evas_object_hide(en->sel_handler_start);
                            evas_object_move(en->sel_handler_start, nx, ny);
                            edje_object_signal_emit(en->sel_handler_start, "elm,state,top", "elm");
                            evas_object_show(en->sel_handler_start);

                            evas_object_show(en->sel_handler_edge_start);
                         }
                       else
                         {
                            if ((en->viewport_region.w > 0 && en->viewport_region.h > 0) &&
                                (nx >= en->viewport_region.x) && (nx <= (en->viewport_region.x + en->viewport_region.w)) &&
                                (ny >= en->viewport_region.y) && (ny <= (en->viewport_region.y + en->viewport_region.h)))
                              {
                                 if (en->layout_region.w != -1 && en->layout_region.h != -1 &&
                                     ((ny - handler_height) > en->layout_region.y))
                                   {
                                      evas_object_move(en->sel_handler_start, nx, ny);
                                      if (nx <= en->layout_region.x + bh_gap)
                                         edje_object_signal_emit(en->sel_handler_start, "elm,state,top,reversed", "elm");
                                      else
                                         edje_object_signal_emit(en->sel_handler_start, "elm,state,top", "elm");
                                   }
                                 else
                                   {
                                      evas_object_move(en->sel_handler_start, nx, ny + r->h);
                                      if (nx <= en->layout_region.x + bh_gap)
                                         edje_object_signal_emit(en->sel_handler_start, "elm,state,bottom,reversed", "elm");
                                      else
                                         edje_object_signal_emit(en->sel_handler_start, "elm,state,bottom", "elm");
                                   }

                                 evas_object_show(en->sel_handler_start);
                                 evas_object_show(en->sel_handler_edge_start);
                              }
                         }
                    }
                  if (list_idx == eh_idx)
                    {
                       Evas_Coord nx, ny, handler_height = 0;
                       Evas_Coord edge_w;

                       edje_object_part_geometry_get(en->sel_handler_end, "handle", NULL, NULL, NULL, &handler_height);

                       //keep pos when bypassing
                       if (en->handler_bypassing)
                         {
                            nx = x + r->x;
                            ny = y + r->y + r->h;
                         }
                       else
                         {
                            nx = x + r->x + r->w;
                            ny = y + r->y + r->h;
                         }

                       evas_object_hide(en->sel_handler_end);

                       edje_object_size_min_calc(en->sel_handler_edge_end, &edge_w, NULL);
                       evas_object_move(en->sel_handler_edge_end, nx, ny - r->h);
                       evas_object_resize(en->sel_handler_edge_end, edge_w, r->h);

                       evas_object_hide(en->sel_handler_edge_end);

                       // if viewport region is not set, show handlers
                       if (en->viewport_region.x == -1 && en->viewport_region.y == -1 &&
                           en->viewport_region.w == -1 && en->viewport_region.h == -1)
                         {
                            evas_object_move(en->sel_handler_end, nx, ny);
                            evas_object_move(en->sel_handler_end, nx, ny);
                            edje_object_signal_emit(en->sel_handler_end, "elm,state,bottom", "elm");
                            evas_object_show(en->sel_handler_end);

                            evas_object_show(en->sel_handler_edge_end);
                         }
                       else
                         {
                            if ((en->viewport_region.w > 0 && en->viewport_region.h > 0) &&
                                (nx >= en->viewport_region.x) && (nx <= (en->viewport_region.x + en->viewport_region.w)) &&
                                (ny >= en->viewport_region.y) && (ny <= (en->viewport_region.y + en->viewport_region.h)))
                              {
                                 evas_object_move(en->sel_handler_end, nx, ny - r->h);
                                 if (en->layout_region.w != -1 && en->layout_region.h != -1 &&
                                     ((ny + handler_height) > (en->layout_region.y + en->layout_region.h)))
                                   {
                                      if (nx >= en->layout_region.w - bh_gap)
                                         edje_object_signal_emit(en->sel_handler_end, "elm,state,top,reversed", "elm");
                                      else
                                         edje_object_signal_emit(en->sel_handler_end, "elm,state,top", "elm");
                                   }
                                 else
                                   {
                                      evas_object_move(en->sel_handler_end, nx, ny);

                                      if (nx >= en->layout_region.w - bh_gap)
                                         edje_object_signal_emit(en->sel_handler_end, "elm,state,bottom,reversed", "elm");
                                      else
                                         edje_object_signal_emit(en->sel_handler_end, "elm,state,bottom", "elm");
                                   }

                                 evas_object_show(en->sel_handler_end);

                                 evas_object_show(en->sel_handler_edge_end);
                              }
                         }
                    }
               }
             // TIZEN ONLY - END

             *(&(sel->rect)) = *r;
             range = eina_list_remove_list(range, range);
             free(r);
          }
     }
   else
     {
        while (range)
          {
             free(range->data);
             range = eina_list_remove_list(range, range);
          }
     }
}

static void
_edje_anchor_mouse_down_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Anchor *an = data;
   Evas_Event_Mouse_Down *ev = event_info;
   Edje_Real_Part *rp = an->en->rp;
   char *buf, *n;
   size_t len;
   int ignored;
   Entry *en;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (((rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT) ||
        (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_BLOCK_HANDLE)) &&
       (en->select_allow))
     return;
   ignored = rp->part->ignore_flags & ev->event_flags;
   if ((!ev->event_flags) || (!ignored))
     {
        n = an->name;
        if (!n) n = "";
        len = 200 + strlen(n);
        buf = alloca(len);
        if (ev->flags & EVAS_BUTTON_TRIPLE_CLICK)
          snprintf(buf, len, "anchor,mouse,down,%i,%s,triple", ev->button, n);
        else if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
          snprintf(buf, len, "anchor,mouse,down,%i,%s,double", ev->button, n);
        else
          snprintf(buf, len, "anchor,mouse,down,%i,%s", ev->button, n);
        _edje_emit(rp->edje, buf, rp->part->name);
     }
}

static void
_edje_anchor_mouse_up_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Anchor *an = data;
   Evas_Event_Mouse_Up *ev = event_info;
   Edje_Real_Part *rp = an->en->rp;
   char *buf, *n;
   size_t len;
   int ignored;
   Entry *en;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   ignored = rp->part->ignore_flags & ev->event_flags;
   if (((rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT) ||
        (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_BLOCK_HANDLE)) &&
       (en->select_allow))
     return;
   n = an->name;
   if (!n) n = "";
   len = 200 + strlen(n);
   buf = alloca(len);
   if ((!ev->event_flags) || (!ignored))
     {
        snprintf(buf, len, "anchor,mouse,up,%i,%s", ev->button, n);
        _edje_emit(rp->edje, buf, rp->part->name);
     }
   if ((rp->still_in) && (rp->clicked_button == ev->button) && (!ignored))
     {
        snprintf(buf, len, "anchor,mouse,clicked,%i,%s", ev->button, n);
        _edje_emit(rp->edje, buf, rp->part->name);
     }
}

static void
_edje_anchor_mouse_move_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Anchor *an = data;
   Evas_Event_Mouse_Move *ev = event_info;
   Edje_Real_Part *rp = an->en->rp;
   char *buf, *n;
   size_t len;
   int ignored;
   Entry *en;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (((rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT) ||
        (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_BLOCK_HANDLE)) &&
       (en->select_allow))
     return;
   ignored = rp->part->ignore_flags & ev->event_flags;
   if ((!ev->event_flags) || (!ignored))
     {
        n = an->name;
        if (!n) n = "";
        len = 200 + strlen(n);
        buf = alloca(len);
        snprintf(buf, len, "anchor,mouse,move,%s", n);
        _edje_emit(rp->edje, buf, rp->part->name);
     }
}

static void
_edje_anchor_mouse_in_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Anchor *an = data;
   Evas_Event_Mouse_In *ev = event_info;
   Edje_Real_Part *rp = an->en->rp;
   char *buf, *n;
   size_t len;
   int ignored;

   ignored = rp->part->ignore_flags & ev->event_flags;
   if ((!ev->event_flags) || (!ignored))
     {
        n = an->name;
        if (!n) n = "";
        len = 200 + strlen(n);
        buf = alloca(len);
        snprintf(buf, len, "anchor,mouse,in,%s", n);
        _edje_emit(rp->edje, buf, rp->part->name);
     }
}

static void
_edje_anchor_mouse_out_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Anchor *an = data;
   Evas_Event_Mouse_Out *ev = event_info;
   Edje_Real_Part *rp = an->en->rp;
   char *buf, *n;
   size_t len;
   int ignored;

   ignored = rp->part->ignore_flags & ev->event_flags;
   if ((!ev->event_flags) || (!ignored))
     {
        n = an->name;
        if (!n) n = "";
        len = 200 + strlen(n);
        buf = alloca(len);
        snprintf(buf, len, "anchor,mouse,out,%s", n);
        _edje_emit(rp->edje, buf, rp->part->name);
     }
}

static void
_anchors_update(Evas_Textblock_Cursor *c __UNUSED__, Evas_Object *o, Entry *en)
{
   Eina_List *l, *ll, *range = NULL;
   Evas_Coord x, y, w, h;
   Evas_Object *smart, *clip;
   Sel *sel;
   Anchor *an;

   smart = evas_object_smart_parent_get(o);
   clip = evas_object_clip_get(o);
   x = y = w = h = -1;
   evas_object_geometry_get(o, &x, &y, &w, &h);
   EINA_LIST_FOREACH(en->anchors, l, an)
     {
        // for item anchors
        if (an->item)
          {
             Evas_Object *ob;

             if (!an->sel)
               {
                  while (an->sel)
                    {
                       sel = an->sel->data;
                       if (sel->obj_bg) evas_object_del(sel->obj_bg);
                       if (sel->obj_fg) evas_object_del(sel->obj_fg);
                       if (sel->obj) evas_object_del(sel->obj);
                       free(sel);
                       an->sel = eina_list_remove_list(an->sel, an->sel);
                    }

                  sel = calloc(1, sizeof(Sel));
                  an->sel = eina_list_append(an->sel, sel);

                  if (en->rp->edje->item_provider.func)
                    {
                       ob = en->rp->edje->item_provider.func
                         (en->rp->edje->item_provider.data, smart,
                             en->rp->part->name, an->name);
                       evas_object_smart_member_add(ob, smart);
                       evas_object_stack_above(ob, o);
                       evas_object_clip_set(ob, clip);
                       evas_object_pass_events_set(ob, EINA_TRUE);
                       evas_object_show(ob);
                       sel->obj = ob;
                    }
               }
          }
        // for link anchors
        else
          {
             range =
               evas_textblock_cursor_range_geometry_get(an->start, an->end);
             if (eina_list_count(range) != eina_list_count(an->sel))
               {
                  while (an->sel)
                    {
                       sel = an->sel->data;
                       if (sel->obj_bg) evas_object_del(sel->obj_bg);
                       if (sel->obj_fg) evas_object_del(sel->obj_fg);
                       if (sel->obj) evas_object_del(sel->obj);
                       free(sel);
                       an->sel = eina_list_remove_list(an->sel, an->sel);
                    }
                  for (ll = range; ll; ll = eina_list_next(ll))
                    {
                       Evas_Object *ob;

                       sel = calloc(1, sizeof(Sel));
                       an->sel = eina_list_append(an->sel, sel);
                       ob = edje_object_add(en->rp->edje->base.evas);
                       edje_object_file_set(ob, en->rp->edje->path, en->rp->part->source5);
                       evas_object_smart_member_add(ob, smart);
                       evas_object_stack_below(ob, o);
                       evas_object_clip_set(ob, clip);
                       evas_object_pass_events_set(ob, EINA_TRUE);
                       evas_object_show(ob);
                       sel->obj_bg = ob;
                       _edje_subobj_register(en->rp->edje, sel->obj_bg);

                       ob = edje_object_add(en->rp->edje->base.evas);
                       edje_object_file_set(ob, en->rp->edje->path, en->rp->part->source6);
                       evas_object_smart_member_add(ob, smart);
                       evas_object_stack_above(ob, o);
                       evas_object_clip_set(ob, clip);
                       evas_object_pass_events_set(ob, EINA_TRUE);
                       evas_object_show(ob);
                       sel->obj_fg = ob;
                       _edje_subobj_register(en->rp->edje, sel->obj_fg);

                       ob = evas_object_rectangle_add(en->rp->edje->base.evas);
                       evas_object_color_set(ob, 0, 0, 0, 0);
                       evas_object_smart_member_add(ob, smart);
                       evas_object_stack_above(ob, o);
                       evas_object_clip_set(ob, clip);
                       evas_object_repeat_events_set(ob, EINA_TRUE);
                       evas_object_event_callback_add(ob, EVAS_CALLBACK_MOUSE_DOWN, _edje_anchor_mouse_down_cb, an);
                       evas_object_event_callback_add(ob, EVAS_CALLBACK_MOUSE_UP, _edje_anchor_mouse_up_cb, an);
                       evas_object_event_callback_add(ob, EVAS_CALLBACK_MOUSE_MOVE, _edje_anchor_mouse_move_cb, an);
                       evas_object_event_callback_add(ob, EVAS_CALLBACK_MOUSE_IN, _edje_anchor_mouse_in_cb, an);
                       evas_object_event_callback_add(ob, EVAS_CALLBACK_MOUSE_OUT, _edje_anchor_mouse_out_cb, an);
                       evas_object_show(ob);
                       sel->obj = ob;
                    }
               }
          }
        EINA_LIST_FOREACH(an->sel, ll, sel)
          {
             if (an->item)
               {
                  Evas_Coord cx, cy, cw, ch;

                  if (!evas_textblock_cursor_format_item_geometry_get
                      (an->start, &cx, &cy, &cw, &ch))
                    continue;
                  evas_object_move(sel->obj, x + cx, y + cy);
                  evas_object_resize(sel->obj, cw, ch);
               }
             else
               {
                  Evas_Textblock_Rectangle *r;

                  r = range->data;
                  *(&(sel->rect)) = *r;
                  if (sel->obj_bg)
                    {
                       evas_object_move(sel->obj_bg, x + r->x, y + r->y);
                       evas_object_resize(sel->obj_bg, r->w, r->h);
                    }
                  if (sel->obj_fg)
                    {
                       evas_object_move(sel->obj_fg, x + r->x, y + r->y);
                       evas_object_resize(sel->obj_fg, r->w, r->h);
                    }
                  if (sel->obj)
                    {
                       evas_object_move(sel->obj, x + r->x, y + r->y);
                       evas_object_resize(sel->obj, r->w, r->h);
                    }
                  range = eina_list_remove_list(range, range);
                  free(r);
               }
          }
     }
}

static void
_anchors_clear(Evas_Textblock_Cursor *c __UNUSED__, Evas_Object *o __UNUSED__, Entry *en)
{
   while (en->anchorlist)
     {
        free(en->anchorlist->data);
        en->anchorlist = eina_list_remove_list(en->anchorlist, en->anchorlist);
     }
   while (en->itemlist)
     {
        free(en->itemlist->data);
        en->itemlist = eina_list_remove_list(en->itemlist, en->itemlist);
     }
   while (en->anchors)
     {
        Anchor *an = en->anchors->data;

        evas_textblock_cursor_free(an->start);
        evas_textblock_cursor_free(an->end);
        while (an->sel)
          {
             Sel *sel = an->sel->data;
             if (sel->obj_bg) evas_object_del(sel->obj_bg);
             if (sel->obj_fg) evas_object_del(sel->obj_fg);
             if (sel->obj) evas_object_del(sel->obj);
             free(sel);
             an->sel = eina_list_remove_list(an->sel, an->sel);
          }
        free(an->name);
        free(an);
        en->anchors = eina_list_remove_list(en->anchors, en->anchors);
     }
}

static void
_anchors_get(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   const Eina_List *anchors_a, *anchors_item;
   Anchor *an = NULL;
   _anchors_clear(c, o, en);

   anchors_a = evas_textblock_node_format_list_get(o, "a");
   anchors_item = evas_textblock_node_format_list_get(o, "item");

   if (anchors_a)
     {
        const Evas_Object_Textblock_Node_Format *node;
        const Eina_List *itr;
        EINA_LIST_FOREACH(anchors_a, itr, node)
          {
             const char *s = evas_textblock_node_format_text_get(node);
             char *p;
             an = calloc(1, sizeof(Anchor));
             if (!an)
               break;

             an->en = en;
             p = strstr(s, "href=");
             if (p)
               {
                  an->name = strdup(p + 5);
               }
             en->anchors = eina_list_append(en->anchors, an);
             an->start = evas_object_textblock_cursor_new(o);
             an->end = evas_object_textblock_cursor_new(o);
             evas_textblock_cursor_at_format_set(an->start, node);
             evas_textblock_cursor_copy(an->start, an->end);

             /* Close the anchor, if the anchor was without text,
              * free it as well */
             node = evas_textblock_node_format_next_get(node);
             for (; node; node = evas_textblock_node_format_next_get(node))
               {
                  s = evas_textblock_node_format_text_get(node);
                  if ((!strcmp(s, "- a")) || (!strcmp(s, "-a")))
                    break;
               }

             if (node)
               {
                  evas_textblock_cursor_at_format_set(an->end, node);
               }
             else if (!evas_textblock_cursor_compare(an->start, an->end))
               {
                  if (an->name) free(an->name);
                  evas_textblock_cursor_free(an->start);
                  evas_textblock_cursor_free(an->end);
                  en->anchors = eina_list_remove(en->anchors, an);
                  free(an);
               }
             an = NULL;
          }
     }

   if (anchors_item)
     {
        const Evas_Object_Textblock_Node_Format *node;
        const Eina_List *itr;
        EINA_LIST_FOREACH(anchors_item, itr, node)
          {
             const char *s = evas_textblock_node_format_text_get(node);
             char *p;
             an = calloc(1, sizeof(Anchor));
             if (!an)
               break;

             an->en = en;
             an->item = 1;
             p = strstr(s, "href=");
             if (p)
               {
                  an->name = strdup(p + 5);
               }
             en->anchors = eina_list_append(en->anchors, an);
             an->start = evas_object_textblock_cursor_new(o);
             an->end = evas_object_textblock_cursor_new(o);
             evas_textblock_cursor_at_format_set(an->start, node);
             evas_textblock_cursor_copy(an->start, an->end);
             /* Although needed in textblock, don't bother with finding the end
              * here cause it doesn't really matter. */
          }
     }
}

static void
_free_entry_change_info(void *_info)
{
   Edje_Entry_Change_Info *info = (Edje_Entry_Change_Info *) _info;
   if (info->insert)
     {
        eina_stringshare_del(info->change.insert.content);
     }
   else
     {
        eina_stringshare_del(info->change.del.content);
     }
   free(info);
}

static void
_range_del_emit(Edje *ed, Evas_Textblock_Cursor *c __UNUSED__, Evas_Object *o __UNUSED__, Entry *en)
{
   size_t start, end;
   char *tmp;
   Edje_Entry_Change_Info *info;

   start = evas_textblock_cursor_pos_get(en->sel_start);
   end = evas_textblock_cursor_pos_get(en->sel_end);
   if (start == end)
      goto noop;

   info = calloc(1, sizeof(*info));
   info->insert = EINA_FALSE;
   info->change.del.start = start;
   info->change.del.end = end;

   tmp = evas_textblock_cursor_range_text_get(en->sel_start, en->sel_end, EVAS_TEXTBLOCK_TEXT_MARKUP);
   info->change.del.content = eina_stringshare_add(tmp);
   if (tmp) free(tmp);
   evas_textblock_cursor_range_delete(en->sel_start, en->sel_end);
   _edje_emit(ed, "entry,changed", en->rp->part->name);
   _edje_emit_full(ed, "entry,changed,user", en->rp->part->name, info,
                   _free_entry_change_info);
noop:
   _sel_clear(en->cursor, en->rp->object, en);
}

static void
_range_del(Evas_Textblock_Cursor *c __UNUSED__, Evas_Object *o __UNUSED__, Entry *en)
{
   evas_textblock_cursor_range_delete(en->sel_start, en->sel_end);
   _sel_clear(en->cursor, en->rp->object, en);
}

static void
_delete_emit(Edje *ed, Evas_Textblock_Cursor *c, Entry *en, size_t pos,
             Eina_Bool backspace)
{
   if (!evas_textblock_cursor_char_next(c))
     {
        return;
     }
   evas_textblock_cursor_char_prev(c);

   Edje_Entry_Change_Info *info = calloc(1, sizeof(*info));
   char *tmp = evas_textblock_cursor_content_get(c);

   info->insert = EINA_FALSE;
   if (backspace)
     {
        info->change.del.start = pos - 1;
        info->change.del.end = pos;
     }
   else
     {
        info->change.del.start = pos + 1;
        info->change.del.end = pos;
     }

   info->change.del.content = eina_stringshare_add(tmp);
   if (tmp) free(tmp);

   evas_textblock_cursor_char_delete(c);
   _edje_emit(ed, "entry,changed", en->rp->part->name);
   _edje_emit_full(ed, "entry,changed,user", en->rp->part->name,
                   info, _free_entry_change_info);
}

static void
_edje_entry_hide_visible_password(Edje_Real_Part *rp)
{
   const Evas_Object_Textblock_Node_Format *node;
   node = evas_textblock_node_format_first_get(rp->object);
   for (; node; node = evas_textblock_node_format_next_get(node))
     {
        const char *text = evas_textblock_node_format_text_get(node);
        if (text)
          {
             if (!strcmp(text, "+ password=off"))
               {
                  evas_textblock_node_format_remove_pair(rp->object,
                                                         (Evas_Object_Textblock_Node_Format *) node);
                  break;
               }
          }
     }
   _edje_entry_real_part_configure(rp);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);
}

static Eina_Bool
_password_timer_cb(void *data)
{
   Entry *en = (Entry *)data;
   _edje_entry_hide_visible_password(en->rp);
   en->pw_timer = NULL;
   return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool
_is_modifier(const char *key)
{
   if ((!strncmp(key, "Shift", 5)) ||
       (!strncmp(key, "Control", 7)) ||
       (!strncmp(key, "Alt", 3)) ||
       (!strncmp(key, "Meta", 4)) ||
       (!strncmp(key, "Super", 5)) ||
       (!strncmp(key, "Hyper", 5)) ||
       (!strcmp(key, "Scroll_Lock")) ||
       (!strcmp(key, "Num_Lock")) ||
       (!strcmp(key, "Caps_Lock")))
     return EINA_TRUE;
   return EINA_FALSE;
}

static void
_compose_seq_reset(Entry *en)
{
   char *str;
   
   EINA_LIST_FREE(en->seq, str) eina_stringshare_del(str);
   en->composing = EINA_FALSE;
}

static void
_edje_key_down_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Edje *ed = data;
   Evas_Event_Key_Down *ev = event_info;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en;
   Eina_Bool control, alt, shift;
   Eina_Bool multiline;
   Eina_Bool cursor_changed;
   int old_cur_pos;
   if (!rp) return;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_EDITABLE))
     return;
   if (!ev->keyname) return;

   // TIZEN ONLY - START
   if (en->cursor_handler)
      evas_object_hide(en->cursor_handler);
   // TIZEN ONLY - END

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        Ecore_IMF_Event_Key_Down ecore_ev;
        Eina_Bool filter_ret;
        ecore_imf_evas_event_key_down_wrap(ev, &ecore_ev);
        if (!en->composing)
          {
             filter_ret = ecore_imf_context_filter_event(en->imf_context,
                                                         ECORE_IMF_EVENT_KEY_DOWN,
                                                         (Ecore_IMF_Event *)&ecore_ev);

             if (en->have_preedit)
               {
                  if (!strcmp(ev->keyname, "Down") ||
                      (!strcmp(ev->keyname, "KP_Down") && !ev->string) ||
                      !strcmp(ev->keyname, "Up") ||
                      (!strcmp(ev->keyname, "KP_Up") && !ev->string) ||
                      !strcmp(ev->keyname, "Next") ||
                      (!strcmp(ev->keyname, "KP_Next") && !ev->string) ||
                      !strcmp(ev->keyname, "Prior") ||
                      (!strcmp(ev->keyname, "KP_Prior") && !ev->string) ||
                      !strcmp(ev->keyname, "Home") ||
                      (!strcmp(ev->keyname, "KP_Home") && !ev->string) ||
                      !strcmp(ev->keyname, "End") ||
                      (!strcmp(ev->keyname, "KP_End") && !ev->string))
                    ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
               }

             if (filter_ret)
               return;
          }
     }
#endif

   if ((!strcmp(ev->keyname, "Escape")) ||
       (!strcmp(ev->keyname, "Return")) || (!strcmp(ev->keyname, "KP_Enter")))
     _edje_entry_imf_context_reset(rp);

   old_cur_pos = evas_textblock_cursor_pos_get(en->cursor);

   control = evas_key_modifier_is_set(ev->modifiers, "Control");
   alt = evas_key_modifier_is_set(ev->modifiers, "Alt");
   shift = evas_key_modifier_is_set(ev->modifiers, "Shift");
   multiline = rp->part->multiline;
   cursor_changed = EINA_FALSE;
   if (!strcmp(ev->keyname, "Escape"))
     {
        _compose_seq_reset(en);
        // dead keys here. Escape for now (should emit these)
        _edje_emit(ed, "entry,key,escape", rp->part->name);
        // TIZEN ONLY (20130327) : This code is doing nothing for entry now.
        //                         And for Tizen "ESC" is used for another purpose.
        //ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->keyname, "Up") ||
            (!strcmp(ev->keyname, "KP_Up") && !ev->string))
     {
        _compose_seq_reset(en);
        if (multiline)
          {
             if (en->select_allow)
               {
                  if (shift) _sel_start(en->cursor, rp->object, en);
                  else _sel_clear(en->cursor, rp->object, en);
               }
             _curs_up(en->cursor, rp->object, en);
             if (en->select_allow)
               {
                  if (shift) _sel_extend(en->cursor, rp->object, en);
                  else _sel_clear(en->cursor, rp->object, en);
               }
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        _edje_emit(ed, "entry,key,up", rp->part->name);
        _edje_emit(rp->edje, "cursor,changed,manual", rp->part->name);
     }
   else if (!strcmp(ev->keyname, "Down") ||
            (!strcmp(ev->keyname, "KP_Down") && !ev->string))
     {
        _compose_seq_reset(en);
        if (multiline)
          {
             if (en->select_allow)
               {
                  if (shift) _sel_start(en->cursor, rp->object, en);
                  else _sel_clear(en->cursor, rp->object, en);
               }
             _curs_down(en->cursor, rp->object, en);
             if (en->select_allow)
               {
                  if (shift) _sel_extend(en->cursor, rp->object, en);
                  else _sel_clear(en->cursor, rp->object, en);
               }
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        _edje_emit(ed, "entry,key,down", rp->part->name);
        _edje_emit(rp->edje, "cursor,changed,manual", rp->part->name);
     }
   else if (!strcmp(ev->keyname, "Left") ||
            (!strcmp(ev->keyname, "KP_Left") && !ev->string))
     {
        _compose_seq_reset(en);
        if (en->select_allow)
          {
             if (shift) _sel_start(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        evas_textblock_cursor_char_prev(en->cursor);
        /* If control is pressed, go to the start of the word */
        if (control) evas_textblock_cursor_word_start(en->cursor);
        if (en->select_allow)
          {
             if (shift) _sel_extend(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        _edje_emit(ed, "entry,key,left", rp->part->name);
        _edje_emit(rp->edje, "cursor,changed,manual", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->keyname, "Right") ||
            (!strcmp(ev->keyname, "KP_Right") && !ev->string))
     {
        _compose_seq_reset(en);
        if (en->select_allow)
          {
             if (shift) _sel_start(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        /* If control is pressed, go to the end of the word */
        if (control) evas_textblock_cursor_word_end(en->cursor);
        evas_textblock_cursor_char_next(en->cursor);
        if (en->select_allow)
          {
             if (shift) _sel_extend(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        _edje_emit(ed, "entry,key,right", rp->part->name);
        _edje_emit(rp->edje, "cursor,changed,manual", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->keyname, "BackSpace"))
     {
        _compose_seq_reset(en);
        if (control && !en->have_selection)
          {
             // del to start of previous word
             _sel_start(en->cursor, rp->object, en);

             evas_textblock_cursor_char_prev(en->cursor);
             evas_textblock_cursor_word_start(en->cursor);

             _sel_preextend(en->cursor, rp->object, en);

             _range_del_emit(ed, en->cursor, rp->object, en);
          }
        else if ((alt) && (shift))
          {
             // undo last action
          }
        else
          {
             if (en->have_selection)
               {
                  _range_del_emit(ed, en->cursor, rp->object, en);
               }
             else
               {
                  if (evas_textblock_cursor_char_prev(en->cursor))
                    {
                       _delete_emit(ed, en->cursor, en, old_cur_pos, EINA_TRUE);
                    }
               }
          }
        _sel_clear(en->cursor, rp->object, en);
        _anchors_get(en->cursor, rp->object, en);
        _edje_emit(ed, "entry,key,backspace", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->keyname, "Delete") ||
            (!strcmp(ev->keyname, "KP_Delete") && !ev->string))
     {
        _compose_seq_reset(en);
        if (control)
          {
             // del to end of next word
             _sel_start(en->cursor, rp->object, en);

             evas_textblock_cursor_word_end(en->cursor);
             evas_textblock_cursor_char_next(en->cursor);

             _sel_extend(en->cursor, rp->object, en);

             _range_del_emit(ed, en->cursor, rp->object, en);
          }
        else if (shift)
          {
             // cut
          }
        else
          {
             if (en->have_selection)
               {
                  _range_del_emit(ed, en->cursor, rp->object, en);
               }
             else
               {
                  _delete_emit(ed, en->cursor, en, old_cur_pos, EINA_FALSE);
               }
          }
        _sel_clear(en->cursor, rp->object, en);
        _anchors_get(en->cursor, rp->object, en);
        _edje_emit(ed, "entry,key,delete", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->keyname, "Home") ||
            ((!strcmp(ev->keyname, "KP_Home")) && !ev->string))
     {
        _compose_seq_reset(en);
        if (en->select_allow)
          {
             if (shift) _sel_start(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        if ((control) && (multiline))
          _curs_start(en->cursor, rp->object, en);
        else
          _curs_lin_start(en->cursor, rp->object, en);
        if (en->select_allow)
          {
             if (shift) _sel_extend(en->cursor, rp->object, en);
          }
        _edje_emit(ed, "entry,key,home", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->keyname, "End") ||
            ((!strcmp(ev->keyname, "KP_End")) && !ev->string))
     {
        _compose_seq_reset(en);
        if (en->select_allow)
          {
             if (shift) _sel_start(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        if ((control) && (multiline))
          _curs_end(en->cursor, rp->object, en);
        else
          _curs_lin_end(en->cursor, rp->object, en);
        if (en->select_allow)
          {
             if (shift) _sel_extend(en->cursor, rp->object, en);
          }
        _edje_emit(ed, "entry,key,end", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && (!shift) && (!strcmp(ev->keyname, "v")))
     {
        _compose_seq_reset(en);
        _edje_emit(ed, "entry,paste,request", rp->part->name);
        _edje_emit(ed, "entry,paste,request,3", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && (!strcmp(ev->keyname, "a")))
     {
        _compose_seq_reset(en);
        if (shift)
          {
             _edje_emit(ed, "entry,selection,none,request", rp->part->name);
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        else
          {
             if (en->select_allow) // TIZEN ONLY
                _edje_emit(ed, "entry,selection,all,request", rp->part->name);
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
     }
   else if ((control) && (((!shift) && !strcmp(ev->keyname, "c")) || !strcmp(ev->keyname, "Insert")))
     {
        _compose_seq_reset(en);
        _edje_emit(ed, "entry,copy,notify", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && (!shift) && ((!strcmp(ev->keyname, "x") || (!strcmp(ev->keyname, "m")))))
     {
        _compose_seq_reset(en);
        _edje_emit(ed, "entry,cut,notify", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && (!strcmp(ev->keyname, "z")))
     {
        _compose_seq_reset(en);
        if (shift)
          {
             // redo
             _edje_emit(ed, "entry,redo,request", rp->part->name);
          }
        else
          {
             // undo
             _edje_emit(ed, "entry,undo,request", rp->part->name);
          }
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && (!shift) && (!strcmp(ev->keyname, "y")))
     {
        _compose_seq_reset(en);
        // redo
        _edje_emit(ed, "entry,redo,request", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && (!shift) && (!strcmp(ev->keyname, "w")))
     {
        _compose_seq_reset(en);
        _sel_clear(en->cursor, rp->object, en);
        // select current word?
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->keyname, "Tab"))
     {
        _compose_seq_reset(en);
        if (multiline)
          {
             if (shift)
               {
                  // remove a tab
               }
             else
               {
                  Edje_Entry_Change_Info *info = calloc(1, sizeof(*info));
                  info->insert = EINA_TRUE;
                  info->change.insert.plain_length = 1;

                  if (en->have_selection)
                    {
                       _range_del_emit(ed, en->cursor, rp->object, en);
                       info->merge = EINA_TRUE;
                    }
                  info->change.insert.pos =
                     evas_textblock_cursor_pos_get(en->cursor);
                  info->change.insert.content = eina_stringshare_add("<tab/>");
                  //yy
//                  evas_textblock_cursor_format_prepend(en->cursor, "tab");
                  _text_filter_format_prepend(en, en->cursor, "tab");
                  _anchors_get(en->cursor, rp->object, en);
                  _edje_emit(ed, "entry,changed", rp->part->name);
                  _edje_emit_full(ed, "entry,changed,user", rp->part->name,
                                  info, _free_entry_change_info);
               }
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        _edje_emit(ed, "entry,key,tab", rp->part->name);
     }
   else if ((!strcmp(ev->keyname, "ISO_Left_Tab")) && (multiline))
     {
        _compose_seq_reset(en);
        // remove a tab
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->keyname, "Prior") ||
            (!strcmp(ev->keyname, "KP_Prior") && !ev->string))
     {
        _compose_seq_reset(en);
        if (en->select_allow)
          {
             if (shift) _sel_start(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        _curs_jump_line_by(en->cursor, rp->object, en, -10);
        if (en->select_allow)
          {
             if (shift) _sel_extend(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        _edje_emit(ed, "entry,key,pgup", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->keyname, "Next") ||
            (!strcmp(ev->keyname, "KP_Next") && !ev->string))
     {
        _compose_seq_reset(en);
        if (en->select_allow)
          {
             if (shift) _sel_start(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        _curs_jump_line_by(en->cursor, rp->object, en, 10);
        if (en->select_allow)
          {
             if (shift) _sel_extend(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        _edje_emit(ed, "entry,key,pgdn", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((!strcmp(ev->keyname, "Return")) || (!strcmp(ev->keyname, "KP_Enter")))
     {
        _compose_seq_reset(en);
        if (multiline)
          {
             Edje_Entry_Change_Info *info = calloc(1, sizeof(*info));
             info->insert = EINA_TRUE;
             info->change.insert.plain_length = 1;
             if (en->have_selection)
               {
                  _range_del_emit(ed, en->cursor, rp->object, en);
                  info->merge = EINA_TRUE;
               }

             info->change.insert.pos =
                evas_textblock_cursor_pos_get(en->cursor);
             if (shift ||
                 evas_object_textblock_legacy_newline_get(rp->object))
               {
                  //yy
//                  evas_textblock_cursor_format_prepend(en->cursor, "br");
                  _text_filter_format_prepend(en, en->cursor, "br");
                  info->change.insert.content = eina_stringshare_add("<br/>");
               }
             else
               {
                  //yy
//                  evas_textblock_cursor_format_prepend(en->cursor, "ps");
                  _text_filter_format_prepend(en, en->cursor, "ps");
                  info->change.insert.content = eina_stringshare_add("<ps/>");
               }
             _anchors_get(en->cursor, rp->object, en);
             _edje_emit(ed, "entry,changed", rp->part->name);
             _edje_emit_full(ed, "entry,changed,user", rp->part->name,
                             info, _free_entry_change_info);
             _edje_emit(ed, "cursor,changed", rp->part->name);
             cursor_changed = EINA_TRUE;
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        _edje_emit(ed, "entry,key,enter", rp->part->name);
     }
   else
     {
        char *compres = NULL, *string = (char *)ev->string;
        Eina_Bool free_string = EINA_FALSE;
        Ecore_Compose_State state;
        
        if (!en->composing)
          {
             _compose_seq_reset(en);
             en->seq = eina_list_append(en->seq, eina_stringshare_add(ev->key));
             state = ecore_compose_get(en->seq, &compres);
             if (state == ECORE_COMPOSE_MIDDLE) en->composing = EINA_TRUE;
             else en->composing = EINA_FALSE;
             if (!en->composing) _compose_seq_reset(en);
             else goto end;
          }
        else
          {
             if (_is_modifier(ev->key)) goto end;
             en->seq = eina_list_append(en->seq, eina_stringshare_add(ev->key));
             state = ecore_compose_get(en->seq, &compres);
             if (state == ECORE_COMPOSE_NONE) _compose_seq_reset(en);
             else if (state == ECORE_COMPOSE_DONE)
               {
                  _compose_seq_reset(en);
                  if (compres)
                    {
                       string = compres;
                       free_string = EINA_TRUE;
                    }
               }
             else goto end;
          }
        if (string)
          {
             Edje_Entry_Change_Info *info = calloc(1, sizeof(*info));
             info->insert = EINA_TRUE;
             info->change.insert.plain_length = 1;
             info->change.insert.content = eina_stringshare_add(string);

             if (en->have_selection)
               {
                  _range_del_emit(ed, en->cursor, rp->object, en);
                  info->merge = EINA_TRUE;
               }

             info->change.insert.pos =
                evas_textblock_cursor_pos_get(en->cursor);
             // if PASSWORD_SHOW_LAST mode, appending text with password=off tag
             if ((rp->part->entry_mode == EDJE_ENTRY_EDIT_MODE_PASSWORD) &&
                 _edje_password_show_last)
               {
                  _edje_entry_hide_visible_password(en->rp);
                  _text_filter_format_prepend(en, en->cursor, "+ password=off");
                  _text_filter_text_prepend(en, en->cursor, string);
                  _text_filter_format_prepend(en, en->cursor, "- password");
                  if (en->pw_timer)
                    {
                       ecore_timer_del(en->pw_timer);
                       en->pw_timer = NULL;
                    }
                  en->pw_timer = ecore_timer_add(TO_DOUBLE(_edje_password_show_last_timeout),
                                                 _password_timer_cb, en);
               }
             else
               _text_filter_text_prepend(en, en->cursor, string);
             _anchors_get(en->cursor, rp->object, en);
             _edje_emit(ed, "entry,changed", rp->part->name);
             _edje_emit_full(ed, "entry,changed,user", rp->part->name,
                             info, _free_entry_change_info);
             _edje_emit(ed, "cursor,changed", rp->part->name);
             cursor_changed = EINA_TRUE;
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        if (free_string) free(string);
     }
end:
   if (!cursor_changed && (old_cur_pos != evas_textblock_cursor_pos_get(en->cursor)))
     _edje_emit(ed, "cursor,changed", rp->part->name);

   _edje_entry_imf_cursor_info_set(en);
   _edje_entry_real_part_configure(rp);
}

static void
_edje_key_up_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Edje *ed = data;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en;

   if (!rp) return;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_EDITABLE))
     return;

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        Evas_Event_Key_Up *ev = event_info;
        Ecore_IMF_Event_Key_Up ecore_ev;

        ecore_imf_evas_event_key_up_wrap(ev, &ecore_ev);
        if (ecore_imf_context_filter_event(en->imf_context,
                                           ECORE_IMF_EVENT_KEY_UP,
                                           (Ecore_IMF_Event *)&ecore_ev))
          return;
     }
#else
   (void) event_info;
#endif
}

// TIZEN ONLY - START
Eina_Bool
_edje_entry_select_word(Edje_Real_Part *rp)
{
   Entry *en;
   if (!rp) return EINA_FALSE;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;

   en = rp->typedata.text->entry_data;
   if (!en) return EINA_FALSE;

   if (en->cursor_fg) evas_object_hide(en->cursor_fg);
   if (en->cursor_bg) evas_object_hide(en->cursor_bg);

   evas_textblock_cursor_word_start(en->cursor);

   if (evas_textblock_cursor_eol_get(en->cursor))
     {
        evas_textblock_cursor_char_prev(en->cursor);
        evas_textblock_cursor_word_start(en->cursor);
     }

   _sel_clear(en->cursor, rp->object, en);
   _sel_enable(en->cursor, rp->object, en);
   _sel_start(en->cursor, rp->object, en);

   evas_textblock_cursor_word_end(en->cursor);
   evas_textblock_cursor_char_next(en->cursor);

   _sel_extend(en->cursor, rp->object, en);
   _edje_entry_real_part_configure(rp);

   en->had_sel = EINA_TRUE;
   en->selecting = EINA_FALSE;

   return EINA_TRUE;
}

static Eina_Bool
_long_press_cb(void *data)
{
   Edje_Real_Part *rp = data;
   Entry *en;
   if (!rp) return ECORE_CALLBACK_CANCEL;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return ECORE_CALLBACK_CANCEL;

   en = rp->typedata.text->entry_data;
   if (!en) return ECORE_CALLBACK_CANCEL;

   if (en->long_press_timer)
     {
        ecore_timer_del(en->long_press_timer);
        en->long_press_timer = NULL;
     }

   en->long_press_state = _ENTRY_LONG_PRESSED;
   en->long_press_timer = NULL;

   _edje_emit(en->rp->edje, "long,pressed", en->rp->part->name);

   if (en->cursor_handler && !en->cursor_handler_disabled && en->focused)
     {
        edje_object_signal_emit(en->cursor_handler, "edje,cursor,handle,show", "edje");
        evas_object_show(en->cursor_handler);
     }
   return ECORE_CALLBACK_CANCEL;
}
// TIZEN ONLY - END

static void
_edje_part_move_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info __UNUSED__)
{
   Edje_Real_Part *rp = data;
   Entry *en;
   if (!rp) return;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   _edje_entry_imf_cursor_location_set(en);
}

static void
_edje_part_mouse_down_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Coord cx, cy;
   Edje_Real_Part *rp = data;
   Evas_Event_Mouse_Down *ev = event_info;
   Entry *en;
   Evas_Coord x, y, w, h;
   //   Eina_Bool multiline;
   Evas_Textblock_Cursor *tc = NULL;
   Eina_Bool dosel = EINA_FALSE;
   Eina_Bool shift;
   if (!rp) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   en->long_press_state = _ENTRY_LONG_PRESSING; // TIZEN ONLY
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return;
   if ((ev->button != 1) && (ev->button != 2)) return;

   // TIZEN ONLY - START
   en->dx = ev->canvas.x;
   en->dy = ev->canvas.y;
   // TIZEN ONLY - END

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        Ecore_IMF_Event_Mouse_Down ecore_ev;
        ecore_imf_evas_event_mouse_down_wrap(ev, &ecore_ev);
        if (ecore_imf_context_filter_event(en->imf_context,
                                           ECORE_IMF_EVENT_MOUSE_DOWN,
                                           (Ecore_IMF_Event *)&ecore_ev))
          return;
     }
#endif

   _edje_entry_imf_context_reset(rp);

   shift = evas_key_modifier_is_set(ev->modifiers, "Shift");
   en->select_mod_start = EINA_FALSE;
   en->select_mod_end = EINA_FALSE;
   if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_DEFAULT)
      dosel = EINA_TRUE;
   else if ((rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT) ||
            (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_BLOCK_HANDLE))
     {
        if (en->select_allow) dosel = EINA_TRUE;
     }
   if (ev->button == 2) dosel = EINA_FALSE;
   if (dosel)
     {
        evas_object_geometry_get(rp->object, &x, &y, &w, &h);
        cx = ev->canvas.x - x;
        cy = ev->canvas.y - y;
        if (ev->flags & EVAS_BUTTON_TRIPLE_CLICK)
          {
             if (shift)
               {
                  tc = evas_object_textblock_cursor_new(rp->object);
                  evas_textblock_cursor_copy(en->cursor, tc);
                  if (evas_textblock_cursor_compare(en->cursor, en->sel_start) < 0)
                    evas_textblock_cursor_line_char_first(en->cursor);
                  else
                    evas_textblock_cursor_line_char_last(en->cursor);
                  _sel_extend(en->cursor, rp->object, en);
               }
             else
               {
                  en->have_selection = EINA_FALSE;
                  en->selecting = EINA_FALSE;
                  _sel_clear(en->cursor, rp->object, en);
                  tc = evas_object_textblock_cursor_new(rp->object);
                  evas_textblock_cursor_copy(en->cursor, tc);
                  evas_textblock_cursor_line_char_first(en->cursor);
                  _sel_start(en->cursor, rp->object, en);
                  evas_textblock_cursor_line_char_last(en->cursor);
                  _sel_extend(en->cursor, rp->object, en);
               }
             goto end;
          }
        else if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK)
          {
             if (shift)
               {
                  tc = evas_object_textblock_cursor_new(rp->object);
                  evas_textblock_cursor_copy(en->cursor, tc);
                  if (evas_textblock_cursor_compare(en->cursor, en->sel_start) < 0)
                    evas_textblock_cursor_word_start(en->cursor);
                  else
                    {
                       evas_textblock_cursor_word_end(en->cursor);
                       evas_textblock_cursor_char_next(en->cursor);
                    }
                  _sel_extend(en->cursor, rp->object, en);
               }
             else
               {
                  en->have_selection = EINA_FALSE;
                  en->selecting = EINA_FALSE;
                  _sel_clear(en->cursor, rp->object, en);
                  tc = evas_object_textblock_cursor_new(rp->object);
                  evas_textblock_cursor_copy(en->cursor, tc);
                  evas_textblock_cursor_word_start(en->cursor);
                  _sel_start(en->cursor, rp->object, en);
                  evas_textblock_cursor_word_end(en->cursor);
                  evas_textblock_cursor_char_next(en->cursor);
                  _sel_extend(en->cursor, rp->object, en);
               }
             goto end;
          }
     }
   tc = evas_object_textblock_cursor_new(rp->object);
   evas_textblock_cursor_copy(en->cursor, tc);
   //   multiline = rp->part->multiline;
   evas_object_geometry_get(rp->object, &x, &y, &w, &h);
   cx = ev->canvas.x - x;
   cy = ev->canvas.y - y;
   if (en->rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_BLOCK_HANDLE)
     {
        evas_textblock_cursor_char_coord_set(en->cursor, cx, cy);
     }
   else
     {
        if (!evas_textblock_cursor_char_coord_set(en->cursor, cx, cy))
          {
             Evas_Coord lx, ly, lw, lh;
             int line;

             line = evas_textblock_cursor_line_coord_set(en->cursor, cy);
             if (line == -1)
               {
                  if (rp->part->multiline)
                     _curs_end(en->cursor, rp->object, en);
                  else
                    {
                       evas_textblock_cursor_paragraph_first(en->cursor);
                       evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);
                       if (!evas_textblock_cursor_char_coord_set(en->cursor, cx, ly + (lh / 2)))
                          _curs_end(en->cursor, rp->object, en);
                    }
               }
             else
               {
                  int lnum;

                  lnum = evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);
                  if (lnum < 0)
                    {
                       _curs_lin_start(en->cursor, rp->object, en);
                    }
                  else
                    {
                       if (cx <= lx)
                          _curs_lin_start(en->cursor, rp->object, en);
                       else
                          _curs_lin_end(en->cursor, rp->object, en);
                    }
               }
          }
     }
   if (dosel)
     {
        if ((en->have_selection) &&
            (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT))
          {
             if (shift)
               _sel_extend(en->cursor, rp->object, en);
             else
               {
                  Eina_List *first, *last;
                  FLOAT_T sc;

                  first = en->sel;
                  last = eina_list_last(en->sel);
                  if (first && last)
                    {
                       Evas_Textblock_Rectangle *r1, *r2;
                       Evas_Coord d, d1, d2;
                       
                       r1 = first->data;
                       r2 = last->data;
                       d = r1->x - cx;
                       d1 = d * d;
                       d = (r1->y + (r1->h / 2)) - cy;
                       d1 += d * d;
                       d = r2->x + r2->w - 1 - cx;
                       d2 = d * d;
                       d = (r2->y + (r2->h / 2)) - cy;
                       d2 += d * d;
                       sc = rp->edje->scale;
                       if (sc == ZERO) sc = _edje_scale;
                       d = (Evas_Coord)MUL(FROM_INT(20), sc); // FIXME: maxing number!
                       d = d * d;
                       if (d1 < d2)
                         {
                            if (d1 <= d)
                              {
                                 en->select_mod_start = EINA_TRUE;
                                 en->selecting = EINA_TRUE;
                              }
                         }
                       else
                         {
                            if (d2 <= d)
                              {
                                 en->select_mod_end = EINA_TRUE;
                                 en->selecting = EINA_TRUE;
                              }
                         }
                    }
               }
          }
        else
          {
             if ((en->have_selection) && (shift))
               _sel_extend(en->cursor, rp->object, en);
             else
               {
                  en->selecting = EINA_TRUE;
                  _sel_clear(en->cursor, rp->object, en);
                  if (en->select_allow)
                    {
                       _sel_start(en->cursor, rp->object, en);
                    }
               }
          }
     }
 end:
   if (evas_textblock_cursor_compare(tc, en->cursor))
     {
        _edje_emit(rp->edje, "cursor,changed", rp->part->name);
        _edje_emit(rp->edje, "cursor,changed,manual", rp->part->name);
     }
   evas_textblock_cursor_free(tc);

   // TIZEN ONLY - START
   if (en->rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_BLOCK_HANDLE)
     {
        if (en->long_press_timer) ecore_timer_del(en->long_press_timer);
        en->long_press_timer = ecore_timer_add(0.5, _long_press_cb, data); //FIXME: timer value
     }
   else
      _edje_entry_real_part_configure(rp);
   if (ev->button == 2)
     {
        _edje_emit(rp->edje, "entry,paste,request", rp->part->name);
        _edje_emit(rp->edje, "entry,paste,request,1", rp->part->name);
     }
}

static void
_edje_part_mouse_up_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Coord cx, cy;
   Edje_Real_Part *rp = data;
   Evas_Event_Mouse_Up *ev = event_info;
   Entry *en;
   Evas_Coord x, y, w, h;
   Evas_Textblock_Cursor *tc;
   if (ev->button != 1) return;
   if (!rp) return;
   //if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return; //TIZEN ONLY
   //if (ev->flags & EVAS_BUTTON_TRIPLE_CLICK) return; // TIZEN ONLY
   //if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK) return; // TIZEN ONLY
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return;

   // TIZEN ONLY - START
   if (en->long_press_timer)
     {
        ecore_timer_del(en->long_press_timer);
        en->long_press_timer = NULL;
     }

   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD)
     {
        _edje_entry_imf_cursor_info_set(en);
        return;
     }

   if (ev->flags & EVAS_BUTTON_TRIPLE_CLICK) return;
   if (ev->flags & EVAS_BUTTON_DOUBLE_CLICK) return;

   if (en->long_press_state == _ENTRY_LONG_PRESSED)
     {
        en->long_press_state = _ENTRY_LONG_PRESS_RELEASED;
        _edje_entry_imf_cursor_info_set(en);

        if (strcmp(_edje_entry_text_get(rp), "") == 0)
          {
             evas_object_hide(en->cursor_handler);
          }
        return;
     }
   en->long_press_state = _ENTRY_LONG_PRESS_RELEASED;

   if ((en->cursor_handler) && (!en->cursor_handler_disabled) && (!en->have_selection))
      evas_object_show(en->cursor_handler);
   // TIZEN ONLY - END

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        Ecore_IMF_Event_Mouse_Up ecore_ev;
        ecore_imf_evas_event_mouse_up_wrap(ev, &ecore_ev);
        if (ecore_imf_context_filter_event(en->imf_context,
                                           ECORE_IMF_EVENT_MOUSE_UP,
                                           (Ecore_IMF_Event *)&ecore_ev))
          return;
     }
#endif

   tc = evas_object_textblock_cursor_new(rp->object);
   evas_textblock_cursor_copy(en->cursor, tc);
   evas_object_geometry_get(rp->object, &x, &y, &w, &h);
   cx = ev->canvas.x - x;
   cy = ev->canvas.y - y;
   if (!evas_textblock_cursor_char_coord_set(en->cursor, cx, cy))
     {
        Evas_Coord lx, ly, lw, lh;
        int line;

        line = evas_textblock_cursor_line_coord_set(en->cursor, cy);
        if (line == -1)
          {
             if (rp->part->multiline)
               _curs_end(en->cursor, rp->object, en);
             else
               {
                  evas_textblock_cursor_paragraph_first(en->cursor);
                  evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);
                  if (!evas_textblock_cursor_char_coord_set(en->cursor, cx, ly + (lh / 2)))
                    _curs_end(en->cursor, rp->object, en);
               }
          }
        else
          {
             int lnum;

             lnum = evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);
             if (lnum < 0)
               {
                  _curs_lin_start(en->cursor, rp->object, en);
               }
             else
               {
                  if (cx <= lx)
                    _curs_lin_start(en->cursor, rp->object, en);
                  else
                    _curs_lin_end(en->cursor, rp->object, en);
               }
          }
     }
   if (rp->part->select_mode != EDJE_ENTRY_SELECTION_MODE_BLOCK_HANDLE)
     {
       if ((rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT))
         {
           if (en->select_allow)
             {
               if (en->had_sel)
                 {
                   if (en->select_mod_end)
                     _sel_extend(en->cursor, rp->object, en);
                   else if (en->select_mod_start)
                     _sel_preextend(en->cursor, rp->object, en);
                 }
               else
                 _sel_extend(en->cursor, rp->object, en);
               //evas_textblock_cursor_copy(en->cursor, en->sel_end);
             }
         }
       else
         evas_textblock_cursor_copy(en->cursor, en->sel_end);

       if (en->selecting)
         {
           if (en->have_selection)
             en->had_sel = EINA_TRUE;
           en->selecting = EINA_FALSE;
         }
     }

   if (evas_textblock_cursor_compare(tc, en->cursor))
     {
        _edje_emit(rp->edje, "cursor,changed", rp->part->name);
        _edje_emit(rp->edje, "cursor,changed,manual", rp->part->name);
     }

   _edje_entry_imf_cursor_info_set(en);

   evas_textblock_cursor_free(tc);

   _edje_entry_real_part_configure(rp);
}

static void
_edje_part_mouse_move_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Coord cx, cy;
   Edje_Real_Part *rp = data;
   Evas_Event_Mouse_Move *ev = event_info;
   Entry *en;
   Evas_Coord x, y, w, h;
   Evas_Textblock_Cursor *tc;
   if (!rp) return;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return;

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        Ecore_IMF_Event_Mouse_Move ecore_ev;
        ecore_imf_evas_event_mouse_move_wrap(ev, &ecore_ev);
        if (ecore_imf_context_filter_event(en->imf_context,
                                           ECORE_IMF_EVENT_MOUSE_MOVE,
                                           (Ecore_IMF_Event *)&ecore_ev))
          return;
     }
#endif

   // TIZEN ONLY - START
   if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_BLOCK_HANDLE)
     {
        _edje_entry_real_part_configure(rp);

        if (en->long_press_state == _ENTRY_LONG_PRESSING)
          {
             Evas_Coord dx, dy;

             dx = en->dx - ev->cur.canvas.x;
             dy = en->dy - ev->cur.canvas.y;

             /* FIXME: Magic number 40 is used to ignore finger move while detecting long press.              */
             /*        This should be replaced with the constant or variable relevant to the elm_finger_size. */
             if (((dx*dx) + (dy*dy)) > (40*40))
               {
                  en->long_press_state = _ENTRY_LONG_PRESS_RELEASED;
                  if (en->long_press_timer)
                    {
                       ecore_timer_del(en->long_press_timer);
                       en->long_press_timer = NULL;
                    }
               }
          }
        else if (en->long_press_state == _ENTRY_LONG_PRESSED)
          {
             Evas_Coord flx, fly, flh;
             Evas_Coord cnx, cny;

             tc = evas_object_textblock_cursor_new(rp->object);
             evas_textblock_cursor_copy(en->cursor, tc);
             evas_object_geometry_get(rp->object, &x, &y, &w, &h);

             cx = ev->cur.canvas.x - x;
             cy = ev->cur.canvas.y - y;
             evas_textblock_cursor_char_coord_set(en->cursor, cx, cy);

             /* handle the special cases in which pressed position is above the first line or below the last line  */
             evas_object_textblock_line_number_geometry_get(rp->object, 0, &flx, &fly, NULL, &flh);
             evas_textblock_cursor_line_geometry_get(en->cursor, &cnx, &cny, NULL, NULL);
             if ((cnx == flx) && (cny == fly))
               {
                  /* cursor is in the first line */
                  evas_textblock_cursor_char_coord_set(en->cursor, cx, fly + flh / 2);
               }
             else
               {
                  Evas_Textblock_Cursor *lc;
                  Evas_Coord llx, lly, llh;

                  lc = evas_object_textblock_cursor_new(rp->object);
                  evas_textblock_cursor_copy(en->cursor, lc);
                  evas_textblock_cursor_paragraph_last(lc);
                  evas_textblock_cursor_line_geometry_get(lc, &llx, &lly, NULL, &llh);
                  if ((cnx == llx) && (cny == lly))
                    {
                       /* cursor is in the last line */
                       evas_textblock_cursor_char_coord_set(en->cursor, cx, lly + llh / 2);
                    }
                  evas_textblock_cursor_free(lc);
               }

             if (evas_textblock_cursor_compare(tc, en->cursor))
               {
                  _edje_emit(rp->edje, "cursor,changed", rp->part->name);
               }
             evas_textblock_cursor_free(tc);

             //_edje_emit(en->rp->edje, "magnifier,changed", en->rp->part->name);
          }
     } // TIZEN ONLY - END
   else
     {
        if (en->selecting)
          {
             tc = evas_object_textblock_cursor_new(rp->object);
             evas_textblock_cursor_copy(en->cursor, tc);
             evas_object_geometry_get(rp->object, &x, &y, &w, &h);
             cx = ev->cur.canvas.x - x;
             cy = ev->cur.canvas.y - y;
             if (!evas_textblock_cursor_char_coord_set(en->cursor, cx, cy))
               {
                  Evas_Coord lx, ly, lw, lh;

                  if (evas_textblock_cursor_line_coord_set(en->cursor, cy) < 0)
                    {
                       if (rp->part->multiline)
                         _curs_end(en->cursor, rp->object, en);
                       else
                         {
                            evas_textblock_cursor_paragraph_first(en->cursor);
                            evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);
                            if (!evas_textblock_cursor_char_coord_set(en->cursor, cx, ly + (lh / 2)))
                              _curs_end(en->cursor, rp->object, en);
                         }
                    }
                  else
                    {
                       evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);
                       if (cx <= lx)
                         _curs_lin_start(en->cursor, rp->object, en);
                       else
                         _curs_lin_end(en->cursor, rp->object, en);
                    }
               }

             if ((rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT) ||
                 (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_BLOCK_HANDLE))
               {
                  if (en->select_allow)
                    {
                       if (en->had_sel)
                         {
                            if (en->select_mod_end)
                              _sel_extend(en->cursor, rp->object, en);
                            else if (en->select_mod_start)
                              _sel_preextend(en->cursor, rp->object, en);
                         }
                       else
                         _sel_extend(en->cursor, rp->object, en);
                    }
               }
             else
               {
                  _sel_extend(en->cursor, rp->object, en);
               }
             if (en->select_allow)
               {
                  if (evas_textblock_cursor_compare(en->sel_start, en->sel_end) != 0)
                    _sel_enable(en->cursor, rp->object, en);
                  if (en->have_selection)
                    _sel_update(en->cursor, rp->object, en);
               }
             if (evas_textblock_cursor_compare(tc, en->cursor))
               {
                  _edje_emit(rp->edje, "cursor,changed", rp->part->name);
                  _edje_emit(rp->edje, "cursor,changed,manual", rp->part->name);
               }
             evas_textblock_cursor_free(tc);

             _edje_entry_real_part_configure(rp);
          }
     }
}

// TIZEN ONLY - START
static Eina_Bool
_cursor_handler_click_cb(void *data)
{
   Edje_Real_Part *rp = data;
   Entry *en;
   if (!rp) return ECORE_CALLBACK_CANCEL;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return ECORE_CALLBACK_CANCEL;
   en = rp->typedata.text->entry_data;
   if (!en) return ECORE_CALLBACK_CANCEL;

   if (en->cursor_handler_click_timer)
     {
        ecore_timer_del(en->cursor_handler_click_timer);
        en->cursor_handler_click_timer = NULL;
     }
   en->cursor_handler_click_timer = NULL;

   return ECORE_CALLBACK_CANCEL;
}

static void
_edje_entry_cursor_handler_mouse_down_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info)
{
   Entry *en;
   Edje_Real_Part *rp = data;
   Evas_Event_Mouse_Down *ev = event_info;
   Evas_Coord ox, oy, ow, oh;
   Evas_Coord lx, ly, lw, lh;

   if (!rp) return;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;

   if (ev->button != 1) return;

   en->rx = en->sx = ev->canvas.x;
   en->ry = en->sy = ev->canvas.y;

   evas_object_geometry_get(obj, &ox, &oy, NULL, NULL);
   edje_object_size_min_calc(obj, &ow, &oh);
   evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);

   en->ox = ox + ow/2 - en->rx;
   en->oy = oy - en->ry - lh/2;

   _edje_emit(rp->edje, "cursor,handler,move,start", en->rp->part->name);

   if (en->cursor_handler_click_timer) ecore_timer_del(en->cursor_handler_click_timer);
   en->cursor_handler_click_timer = ecore_timer_add(0.1, _cursor_handler_click_cb, data); //FIXME: timer value
}

static void
_edje_entry_cursor_handler_mouse_up_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Entry *en;
   Edje_Real_Part *rp = data;
   Evas_Event_Mouse_Up *ev = event_info;

   if (!rp) return;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;

   if (ev->button != 1) return;

   if (en->cursor_handler_click_timer)
     {
        ecore_timer_del(en->cursor_handler_click_timer);
        en->cursor_handler_click_timer = NULL;

        _edje_emit(rp->edje, "cursor,handler,clicked", rp->part->name);
     }
   else
     {
      _edje_entry_imf_cursor_info_set(en);
      _edje_emit(rp->edje, "cursor,handler,move,end", en->rp->part->name);
     }
}

static void
_edje_entry_cursor_handler_mouse_move_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Entry *en;
   Edje_Real_Part *rp = data;
   Evas_Event_Mouse_Move *ev = event_info;
   Evas_Coord x, y, tx, ty, tw, th;
   Evas_Coord cx, cy;
   Evas_Coord lh;

   if (!rp) return;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;

   if (ev->buttons != 1) return;

   x = ev->cur.canvas.x;
   y = ev->cur.canvas.y;

   en->rx += (x - en->sx);
   en->ry += (y - en->sy);
   en->sx = ev->cur.canvas.x;
   en->sy = ev->cur.canvas.y;

   evas_object_geometry_get(rp->object, &tx, &ty, NULL, NULL);
   evas_object_textblock_size_formatted_get(rp->object, &tw, &th);

   cx = en->rx + en->ox - tx;
   cy = en->ry + en->oy - ty;

   evas_textblock_cursor_line_geometry_get(en->cursor, NULL, NULL, NULL, &lh);
   if (cx <= 0) cx = 1;
   if (cy <= 0) cy = lh / 2;
   if (cy > th) cy = th - 1;

   evas_textblock_cursor_char_coord_set(en->cursor, cx, cy);

   //_edje_entry_imf_cursor_info_set(en);
   _edje_emit(rp->edje, "cursor,changed", rp->part->name); //to scroll scroller
   _edje_entry_real_part_configure(rp);

   _edje_emit(rp->edje, "cursor,handler,moving", rp->part->name);
}

static void
_edje_entry_start_handler_mouse_down_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Evas_Coord ox, oy, ow, oh;
   Evas_Coord lx, ly, lw, lh;
   Edje_Real_Part *rp = data;
   Entry *en = rp->typedata.text->entry_data;

   if (ev->button != 1) return;

   en->rx = en->sx = ev->canvas.x;
   en->ry = en->sy = ev->canvas.y;

   evas_object_geometry_get(obj, &ox, &oy, NULL, NULL);
   edje_object_size_min_calc(obj, &ow, &oh);
   evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);

   en->ox = ox + ow/2 - en->rx;
   //en->oy = oy + oh - en->ry + lh/2;
   en->oy = oy + oh - en->ry - lh/2;

   en->select_mod_start = EINA_TRUE;
   en->selecting = EINA_TRUE;

   _edje_emit(en->rp->edje, "handler,move,start", en->rp->part->name);
}

static void
_edje_entry_start_handler_mouse_up_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Edje_Real_Part *rp = data;
   Entry *en = rp->typedata.text->entry_data;

   //switch sel_start,end
   if (evas_textblock_cursor_compare(en->sel_start, en->sel_end) >= 0)
     {
        Evas_Textblock_Cursor *tc;
        tc = evas_object_textblock_cursor_new(rp->object);
        evas_textblock_cursor_copy(en->sel_start, tc);
        evas_textblock_cursor_copy(en->sel_end, en->sel_start);
        evas_textblock_cursor_copy(tc, en->sel_end);
        evas_textblock_cursor_free(tc);
     }
   //update handlers
   en->handler_bypassing = EINA_FALSE;
   _edje_entry_real_part_configure(rp);

   _edje_emit(en->rp->edje, "handler,move,end", en->rp->part->name);
}

static void
_edje_entry_start_handler_mouse_move_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   Edje_Real_Part *rp = data;
   Entry *en = rp->typedata.text->entry_data;
   Evas_Coord x, y, tx, ty, tw, th;
   Evas_Coord cx, cy;
   Evas_Coord lh;
   Evas_Textblock_Cursor_Type cur_type;
   Evas_Coord sx, sy;

   if (ev->buttons != 1) return;

   x = ev->cur.canvas.x;
   y = ev->cur.canvas.y;

   en->rx += (x - en->sx);
   en->ry += (y - en->sy);
   en->sx = ev->cur.canvas.x;
   en->sy = ev->cur.canvas.y;

   evas_object_geometry_get(rp->object, &tx, &ty, NULL, NULL);
   evas_object_textblock_size_formatted_get(rp->object, &tw, &th);

   cx = en->rx + en->ox - tx;
   cy = en->ry + en->oy - ty;

   evas_textblock_cursor_line_geometry_get(en->cursor, NULL, NULL, NULL, &lh);
   if (cx <= 0) cx = 1;
   if (cy <= 0) cy = lh / 2;
   if (cy > th) cy = th - 1;

   switch (rp->part->cursor_mode)
     {
      case EDJE_ENTRY_CURSOR_MODE_BEFORE:
         cur_type = EVAS_TEXTBLOCK_CURSOR_BEFORE;
         break;
      case EDJE_ENTRY_CURSOR_MODE_UNDER:
      default:
         cur_type = EVAS_TEXTBLOCK_CURSOR_UNDER;
     }

   evas_textblock_cursor_geometry_get(en->sel_end, &sx, &sy, NULL, NULL, NULL, cur_type);

   evas_textblock_cursor_char_coord_set(en->cursor, cx, cy);

   // by passing
   if (evas_textblock_cursor_compare(en->cursor, en->sel_end) >= 0)
      en->handler_bypassing = EINA_TRUE;
   else
      en->handler_bypassing = EINA_FALSE;
   //

   if (en->select_allow)
     {
        if (en->had_sel)
          {
             if (en->select_mod_start)
               _sel_preextend(en->cursor, rp->object, en);
          }
     }
   _edje_entry_real_part_configure(rp);
   _edje_emit(en->rp->edje, "handler,moving", en->rp->part->name);
}

static void
_edje_entry_end_handler_mouse_down_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Down *ev = event_info;
   Evas_Coord ox, oy, ow, oh;
   Evas_Coord lx, ly, lw, lh;
   Edje_Real_Part *rp = data;
   Entry *en = rp->typedata.text->entry_data;

   if (ev->button != 1) return;

   en->rx = en->sx = ev->canvas.x;
   en->ry = en->sy = ev->canvas.y;

   evas_object_geometry_get(obj, &ox, &oy, NULL, NULL);
   edje_object_size_min_calc(obj, &ow, &oh);
   evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);

   en->ox = ox + ow/2 - en->rx;
   en->oy = oy - en->ry - lh/2;

   en->select_mod_end = EINA_TRUE;
   en->selecting = EINA_TRUE;

   _edje_emit(en->rp->edje, "handler,move,start", en->rp->part->name);
}

static void
_edje_entry_end_handler_mouse_up_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Edje_Real_Part *rp = data;
   Entry *en = rp->typedata.text->entry_data;

   //switch sel_start,end
   if (evas_textblock_cursor_compare(en->sel_start, en->sel_end) >= 0)
     {
        Evas_Textblock_Cursor *tc;
        tc = evas_object_textblock_cursor_new(rp->object);
        evas_textblock_cursor_copy(en->sel_start, tc);
        evas_textblock_cursor_copy(en->sel_end, en->sel_start);
        evas_textblock_cursor_copy(tc, en->sel_end);
        // first char case
        if (evas_textblock_cursor_compare(en->sel_start, en->sel_end) == 0)
          {
             evas_textblock_cursor_char_next(en->sel_end);
          }
        evas_textblock_cursor_free(tc);
     }
   //update handlers
   en->handler_bypassing = EINA_FALSE;
   _edje_entry_real_part_configure(rp);

   _edje_emit(en->rp->edje, "handler,move,end", en->rp->part->name);
}

static void
_edje_entry_end_handler_mouse_move_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Evas_Event_Mouse_Move *ev = event_info;
   Edje_Real_Part *rp = data;
   Entry *en = rp->typedata.text->entry_data;
   Evas_Coord x, y, tx, ty, tw, th;
   Evas_Coord cx, cy;
   Evas_Textblock_Cursor_Type cur_type;
   Evas_Coord sx, sy;

   if (ev->buttons != 1) return;

   x = ev->cur.canvas.x;
   y = ev->cur.canvas.y;

   en->rx += (x - en->sx);
   en->ry += (y - en->sy);
   en->sx = ev->cur.canvas.x;
   en->sy = ev->cur.canvas.y;

   evas_object_geometry_get(rp->object, &tx, &ty, NULL, NULL);
   evas_object_textblock_size_formatted_get(rp->object, &tw, &th);

   cx = en->rx + en->ox - tx;
   cy = en->ry + en->oy - ty;

   if (cx < 0) cx = 0;
   if (cy < 0) cy = 0;
   if (cy > th) cy = th - 1;

   switch (rp->part->cursor_mode)
     {
      case EDJE_ENTRY_CURSOR_MODE_BEFORE:
         cur_type = EVAS_TEXTBLOCK_CURSOR_BEFORE;
         break;
      case EDJE_ENTRY_CURSOR_MODE_UNDER:
      default:
         cur_type = EVAS_TEXTBLOCK_CURSOR_UNDER;
     }

   evas_textblock_cursor_geometry_get(en->sel_start, &sx, &sy, NULL, NULL, NULL, cur_type);
   evas_textblock_cursor_char_coord_set(en->cursor, cx, cy);

   // by passing
   if (evas_textblock_cursor_compare(en->cursor, en->sel_start) <= 0)
      en->handler_bypassing = EINA_TRUE;
   else
      en->handler_bypassing = EINA_FALSE;
   //

   if (en->select_allow)
     {
        if (en->had_sel)
          {
             if (en->select_mod_end)
               _sel_extend(en->cursor, rp->object, en);
          }
     }
   _edje_entry_real_part_configure(rp);
   _edje_emit(en->rp->edje, "handler,moving", en->rp->part->name);
}
// TIZEN ONLY - END

static void
_evas_focus_in_cb(void *data, Evas *e, __UNUSED__ void *event_info)
{
   Edje *ed = (Edje *)data;

   if (evas_focus_get(e) == ed->obj)
     {
        _edje_focus_in_cb(data, NULL, NULL, NULL);
     }
}

static void
_evas_focus_out_cb(void *data, Evas *e, __UNUSED__ void *event_info)
{
   Edje *ed = (Edje *)data;

   if (evas_focus_get(e) == ed->obj)
     {
        _edje_focus_out_cb(data, NULL, NULL, NULL);
     }
}

/***************************************************************/
void
_edje_entry_init(Edje *ed)
{
   if (!ed->has_entries)
     return;
   if (ed->entries_inited)
     return;
   ed->entries_inited = EINA_TRUE;

   evas_object_event_callback_add(ed->obj, EVAS_CALLBACK_FOCUS_IN, _edje_focus_in_cb, ed);
   evas_object_event_callback_add(ed->obj, EVAS_CALLBACK_FOCUS_OUT, _edje_focus_out_cb, ed);
   evas_object_event_callback_add(ed->obj, EVAS_CALLBACK_KEY_DOWN, _edje_key_down_cb, ed);
   evas_object_event_callback_add(ed->obj, EVAS_CALLBACK_KEY_UP, _edje_key_up_cb, ed);
   evas_event_callback_add(ed->base.evas, EVAS_CALLBACK_CANVAS_FOCUS_IN, _evas_focus_in_cb, ed);
   evas_event_callback_add(ed->base.evas, EVAS_CALLBACK_CANVAS_FOCUS_OUT, _evas_focus_out_cb, ed);
}

void
_edje_entry_shutdown(Edje *ed)
{
   if (!ed->has_entries)
     return;
   if (!ed->entries_inited)
     return;
   ed->entries_inited = EINA_FALSE;

   evas_object_event_callback_del(ed->obj, EVAS_CALLBACK_FOCUS_IN, _edje_focus_in_cb);
   evas_object_event_callback_del(ed->obj, EVAS_CALLBACK_FOCUS_OUT, _edje_focus_out_cb);
   evas_object_event_callback_del(ed->obj, EVAS_CALLBACK_KEY_DOWN, _edje_key_down_cb);
   evas_object_event_callback_del(ed->obj, EVAS_CALLBACK_KEY_UP, _edje_key_up_cb);
   if (evas_event_callback_del_full(ed->base.evas, EVAS_CALLBACK_CANVAS_FOCUS_IN, _evas_focus_in_cb, ed) != ed)
     ERR("could not unregister EVAS_CALLBACK_FOCUS_IN");
   if (evas_event_callback_del_full(ed->base.evas, EVAS_CALLBACK_CANVAS_FOCUS_OUT, _evas_focus_out_cb, ed) != ed)
     ERR("could not unregister EVAS_CALLBACK_FOCUS_OUT");
}

void
_edje_entry_real_part_init(Edje_Real_Part *rp)
{
   Entry *en;
#ifdef HAVE_ECORE_IMF
   const char *ctx_id;
   const Ecore_IMF_Context_Info *ctx_info;
#endif

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = calloc(1, sizeof(Entry));
   if (!en) return;
   rp->typedata.text->entry_data = en;
   en->rp = rp;

   evas_object_event_callback_add(rp->object, EVAS_CALLBACK_MOVE, _edje_part_move_cb, rp);

   evas_object_event_callback_add(rp->object, EVAS_CALLBACK_MOUSE_DOWN, _edje_part_mouse_down_cb, rp);
   evas_object_event_callback_add(rp->object, EVAS_CALLBACK_MOUSE_UP, _edje_part_mouse_up_cb, rp);
   evas_object_event_callback_add(rp->object, EVAS_CALLBACK_MOUSE_MOVE, _edje_part_mouse_move_cb, rp);

   if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_DEFAULT)
     en->select_allow = EINA_TRUE;

   if (rp->part->entry_mode == EDJE_ENTRY_EDIT_MODE_PASSWORD)
     {
        Edje_Part_Description_Text *txt;

        txt = (Edje_Part_Description_Text *)rp->chosen_description;

        en->select_allow = EINA_FALSE;
        if (txt && edje_string_get(&txt->text.repch))
          evas_object_textblock_replace_char_set(rp->object, edje_string_get(&txt->text.repch));
        else
          evas_object_textblock_replace_char_set(rp->object, "*");
     }

   en->cursor_bg = edje_object_add(rp->edje->base.evas);
   edje_object_file_set(en->cursor_bg, rp->edje->path, rp->part->source3);
   evas_object_smart_member_add(en->cursor_bg, rp->edje->obj);
   evas_object_stack_below(en->cursor_bg, rp->object);
   evas_object_clip_set(en->cursor_bg, evas_object_clip_get(rp->object));
   evas_object_pass_events_set(en->cursor_bg, EINA_TRUE);
   _edje_subobj_register(en->rp->edje, en->cursor_bg);

   en->cursor_fg = edje_object_add(rp->edje->base.evas);
   edje_object_file_set(en->cursor_fg, rp->edje->path, rp->part->source4);
   evas_object_smart_member_add(en->cursor_fg, rp->edje->obj);
   evas_object_stack_above(en->cursor_fg, rp->object);
   evas_object_clip_set(en->cursor_fg, evas_object_clip_get(rp->object));
   evas_object_pass_events_set(en->cursor_fg, EINA_TRUE);
   _edje_subobj_register(en->rp->edje, en->cursor_fg);

   // TIZEN ONLY - START
   //cursor handler
   en->cursor_handler_disabled = EINA_FALSE;
   en->cursor_handler = NULL;
   if (rp->part->source9)
     {
        en->cursor_handler = edje_object_add(en->rp->edje->base.evas);
        edje_object_file_set(en->cursor_handler , en->rp->edje->path, rp->part->source9);

        evas_object_layer_set(en->cursor_handler , EVAS_LAYER_MAX - 2);
        en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, en->cursor_handler );

        evas_object_event_callback_add(en->cursor_handler , EVAS_CALLBACK_MOUSE_DOWN, _edje_entry_cursor_handler_mouse_down_cb, en->rp);
        evas_object_event_callback_add(en->cursor_handler , EVAS_CALLBACK_MOUSE_UP, _edje_entry_cursor_handler_mouse_up_cb, en->rp);
        evas_object_event_callback_add(en->cursor_handler , EVAS_CALLBACK_MOUSE_MOVE, _edje_entry_cursor_handler_mouse_move_cb, en->rp);
     }
   // TIZEN ONLY - END
   en->focused = EINA_FALSE;

   evas_object_textblock_legacy_newline_set(rp->object, EINA_TRUE);

   if (rp->part->entry_mode >= EDJE_ENTRY_EDIT_MODE_EDITABLE)
     {
        evas_object_show(en->cursor_bg);
        evas_object_show(en->cursor_fg);
        en->input_panel_enable = EINA_TRUE;

#ifdef HAVE_ECORE_IMF
        ecore_imf_init();

        en->commit_cancel = EINA_FALSE;

        edje_object_signal_callback_add(rp->edje->obj, "focus,part,in", rp->part->name, _edje_entry_focus_in_cb, rp);
        edje_object_signal_callback_add(rp->edje->obj, "focus,part,out", rp->part->name, _edje_entry_focus_out_cb, rp);

        ctx_id = ecore_imf_context_default_id_get();
        if (ctx_id)
          {
             ctx_info = ecore_imf_context_info_by_id_get(ctx_id);
             if (!ctx_info->canvas_type ||
                 strcmp(ctx_info->canvas_type, "evas") == 0)
               {
                  en->imf_context = ecore_imf_context_add(ctx_id);
               }
             else
               {
                  ctx_id = ecore_imf_context_default_id_by_canvas_type_get("evas");
                  if (ctx_id)
                    {
                       en->imf_context = ecore_imf_context_add(ctx_id);
                    }
               }
          }
        else
          en->imf_context = NULL;

        if (!en->imf_context) goto done;

        ecore_imf_context_client_window_set
           (en->imf_context,
               (void *)ecore_evas_window_get
               (ecore_evas_ecore_evas_get(rp->edje->base.evas)));
        ecore_imf_context_client_canvas_set(en->imf_context, rp->edje->base.evas);

        ecore_imf_context_retrieve_surrounding_callback_set(en->imf_context,
                                                            _edje_entry_imf_retrieve_surrounding_cb, rp->edje);
        ecore_imf_context_event_callback_add(en->imf_context, ECORE_IMF_CALLBACK_COMMIT, _edje_entry_imf_event_commit_cb, rp->edje);
        ecore_imf_context_event_callback_add(en->imf_context, ECORE_IMF_CALLBACK_DELETE_SURROUNDING, _edje_entry_imf_event_delete_surrounding_cb, rp->edje);
        ecore_imf_context_event_callback_add(en->imf_context, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, _edje_entry_imf_event_preedit_changed_cb, rp->edje);
        ecore_imf_context_input_mode_set(en->imf_context,
                                         rp->part->entry_mode == EDJE_ENTRY_EDIT_MODE_PASSWORD ?
                                         ECORE_IMF_INPUT_MODE_INVISIBLE : ECORE_IMF_INPUT_MODE_FULL);

        if (rp->part->entry_mode == EDJE_ENTRY_EDIT_MODE_PASSWORD)
          ecore_imf_context_input_panel_language_set(en->imf_context, ECORE_IMF_INPUT_PANEL_LANG_ALPHABET);
#endif
     }
#ifdef HAVE_ECORE_IMF
done:
#endif
   en->cursor = (Evas_Textblock_Cursor *)evas_object_textblock_cursor_get(rp->object);
}

void
_edje_entry_real_part_shutdown(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   rp->typedata.text->entry_data = NULL;
   _sel_clear(en->cursor, rp->object, en);
   _anchors_clear(en->cursor, rp->object, en);
#ifdef HAVE_ECORE_IMF
   _preedit_clear(en);
#endif
   // TIZEN ONLY - START
   rp->edje->subobjs = eina_list_remove(rp->edje->subobjs, en->cursor_bg);
   rp->edje->subobjs = eina_list_remove(rp->edje->subobjs, en->cursor_fg);
   rp->edje->subobjs = eina_list_remove(rp->edje->subobjs, en->sel_handler_start);
   rp->edje->subobjs = eina_list_remove(rp->edje->subobjs, en->sel_handler_end);
   rp->edje->subobjs = eina_list_remove(rp->edje->subobjs, en->sel_handler_edge_start);
   rp->edje->subobjs = eina_list_remove(rp->edje->subobjs, en->sel_handler_edge_end);
   rp->edje->subobjs = eina_list_remove(rp->edje->subobjs, en->cursor_handler);
   // TIZEN ONLY - END
   evas_object_del(en->cursor_bg);
   evas_object_del(en->cursor_fg);

   // TIZEN ONLY - START
   if (en->sel_handler_start)
     {
        evas_object_del(en->sel_handler_start);
        en->sel_handler_start = NULL;
     }
   if (en->sel_handler_end)
     {
        evas_object_del(en->sel_handler_end);
        en->sel_handler_end = NULL;
     }
   if (en->cursor_handler)
     {
        evas_object_del(en->cursor_handler);
        en->cursor_handler = NULL;
     }
   if (en->sel_handler_edge_start)
     {
        evas_object_del(en->sel_handler_edge_start);
        en->sel_handler_edge_start = NULL;
     }
   if (en->sel_handler_edge_end)
     {
        evas_object_del(en->sel_handler_edge_end);
        en->sel_handler_edge_end = NULL;
     }

   if (en->long_press_timer)
     {
        ecore_timer_del(en->long_press_timer);
        en->long_press_timer = NULL;
     }
   if (en->cursor_handler_click_timer)
     {
        ecore_timer_del(en->cursor_handler_click_timer);
        en->cursor_handler_click_timer = NULL;
     }
   // TIZEN ONLY - END

   if (en->pw_timer)
     {
        ecore_timer_del(en->pw_timer);
        en->pw_timer = NULL;
     }

#ifdef HAVE_ECORE_IMF
   if (rp->part->entry_mode >= EDJE_ENTRY_EDIT_MODE_EDITABLE)
     {
        if (en->imf_context)
          {
             ecore_imf_context_event_callback_del(en->imf_context, ECORE_IMF_CALLBACK_COMMIT, _edje_entry_imf_event_commit_cb);
             ecore_imf_context_event_callback_del(en->imf_context, ECORE_IMF_CALLBACK_DELETE_SURROUNDING, _edje_entry_imf_event_delete_surrounding_cb);
             ecore_imf_context_event_callback_del(en->imf_context, ECORE_IMF_CALLBACK_PREEDIT_CHANGED, _edje_entry_imf_event_preedit_changed_cb);

             ecore_imf_context_del(en->imf_context);
             en->imf_context = NULL;
          }

        edje_object_signal_callback_del(rp->edje->obj, "focus,part,in", rp->part->name, _edje_entry_focus_in_cb);
        edje_object_signal_callback_del(rp->edje->obj, "focus,part,out", rp->part->name, _edje_entry_focus_out_cb);
        ecore_imf_shutdown();
     }
#endif
   _compose_seq_reset(en);
   
   free(en);
}

void
_edje_entry_real_part_configure(Edje_Real_Part *rp)
{
   Evas_Coord x, y, w, h, xx, yy, ww, hh;
   Entry *en;
   Evas_Textblock_Cursor_Type cur_type;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   switch (rp->part->cursor_mode)
     {
      case EDJE_ENTRY_CURSOR_MODE_BEFORE:
         cur_type = EVAS_TEXTBLOCK_CURSOR_BEFORE;
         break;
      case EDJE_ENTRY_CURSOR_MODE_UNDER:
         /* no break for a resaon */
      default:
         cur_type = EVAS_TEXTBLOCK_CURSOR_UNDER;
     }

   _sel_update(en->cursor, rp->object, en);
   _anchors_update(en->cursor, rp->object, en);
   x = y = w = h = -1;
   xx = yy = ww = hh = -1;
   evas_object_geometry_get(rp->object, &x, &y, &w, &h);
   evas_textblock_cursor_geometry_get(en->cursor, &xx, &yy, &ww, &hh, NULL, cur_type);
   if (ww < 1) ww = 1;
   if (hh < 1) hh = 1;
   if (en->cursor_bg)
     {
        evas_object_move(en->cursor_bg, x + xx, y + yy);
        evas_object_resize(en->cursor_bg, ww, hh);
     }
   if (en->cursor_fg)
     {
        evas_object_move(en->cursor_fg, x + xx, y + yy);
        evas_object_resize(en->cursor_fg, ww, hh);
     }
   if (en->cursor_handler && (!en->cursor_handler_disabled || en->long_press_state == _ENTRY_LONG_PRESSED || en->long_press_state == _ENTRY_LONG_PRESSING))
     {
        Evas_Coord chx, chy;
        chx = x + xx;
        chy = y + yy + hh;
        evas_object_move(en->cursor_handler, chx, chy);
        if ((chx < en->viewport_region.x) || (chy < en->viewport_region.y))
           evas_object_hide(en->cursor_handler);
     }
}

const char *
_edje_entry_selection_get(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return NULL;
   en = rp->typedata.text->entry_data;
   if (!en) return NULL;
   // get selection - convert to markup
   if ((!en->selection) && (en->have_selection))
     en->selection = evas_textblock_cursor_range_text_get
        (en->sel_start, en->sel_end, EVAS_TEXTBLOCK_TEXT_MARKUP);
   return en->selection;
}

const char *
_edje_entry_text_get(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return NULL;
   en = rp->typedata.text->entry_data;
   if (!en) return NULL;
   // get text - convert to markup
   return evas_object_textblock_text_markup_get(rp->object);
}

void
_edje_entry_text_markup_set(Edje_Real_Part *rp, const char *text)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   _edje_entry_imf_context_reset(rp);
   // set text as markup
   _sel_clear(en->cursor, rp->object, en);
   evas_object_textblock_text_markup_set(rp->object, text);
   _edje_entry_set_cursor_start(rp);

   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);

   // TIZEN ONLY(130129) : Currently, for freezing cursor movement.
   if (!en->freeze)
     {
        _edje_entry_imf_cursor_info_set(en);
        _edje_entry_real_part_configure(rp);
     }
   //

#if 0
   /* Don't emit cursor changed cause it didn't. It's just init to 0. */
   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
#endif
}

void
_edje_entry_text_markup_append(Edje_Real_Part *rp, const char *text)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   Evas_Textblock_Cursor *end_cur;
   if (!en) return;
   end_cur = evas_object_textblock_cursor_new(rp->object);
   evas_textblock_cursor_paragraph_last(end_cur);

   _text_filter_markup_prepend(en, end_cur, text);
   evas_textblock_cursor_free(end_cur);

   /* We are updating according to the real cursor on purpose */
   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);

   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_text_markup_insert(Edje_Real_Part *rp, const char *text)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   _edje_entry_imf_context_reset(rp);

   // prepend markup @ cursor pos
   if (en->have_selection)
     _range_del(en->cursor, rp->object, en);
   //xx
//   evas_object_textblock_text_markup_prepend(en->cursor, text);
   _text_filter_markup_prepend(en, en->cursor, text);
   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);
   _edje_emit(rp->edje, "cursor,changed", rp->part->name);

   _edje_entry_imf_cursor_info_set(en);
   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_set_cursor_start(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   _curs_start(en->cursor, rp->object, en);

   // TIZEN ONLY(130129) : Currently, for freezing cursor movement.
   if (!en->freeze)
      _edje_entry_imf_cursor_info_set(en);
}

void
_edje_entry_set_cursor_end(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   _curs_end(en->cursor, rp->object, en);

   _edje_entry_imf_cursor_info_set(en);
}

void
_edje_entry_select_none(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   _sel_clear(en->cursor, rp->object, en);
}

void
_edje_entry_select_all(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;

   _edje_entry_imf_context_reset(rp);

   _sel_clear(en->cursor, rp->object, en);
   _curs_start(en->cursor, rp->object, en);
   _sel_enable(en->cursor, rp->object, en);
   _sel_start(en->cursor, rp->object, en);
   _curs_end(en->cursor, rp->object, en);
   _sel_extend(en->cursor, rp->object, en);

   _edje_entry_real_part_configure(rp);

   // TIZEN ONLY - START
   en->select_allow = EINA_TRUE;
   en->had_sel = EINA_TRUE;

   _edje_emit(en->rp->edje, "selection,end", en->rp->part->name);
   // TIZEN ONLY - END
}

void
_edje_entry_select_begin(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;

   _edje_entry_imf_context_reset(rp);

   _sel_clear(en->cursor, rp->object, en);
   _sel_enable(en->cursor, rp->object, en);
   _sel_start(en->cursor, rp->object, en);
   _sel_extend(en->cursor, rp->object, en);

   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_select_extend(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   _edje_entry_imf_context_reset(rp);
   _sel_extend(en->cursor, rp->object, en);

   _edje_entry_real_part_configure(rp);
}

const Eina_List *
_edje_entry_anchor_geometry_get(Edje_Real_Part *rp, const char *anchor)
{
   Entry *en;
   Eina_List *l;
   Anchor *an;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return NULL;
   en = rp->typedata.text->entry_data;
   if (!en) return NULL;
   EINA_LIST_FOREACH(en->anchors, l, an)
     {
        if (an->item) continue;
        if (!strcmp(anchor, an->name))
          return an->sel;
     }
   return NULL;
}

const Eina_List *
_edje_entry_anchors_list(Edje_Real_Part *rp)
{
   Entry *en;
   Eina_List *l, *anchors = NULL;
   Anchor *an;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return NULL;
   en = rp->typedata.text->entry_data;
   if (!en) return NULL;
   if (!en->anchorlist)
     {
        EINA_LIST_FOREACH(en->anchors, l, an)
          {
             const char *n = an->name;
             if (an->item) continue;
             if (!n) n = "";
             anchors = eina_list_append(anchors, strdup(n));
          }
        en->anchorlist = anchors;
     }
   return en->anchorlist;
}

Eina_Bool
_edje_entry_item_geometry_get(Edje_Real_Part *rp, const char *item, Evas_Coord *cx, Evas_Coord *cy, Evas_Coord *cw, Evas_Coord *ch)
{
   Entry *en;
   Eina_List *l;
   Anchor *an;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;
   en = rp->typedata.text->entry_data;
   if (!en) return EINA_FALSE;
   EINA_LIST_FOREACH(en->anchors, l, an)
     {
        if (an->item) continue;
        if (!strcmp(item, an->name))
          {
             evas_textblock_cursor_format_item_geometry_get(an->start, cx, cy, cw, ch);
             return EINA_TRUE;
          }
     }
   return EINA_FALSE;
}

const Eina_List *
_edje_entry_items_list(Edje_Real_Part *rp)
{
   Entry *en;
   Eina_List *l, *items = NULL;
   Anchor *an;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return NULL;
   en = rp->typedata.text->entry_data;
   if (!en) return NULL;
   if (!en->itemlist)
     {
        EINA_LIST_FOREACH(en->anchors, l, an)
          {
             const char *n = an->name;
             if (an->item) continue;
             if (!n) n = "";
             items = eina_list_append(items, strdup(n));
          }
        en->itemlist = items;
     }
   return en->itemlist;
}

// TIZEN ONLY - START
void
_edje_entry_layout_region_set(Edje_Real_Part *rp, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   Entry *en;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;

   en->layout_region.x = x;
   en->layout_region.y = y;
   en->layout_region.w = w;
   en->layout_region.h = h;
   _sel_update(en->cursor, rp->object, en);
}

void
_edje_entry_viewport_region_set(Edje_Real_Part *rp, Evas_Coord x, Evas_Coord y, Evas_Coord w, Evas_Coord h)
{
   Entry *en;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;

   en->viewport_region.x = x;
   en->viewport_region.y = y;
   en->viewport_region.w = w;
   en->viewport_region.h = h;
   _sel_update(en->cursor, rp->object, en);
}

void
_edje_entry_selection_handler_geometry_get(Edje_Real_Part *rp, Edje_Selection_Handler type, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   Entry *en;
   Evas_Coord ex, ey, hx, hy, edx, edy, edw, edh;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;

   evas_object_geometry_get(rp->object, &ex, &ey, NULL, NULL);
   switch (type)
     {
      case EDJE_SELECTION_HANDLER_START:
         if (en->sel_handler_start)
           {
              evas_object_geometry_get(en->sel_handler_start, &hx, &hy, NULL, NULL);
              edje_object_part_geometry_get(en->sel_handler_start, "handle", &edx, &edy, &edw, &edh);
              hx = hx - ex + edx;
              hy = hy - ey + edy;
           }
         break;
      case EDJE_SELECTION_HANDLER_END:
         if (en->sel_handler_end)
           {
              evas_object_geometry_get(en->sel_handler_end, &hx, &hy, NULL, NULL);
              edje_object_part_geometry_get(en->sel_handler_end, "handle", &edx, &edy, &edw, &edh);
              hx = hx - ex + edx;
              hy = hy - ey + edy;
           }
         break;
      default:
         break;
     }

   if (x) *x = hx;
   if (y) *y = hy;
   if (w) *w = edw;
   if (h) *h = edh;
}

Eina_Bool
_edje_entry_cursor_handler_disabled_get(Edje_Real_Part *rp)
{
   Entry *en;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;
   en = rp->typedata.text->entry_data;
   if (!en) return EINA_FALSE;
   return en->cursor_handler_disabled;
}

void
_edje_entry_cursor_handler_disabled_set(Edje_Real_Part *rp, Eina_Bool disabled)
{
   Entry *en;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;

   en->cursor_handler_disabled = disabled;
}
// TIZEN ONLY - END

void
_edje_entry_cursor_geometry_get(Edje_Real_Part *rp, Evas_Coord *cx, Evas_Coord *cy, Evas_Coord *cw, Evas_Coord *ch)
{
   Evas_Coord x, y, w, h, xx, yy, ww, hh;
   Entry *en;
   Evas_Textblock_Cursor_Type cur_type;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   switch (rp->part->cursor_mode)
     {
      case EDJE_ENTRY_CURSOR_MODE_BEFORE:
         cur_type = EVAS_TEXTBLOCK_CURSOR_BEFORE;
         break;
      case EDJE_ENTRY_CURSOR_MODE_UNDER:
         /* no break for a resaon */
      default:
         cur_type = EVAS_TEXTBLOCK_CURSOR_UNDER;
     }

   x = y = w = h = -1;
   xx = yy = ww = hh = -1;
   evas_object_geometry_get(rp->object, &x, &y, &w, &h);
   evas_textblock_cursor_geometry_get(en->cursor, &xx, &yy, &ww, &hh, NULL, cur_type);
   if (ww < 1) ww = 1;
   if (rp->part->cursor_mode == EDJE_ENTRY_CURSOR_MODE_BEFORE)
     edje_object_size_min_restricted_calc(en->cursor_fg, &ww, NULL, ww, 0);
   if (hh < 1) hh = 1;
   if (cx) *cx = x + xx;
   if (cy) *cy = y + yy;
   if (cw) *cw = ww;
   if (ch) *ch = hh;
}

void
_edje_entry_user_insert(Edje_Real_Part *rp, const char *text)
{
   Entry *en;
   Edje_Entry_Change_Info *info;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   info = calloc(1, sizeof(*info));
   if (!info) return;
   info->insert = EINA_TRUE;
   info->change.insert.plain_length = 1;
   info->change.insert.content = eina_stringshare_add(text);
     {
        char *tmp;
        tmp = evas_textblock_text_markup_to_utf8(rp->object,
                                                 info->change.insert.content);
        info->change.insert.plain_length = eina_unicode_utf8_get_len(tmp);
        free(tmp);
     }

   if (en->have_selection)
     {
        _range_del_emit(rp->edje, en->cursor, rp->object, en);
        info->merge = EINA_TRUE;
     }
   info->change.insert.pos = evas_textblock_cursor_pos_get(en->cursor);
   _text_filter_markup_prepend(en, en->cursor, text);
   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);
   _edje_emit_full(rp->edje, "entry,changed,user", rp->part->name,
                   info, _free_entry_change_info);
   _edje_emit(rp->edje, "cursor,changed", rp->part->name);

   _edje_entry_imf_cursor_info_set(en);
   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_select_allow_set(Edje_Real_Part *rp, Eina_Bool allow)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_DEFAULT)
     return;
   en->select_allow = allow;
}

Eina_Bool
_edje_entry_select_allow_get(const Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;
   en = rp->typedata.text->entry_data;
   if (!en) return EINA_FALSE;
   return en->select_allow;
}

void
_edje_entry_select_abort(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   if (en->selecting)
     {
        en->selecting = EINA_FALSE;

        _edje_entry_imf_context_reset(rp);
        _edje_entry_imf_cursor_info_set(en);
        _edje_entry_real_part_configure(rp);
     }
}

// TIZEN ONLY - START
Eina_Bool
_edje_entry_selection_geometry_get(Edje_Real_Part *rp, Evas_Coord *x, Evas_Coord *y, Evas_Coord *w, Evas_Coord *h)
{
   Entry *en = rp->typedata.text->entry_data;
   if (!en || !en->have_selection || !en->sel)
     return EINA_FALSE;

   Sel *sel;
   Eina_List *l;
   Evas_Textblock_Rectangle rect;
   Evas_Coord tx, ty, tw, th;

   l = en->sel;
   if (!l) return EINA_FALSE;
   sel = eina_list_data_get(l);
   if (!sel) return EINA_FALSE;
   rect = sel->rect;
   ty = rect.y;
   tw = rect.w;

   l = eina_list_last(en->sel);
   if (!l) return EINA_FALSE;
   sel = eina_list_data_get(l);
   if (!sel) return EINA_FALSE;
   rect = sel->rect;

   tx = rect.x;
   th = rect.y - ty + rect.h;

   EINA_LIST_FOREACH(en->sel, l, sel)
     {
        if (sel)
          {
             if (tw < sel->rect.w) tw = sel->rect.w;
             if (sel->rect.x < tx) tx = sel->rect.x;
          }
     }

   Evas_Coord vx, vy;
   evas_object_geometry_get(rp->object, &vx, &vy, NULL, NULL);
   tx += vx;
   ty += vy;

   if (x) *x = tx;
   if (y) *y = ty;
   if (w) *w = tw;
   if (h) *h = th;

   return EINA_TRUE;
}
// TIZEN ONLY - END

void *
_edje_entry_imf_context_get(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return NULL;
   en = rp->typedata.text->entry_data;
   if (!en) return NULL;

#ifdef HAVE_ECORE_IMF
   return en->imf_context;
#else
   return NULL;
#endif
}

void
_edje_entry_autocapital_type_set(Edje_Real_Part *rp, Edje_Text_Autocapital_Type autocapital_type)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;

   if (rp->part->entry_mode == EDJE_ENTRY_EDIT_MODE_PASSWORD)
     autocapital_type = EDJE_TEXT_AUTOCAPITAL_TYPE_NONE;

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     ecore_imf_context_autocapital_type_set(en->imf_context, autocapital_type);
#endif
}

Edje_Text_Autocapital_Type
_edje_entry_autocapital_type_get(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EDJE_TEXT_AUTOCAPITAL_TYPE_NONE;
   en = rp->typedata.text->entry_data;
   if (!en) return EDJE_TEXT_AUTOCAPITAL_TYPE_NONE;

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     return ecore_imf_context_autocapital_type_get(en->imf_context);
#endif

   return EDJE_TEXT_AUTOCAPITAL_TYPE_NONE;
}

void
_edje_entry_prediction_allow_set(Edje_Real_Part *rp, Eina_Bool prediction)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   en->prediction_allow = prediction;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     ecore_imf_context_prediction_allow_set(en->imf_context, prediction);
#endif
}

Eina_Bool
_edje_entry_prediction_allow_get(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;
   en = rp->typedata.text->entry_data;
   if (!en) return EINA_FALSE;
   return en->prediction_allow;
}

void
_edje_entry_input_panel_enabled_set(Edje_Real_Part *rp, Eina_Bool enabled)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   en->input_panel_enable = enabled;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     ecore_imf_context_input_panel_enabled_set(en->imf_context, enabled);
#endif
}

Eina_Bool
_edje_entry_input_panel_enabled_get(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;
   en = rp->typedata.text->entry_data;
   if (!en) return EINA_FALSE;
   return en->input_panel_enable;
}

void
_edje_entry_input_panel_show(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     ecore_imf_context_input_panel_show(en->imf_context);
#endif
}

void
_edje_entry_input_panel_hide(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     ecore_imf_context_input_panel_hide(en->imf_context);
#endif
}

void
_edje_entry_input_panel_language_set(Edje_Real_Part *rp, Edje_Input_Panel_Lang lang)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   en->input_panel_lang = lang;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     ecore_imf_context_input_panel_language_set(en->imf_context, lang);
#endif
}

Edje_Input_Panel_Lang
_edje_entry_input_panel_language_get(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EDJE_INPUT_PANEL_LANG_AUTOMATIC;
   en = rp->typedata.text->entry_data;
   if (!en) return EDJE_INPUT_PANEL_LANG_AUTOMATIC;
   return en->input_panel_lang;
}

#ifdef HAVE_ECORE_IMF
void
_edje_entry_input_panel_imdata_set(Edje_Real_Part *rp, const void *data, int len)
#else
void
_edje_entry_input_panel_imdata_set(Edje_Real_Part *rp, const void *data __UNUSED__, int len __UNUSED__)
#endif
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     ecore_imf_context_input_panel_imdata_set(en->imf_context, data, len);
#endif
}

#ifdef HAVE_ECORE_IMF
void
_edje_entry_input_panel_imdata_get(Edje_Real_Part *rp, void *data, int *len)
#else
void
_edje_entry_input_panel_imdata_get(Edje_Real_Part *rp, void *data __UNUSED__, int *len __UNUSED__)
#endif
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     ecore_imf_context_input_panel_imdata_get(en->imf_context, data, len);
#endif
}

#ifdef HAVE_ECORE_IMF
void
_edje_entry_input_panel_return_key_type_set(Edje_Real_Part *rp, Edje_Input_Panel_Return_Key_Type return_key_type)
#else
void
_edje_entry_input_panel_return_key_type_set(Edje_Real_Part *rp, Edje_Input_Panel_Return_Key_Type return_key_type __UNUSED__)
#endif
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     ecore_imf_context_input_panel_return_key_type_set(en->imf_context, return_key_type);
#endif
}

Edje_Input_Panel_Return_Key_Type
_edje_entry_input_panel_return_key_type_get(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EDJE_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT;
   en = rp->typedata.text->entry_data;
   if (!en) return EDJE_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     return ecore_imf_context_input_panel_return_key_type_get(en->imf_context);
#endif
   return EDJE_INPUT_PANEL_RETURN_KEY_TYPE_DEFAULT;
}

#ifdef HAVE_ECORE_IMF
void
_edje_entry_input_panel_return_key_disabled_set(Edje_Real_Part *rp, Eina_Bool disabled)
#else
void
_edje_entry_input_panel_return_key_disabled_set(Edje_Real_Part *rp, Eina_Bool disabled __UNUSED__)
#endif
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     ecore_imf_context_input_panel_return_key_disabled_set(en->imf_context, disabled);
#endif
}

Eina_Bool
_edje_entry_input_panel_return_key_disabled_get(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;
   en = rp->typedata.text->entry_data;
   if (!en) return EINA_FALSE;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     return ecore_imf_context_input_panel_return_key_disabled_get(en->imf_context);
#endif
   return EINA_FALSE;
}

static Evas_Textblock_Cursor *
_cursor_get(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return NULL;
   en = rp->typedata.text->entry_data;
   if (!en) return NULL;
   switch (cur)
     {
      case EDJE_CURSOR_MAIN:
         return en->cursor;
      case EDJE_CURSOR_SELECTION_BEGIN:
         return en->sel_start;
      case EDJE_CURSOR_SELECTION_END:
         return en->sel_end;
      case EDJE_CURSOR_PREEDIT_START:
         if (!en->preedit_start)
           en->preedit_start = evas_object_textblock_cursor_new(rp->object);
         return en->preedit_start;
      case EDJE_CURSOR_PREEDIT_END:
         if (!en->preedit_end)
           en->preedit_end = evas_object_textblock_cursor_new(rp->object);
         return en->preedit_end;
      case EDJE_CURSOR_USER:
         if (!en->cursor_user)
           en->cursor_user = evas_object_textblock_cursor_new(rp->object);
         return en->cursor_user;
      case EDJE_CURSOR_USER_EXTRA:
         if (!en->cursor_user_extra)
           en->cursor_user_extra = evas_object_textblock_cursor_new(rp->object);
         return en->cursor_user_extra;
      default:
         break;
     }
   return NULL;
}

Eina_Bool
_edje_entry_cursor_next(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;
   en = rp->typedata.text->entry_data;
   if (!en) return EINA_FALSE;
   
   if (!c) return EINA_FALSE;

   _edje_entry_imf_context_reset(rp);

   if (!evas_textblock_cursor_char_next(c))
     {
        return EINA_FALSE;
     }
   _sel_update(c, rp->object, rp->typedata.text->entry_data);
   _edje_entry_imf_cursor_info_set(en);

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
   return EINA_TRUE;
}

Eina_Bool
_edje_entry_cursor_prev(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;
   en = rp->typedata.text->entry_data;
   if (!en) return EINA_FALSE;
   if (!c) return EINA_FALSE;

   _edje_entry_imf_context_reset(rp);

   if (!evas_textblock_cursor_char_prev(c))
     {
        if (evas_textblock_cursor_paragraph_prev(c)) goto ok;
        else return EINA_FALSE;
     }
ok:
   _sel_update(c, rp->object, rp->typedata.text->entry_data);

   _edje_entry_imf_cursor_info_set(en);

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
   return EINA_TRUE;
}

Eina_Bool
_edje_entry_cursor_up(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   Evas_Coord lx, ly, lw, lh, cx, cy, cw, ch;
   int ln;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;
   en = rp->typedata.text->entry_data;
   if (!en) return EINA_FALSE;
   if (!c) return EINA_FALSE;

   _edje_entry_imf_context_reset(rp);

   ln = evas_textblock_cursor_line_geometry_get(c, NULL, NULL, NULL, NULL);
   ln--;
   if (ln < 0) return EINA_FALSE;
   if (!evas_object_textblock_line_number_geometry_get(rp->object, ln,
                                                       &lx, &ly, &lw, &lh))
     return EINA_FALSE;
   evas_textblock_cursor_char_geometry_get(c, &cx, &cy, &cw, &ch);
   if (!evas_textblock_cursor_char_coord_set(c, cx, ly + (lh / 2)))
     {
        if (cx < (lx + (lw / 2)))
          evas_textblock_cursor_line_char_last(c);
        else
          evas_textblock_cursor_line_char_last(c);
     }
   _sel_update(c, rp->object, rp->typedata.text->entry_data);

   _edje_entry_imf_cursor_info_set(en);

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
   return EINA_TRUE;
}

Eina_Bool
_edje_entry_cursor_down(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   Evas_Coord lx, ly, lw, lh, cx, cy, cw, ch;
   int ln;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;
   en = rp->typedata.text->entry_data;
   if (!en) return EINA_FALSE;
   if (!c) return EINA_FALSE;

   _edje_entry_imf_context_reset(rp);

   ln = evas_textblock_cursor_line_geometry_get(c, NULL, NULL, NULL, NULL);
   ln++;
   if (!evas_object_textblock_line_number_geometry_get(rp->object, ln,
                                                       &lx, &ly, &lw, &lh))
     return EINA_FALSE;
   evas_textblock_cursor_char_geometry_get(c, &cx, &cy, &cw, &ch);
   if (!evas_textblock_cursor_char_coord_set(c, cx, ly + (lh / 2)))
     {
        if (cx < (lx + (lw / 2)))
          evas_textblock_cursor_line_char_last(c);
        else
          evas_textblock_cursor_line_char_last(c);
     }
   _sel_update(c, rp->object, rp->typedata.text->entry_data);

   _edje_entry_imf_cursor_info_set(en);
   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
   return EINA_TRUE;
}

void
_edje_entry_cursor_begin(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   if (!c) return;

   _edje_entry_imf_context_reset(rp);

   evas_textblock_cursor_paragraph_first(c);
   _sel_update(c, rp->object, rp->typedata.text->entry_data);

   _edje_entry_imf_cursor_info_set(en);
   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_cursor_end(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   if (!c) return;

   _edje_entry_imf_context_reset(rp);

   _curs_end(c, rp->object, rp->typedata.text->entry_data);
   _sel_update(c, rp->object, rp->typedata.text->entry_data);

   _edje_entry_imf_cursor_info_set(en);

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_cursor_copy(Edje_Real_Part *rp, Edje_Cursor cur, Edje_Cursor dst)
{
   Entry *en;
   Evas_Textblock_Cursor *c;
   Evas_Textblock_Cursor *d;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   c = _cursor_get(rp, cur);
   if (!c) return;
   d = _cursor_get(rp, dst);
   if (!d) return;
   evas_textblock_cursor_copy(c, d);
   _sel_update(c, rp->object, rp->typedata.text->entry_data);

   _edje_entry_imf_context_reset(rp);
   _edje_entry_imf_cursor_info_set(en);
   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_cursor_line_begin(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   if (!c) return;
   _edje_entry_imf_context_reset(rp);

   evas_textblock_cursor_line_char_first(c);
   _sel_update(c, rp->object, rp->typedata.text->entry_data);

   _edje_entry_imf_cursor_info_set(en);

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_cursor_line_end(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   if (!c) return;
   _edje_entry_imf_context_reset(rp);
   evas_textblock_cursor_line_char_last(c);
   _sel_update(c, rp->object, rp->typedata.text->entry_data);

   _edje_entry_imf_cursor_info_set(en);
   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
}

Eina_Bool
_edje_entry_cursor_coord_set(Edje_Real_Part *rp, Edje_Cursor cur,
                             Evas_Coord x, Evas_Coord y)
{
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   if (!c) return EINA_FALSE;
   return evas_textblock_cursor_char_coord_set(c, x, y);
}

Eina_Bool
_edje_entry_cursor_is_format_get(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   if (!c) return EINA_FALSE;
   if (evas_textblock_cursor_is_format(c)) return EINA_TRUE;
   return EINA_FALSE;
}

Eina_Bool
_edje_entry_cursor_is_visible_format_get(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   if (!c) return EINA_FALSE;
   return evas_textblock_cursor_format_is_visible_get(c);
}

char *
_edje_entry_cursor_content_get(Edje_Real_Part *rp, Edje_Cursor cur)
{
   static char *s = NULL;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);

   if (!c) return NULL;
   if (s)
     {
        free(s);
        s = NULL;
     }

   s = evas_textblock_cursor_content_get(c);
   return s;
}

void
_edje_entry_cursor_pos_set(Edje_Real_Part *rp, Edje_Cursor cur, int pos)
{
   Entry *en;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
   if (!c) return;
   /* Abort if cursor position didn't really change */
   if (evas_textblock_cursor_pos_get(c) == pos)
     return;

   _edje_entry_imf_context_reset(rp);
   evas_textblock_cursor_pos_set(c, pos);
   _sel_update(c, rp->object, rp->typedata.text->entry_data);

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   // TIZEN ONLY(130129) : Currently, for freezing cursor movement.
   if (!en->freeze)
     {
        _edje_entry_imf_cursor_info_set(en);
        _edje_entry_real_part_configure(rp);
     }
   //
}

int
_edje_entry_cursor_pos_get(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   if (!c) return 0;
   return evas_textblock_cursor_pos_get(c);
}

void
_edje_entry_input_panel_layout_set(Edje_Real_Part *rp, Edje_Input_Panel_Layout layout)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     ecore_imf_context_input_panel_layout_set(en->imf_context, layout);
#else
   (void) layout;
#endif
}

Edje_Input_Panel_Layout
_edje_entry_input_panel_layout_get(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EDJE_INPUT_PANEL_LAYOUT_INVALID;
   en = rp->typedata.text->entry_data;
   if (!en) return EDJE_INPUT_PANEL_LAYOUT_INVALID;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     return ecore_imf_context_input_panel_layout_get(en->imf_context);
#endif

   return EDJE_INPUT_PANEL_LAYOUT_INVALID;
}

void
_edje_entry_input_panel_layout_variation_set(Edje_Real_Part *rp, int variation)
{
   Entry *en;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     ecore_imf_context_input_panel_layout_variation_set(en->imf_context, variation);
#else
   (void) variation;
#endif
}

int
_edje_entry_input_panel_layout_variation_get(Edje_Real_Part *rp)
{
   Entry *en;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return 0;
   en = rp->typedata.text->entry_data;
   if (!en) return 0;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     return ecore_imf_context_input_panel_layout_variation_get(en->imf_context);
#endif

   return 0;
}

void
_edje_entry_imf_context_reset(Edje_Real_Part *rp)
{
   Entry *en;
   
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   en = rp->typedata.text->entry_data;
   if (!en) return;
#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     ecore_imf_context_reset(en->imf_context);
   if (en->commit_cancel)
      en->commit_cancel = EINA_FALSE;
#endif
}

static void
_edje_entry_imf_cursor_location_set(Entry *en)
{
#ifdef HAVE_ECORE_IMF
   Evas_Coord cx, cy, cw, ch;
   if (!en || !en->rp || !en->imf_context) return;

   _edje_entry_cursor_geometry_get(en->rp, &cx, &cy, &cw, &ch);
   ecore_imf_context_cursor_location_set(en->imf_context, cx, cy, cw, ch);
#else
   (void) en;
#endif
}

static void
_edje_entry_imf_cursor_info_set(Entry *en)
{
   int cursor_pos;

#ifdef HAVE_ECORE_IMF
   if (!en || !en->rp || !en->imf_context) return;

   if (en->have_selection)
     {
        if (evas_textblock_cursor_compare(en->sel_start, en->sel_end) < 0)
          cursor_pos = evas_textblock_cursor_pos_get(en->sel_start);
        else
          cursor_pos = evas_textblock_cursor_pos_get(en->sel_end);
     }
   else
     cursor_pos = evas_textblock_cursor_pos_get(en->cursor);

   ecore_imf_context_cursor_position_set(en->imf_context, cursor_pos);

   _edje_entry_imf_cursor_location_set(en);
#else
   (void) en;
#endif
}

#ifdef HAVE_ECORE_IMF
static Eina_Bool
_edje_entry_imf_retrieve_surrounding_cb(void *data, Ecore_IMF_Context *ctx __UNUSED__, char **text, int *cursor_pos)
{
   Edje *ed = data;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en = NULL;
   const char *str;
   char *plain_text;

   if (!rp) return EINA_FALSE;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return EINA_FALSE;
   else
     en = rp->typedata.text->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return EINA_FALSE;

   if (text)
     {
        str = _edje_entry_text_get(rp);
        if (str)
          plain_text = evas_textblock_text_markup_to_utf8(NULL, str);
        else
          plain_text = strdup("");
        *text = plain_text;
     }

   if (cursor_pos)
     *cursor_pos = evas_textblock_cursor_pos_get(en->cursor);

   return EINA_TRUE;
}

static void
_edje_entry_imf_event_commit_cb(void *data, Ecore_IMF_Context *ctx __UNUSED__, void *event_info)
{
   Edje *ed = data;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en = NULL;
   char *commit_str = event_info;
   int start_pos;

   if ((!rp)) return;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   else
     en = rp->typedata.text->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return;

   if (en->have_selection)
     {
        if (strcmp(commit_str, ""))
          {
             /* delete selected characters */
             _range_del_emit(ed, en->cursor, rp->object, en);
             _sel_clear(en->cursor, rp->object, en);
          }
     }

   start_pos = evas_textblock_cursor_pos_get(en->cursor);

   /* delete preedit characters */
   _preedit_del(en);
   _preedit_clear(en);

   // Skipping commit process when it is useless
   if (en->commit_cancel)
     {
        en->commit_cancel = EINA_FALSE;
        return;
     }

   if ((rp->part->entry_mode == EDJE_ENTRY_EDIT_MODE_PASSWORD) &&
       _edje_password_show_last)
     _edje_entry_hide_visible_password(en->rp);
   if ((rp->part->entry_mode == EDJE_ENTRY_EDIT_MODE_PASSWORD) &&
       _edje_password_show_last && (!en->preedit_start))
     {
        _text_filter_format_prepend(en, en->cursor, "+ password=off");
        _text_filter_text_prepend(en, en->cursor, commit_str);
        _text_filter_format_prepend(en, en->cursor, "- password");

        if (en->pw_timer)
          {
             ecore_timer_del(en->pw_timer);
             en->pw_timer = NULL;
          }
        en->pw_timer = ecore_timer_add(TO_DOUBLE(_edje_password_show_last_timeout),
                                       _password_timer_cb, en);
     }
   else
     _text_filter_text_prepend(en, en->cursor, commit_str);


   _edje_entry_imf_cursor_info_set(en);
   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);

     {
        Edje_Entry_Change_Info *info = calloc(1, sizeof(*info));
        info->insert = EINA_TRUE;
        info->change.insert.pos = start_pos;
        info->change.insert.content = eina_stringshare_add(commit_str);
        info->change.insert.plain_length =
           eina_unicode_utf8_get_len(info->change.insert.content);
        _edje_emit_full(ed, "entry,changed,user", rp->part->name,
                        info, _free_entry_change_info);
        _edje_emit(ed, "cursor,changed", rp->part->name);
     }

   _edje_entry_imf_cursor_info_set(en);
   _edje_entry_real_part_configure(rp);
}

static void
_edje_entry_imf_event_preedit_changed_cb(void *data, Ecore_IMF_Context *ctx __UNUSED__, void *event_info __UNUSED__)
{
   Edje *ed = data;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en = NULL;
   int cursor_pos;
   int preedit_start_pos, preedit_end_pos;
   char *preedit_string;
   char *preedit_tag_index;
   char *pretag = NULL;
   char *markup_txt = NULL;
   char *tagname[] = {NULL, "preedit", "preedit_sel", "preedit_sel",
                      "preedit_sub1", "preedit_sub2", "preedit_sub3", "preedit_sub4"};
   int i;
   size_t preedit_type_size = sizeof(tagname) / sizeof(tagname[0]);
   Eina_Bool preedit_end_state = EINA_FALSE;
   Eina_List *attrs = NULL, *l = NULL;
   Ecore_IMF_Preedit_Attr *attr;
   Eina_Strbuf *buf;
   Eina_Strbuf *preedit_attr_str;

   if ((!rp)) return;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   else
     en = rp->typedata.text->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return;

   if (!en->imf_context) return;

   ecore_imf_context_preedit_string_with_attributes_get(en->imf_context,
                                                        &preedit_string,
                                                        &attrs, &cursor_pos);
   if (!preedit_string) return;

   if (!strcmp(preedit_string, ""))
     preedit_end_state = EINA_TRUE;

   if (en->have_selection && !preedit_end_state)
     {
        /* delete selected characters */
        _range_del_emit(ed, en->cursor, rp->object, en);
     }

   if (en->preedit_start && en->preedit_end)
     {
        /* extract the tag string */
        char *str = evas_textblock_cursor_range_text_get(en->preedit_start, en->preedit_end, EVAS_TEXTBLOCK_TEXT_MARKUP);

        if (str)
          {
             preedit_tag_index = strstr(str, "<preedit");

             if ((preedit_tag_index - str) > 0)
               {
                  pretag = calloc(1, sizeof(char)*(preedit_tag_index-str+1));
                  if (preedit_tag_index)
                    {
                       strncpy(pretag, str, preedit_tag_index-str);
                    }
               }
             free(str);
          }
     }

   /* delete preedit characters */
   _preedit_del(en);

   preedit_start_pos = evas_textblock_cursor_pos_get(en->cursor);

   /* restore previous tag */
   if (pretag)
     {
        _text_filter_markup_prepend(en, en->cursor, pretag);
        free(pretag);
     }

   /* insert preedit character(s) */
   if (strlen(preedit_string) > 0)
     {
        buf = eina_strbuf_new();
        if (attrs)
          {
             EINA_LIST_FOREACH(attrs, l, attr)
               {
                  if (attr->preedit_type <= preedit_type_size &&
                      tagname[attr->preedit_type])
                    {
                       preedit_attr_str = eina_strbuf_new();
                       if (preedit_attr_str)
                         {
                            eina_strbuf_append_n(preedit_attr_str, preedit_string + attr->start_index, attr->end_index - attr->start_index);
                            markup_txt = evas_textblock_text_utf8_to_markup(NULL, eina_strbuf_string_get(preedit_attr_str));

                            if (markup_txt)
                              {
                                 eina_strbuf_append_printf(buf, "<%s>%s</%s>", tagname[attr->preedit_type], markup_txt, tagname[attr->preedit_type]);
                                 free(markup_txt);
                              }
                            eina_strbuf_free(preedit_attr_str);
                         }
                    }
                  else
                    eina_strbuf_append(buf, preedit_string);
               }
          }
        else
          {
             eina_strbuf_append(buf, preedit_string);
          }

        // For skipping useless commit
        if (!preedit_end_state)
           en->have_preedit = EINA_TRUE;

        if ((rp->part->entry_mode == EDJE_ENTRY_EDIT_MODE_PASSWORD) &&
            _edje_password_show_last)
          {
             _edje_entry_hide_visible_password(en->rp);
             _text_filter_format_prepend(en, en->cursor, "+ password=off");
             _text_filter_markup_prepend(en, en->cursor, eina_strbuf_string_get(buf));
             _text_filter_format_prepend(en, en->cursor, "- password");
             if (en->pw_timer)
               {
                  ecore_timer_del(en->pw_timer);
                  en->pw_timer = NULL;
               }
             en->pw_timer = ecore_timer_add(TO_DOUBLE(_edje_password_show_last_timeout),
                                            _password_timer_cb, en);
          }
        else
          {
             _text_filter_markup_prepend(en, en->cursor, eina_strbuf_string_get(buf));
          }
        eina_strbuf_free(buf);
     }

   if (!preedit_end_state)
     {
        /* set preedit start cursor */
        if (!en->preedit_start)
          en->preedit_start = evas_object_textblock_cursor_new(rp->object);
        evas_textblock_cursor_copy(en->cursor, en->preedit_start);

        /* set preedit end cursor */
        if (!en->preedit_end)
          en->preedit_end = evas_object_textblock_cursor_new(rp->object);
        evas_textblock_cursor_copy(en->cursor, en->preedit_end);

        preedit_end_pos = evas_textblock_cursor_pos_get(en->cursor);

        for (i = 0; i < (preedit_end_pos - preedit_start_pos); i++)
          {
             evas_textblock_cursor_char_prev(en->preedit_start);
          }

        en->have_preedit = EINA_TRUE;

        /* set cursor position */
        evas_textblock_cursor_pos_set(en->cursor, preedit_start_pos + cursor_pos);
     }

   _edje_entry_imf_cursor_info_set(en);
   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "preedit,changed", rp->part->name);
   _edje_emit(ed, "cursor,changed", rp->part->name);

   /* delete attribute list */
   if (attrs)
     {
        EINA_LIST_FREE(attrs, attr) free(attr);
     }

   free(preedit_string);
}

static void
_edje_entry_imf_event_delete_surrounding_cb(void *data, Ecore_IMF_Context *ctx __UNUSED__, void *event_info)
{
   Edje *ed = data;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en = NULL;
   Ecore_IMF_Event_Delete_Surrounding *ev = event_info;
   Evas_Textblock_Cursor *del_start, *del_end;
   int cursor_pos;

   if ((!rp) || (!ev)) return;
   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;
   else
     en = rp->typedata.text->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
     return;

   cursor_pos = evas_textblock_cursor_pos_get(en->cursor);

   del_start = evas_object_textblock_cursor_new(en->rp->object);
   evas_textblock_cursor_pos_set(del_start, cursor_pos + ev->offset);

   del_end = evas_object_textblock_cursor_new(en->rp->object);
   evas_textblock_cursor_pos_set(del_end, cursor_pos + ev->offset + ev->n_chars);

   evas_textblock_cursor_range_delete(del_start, del_end);

   evas_textblock_cursor_free(del_start);
   evas_textblock_cursor_free(del_end);
}
#endif


//////////////////////////////////////  TIZEN ONLY(130129)  : START //////////////////////////////////
void _edje_entry_freeze(Edje_Real_Part *rp)
{
   Entry *en = NULL;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;

   en = rp->typedata.text->entry_data;
   if (!en) return;

   en->freeze = EINA_TRUE;
}

void _edje_entry_thaw(Edje_Real_Part *rp)
{
   Entry *en = NULL;

   if ((rp->type != EDJE_RP_TYPE_TEXT) ||
       (!rp->typedata.text)) return;

   en = rp->typedata.text->entry_data;
   if (!en) return;

   en->freeze = EINA_FALSE;

   _edje_entry_imf_cursor_info_set(en);
   _edje_entry_real_part_configure(rp);
}
///////////////////////////////////////////  TIZEN ONLY : END   ////////////////////////////////////////

/* vim:set ts=8 sw=3 sts=3 expandtab cino=>5n-2f0^-2{2(0W1st0 :*/
