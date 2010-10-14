#include "edje_private.h"

#ifdef HAVE_ECORE_IMF

static Eina_Bool _edje_entry_imf_retrieve_surrounding_cb(void *data, Ecore_IMF_Context *ctx, char **text, int *cursor_pos);
static Eina_Bool _edje_entry_imf_event_commit_cb(void *data, int type, void *event);
static Eina_Bool _edje_entry_imf_event_preedit_changed_cb(void *data, int type, void *event);
static Eina_Bool _edje_entry_imf_event_delete_surrounding_cb(void *data, int type, void *event);
#endif

typedef struct _Entry Entry;
typedef struct _Sel Sel;
typedef struct _Anchor Anchor;

struct _Entry
{
   Edje_Real_Part *rp;
   Evas_Coord cx, cy;
   Evas_Object *cursor_bg;
   Evas_Object *cursor_fg;
   Evas_Textblock_Cursor *cursor;
   Evas_Textblock_Cursor *sel_start, *sel_end;
   Evas_Textblock_Cursor *cursor_user, *cursor_user_extra;
   Evas_Textblock_Cursor *preedit_start, *preedit_end;
   Eina_List *sel;
   Eina_List *anchors;
   Eina_List *anchorlist;
   Eina_List *itemlist;
   char *selection;
   Eina_Bool selecting : 1;
   Eina_Bool have_selection : 1;
   Eina_Bool select_allow : 1;
   Eina_Bool select_mod_start : 1;
   Eina_Bool select_mod_end : 1;
   Eina_Bool had_sel : 1;

#ifdef HAVE_ECORE_IMF   
   int	comp_len;
   Eina_Bool have_composition : 1;
   Ecore_IMF_Context *imf_context;

   Ecore_Event_Handler *imf_ee_handler_commit;
   Ecore_Event_Handler *imf_ee_handler_delete;
   Ecore_Event_Handler *imf_ee_handler_changed;
#endif   
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
_edje_entry_focus_in_cb(void *data, Evas_Object *o __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Edje_Real_Part *rp;
   Entry *en;

   rp = data;
   if (!rp || !rp->entry_data || !rp->edje || !rp->edje->obj) return;

   en = rp->entry_data;
   if (!en || !en->imf_context) return;

   if (evas_object_focus_get(rp->edje->obj))
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_focus_in(en->imf_context);
     }
}

static void
_edje_entry_focus_out_cb(void *data, Evas_Object *o __UNUSED__, const char *emission __UNUSED__, const char *source __UNUSED__)
{
   Edje_Real_Part *rp;
   Entry *en;

   rp = data;
   if (!rp || !rp->entry_data) return;

   en = rp->entry_data;
   if (!en || !en->imf_context) return;

   ecore_imf_context_reset(en->imf_context);
   ecore_imf_context_cursor_position_set(en->imf_context, evas_textblock_cursor_pos_get(en->cursor));
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
#ifdef HAVE_ECORE_IMF
   rp = ed->focused_part;
   if (!rp) return;

   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
         (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_EDITABLE))
     return;

   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_focus_in(en->imf_context);
     }
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
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
         (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_EDITABLE))
     return;

   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
              evas_textblock_cursor_pos_get(en->cursor));
        ecore_imf_context_focus_out(en->imf_context);
     }
#endif
}

static void
_text_filter_text_prepend(Entry *en, const char *text)
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
        evas_textblock_cursor_text_prepend(en->cursor, text2);
        free(text2);
     }
}

static void
_text_filter_format_prepend(Entry *en, const char *text)
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
        evas_textblock_cursor_format_prepend(en->cursor, text2);
        free(text2);
     }
}

static void
_text_filter_markup_prepend(Entry *en, const char *text)
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
     {
        evas_object_textblock_text_markup_prepend(en->cursor, text2);
        free(text2);
     }
}

static void
_curs_update_from_curs(Evas_Textblock_Cursor *c, Evas_Object *o __UNUSED__, Entry *en)
{
   Evas_Coord cx, cy, cw, ch;
   if (c != en->cursor) return;
   evas_textblock_cursor_geometry_get(c, &cx, &cy, &cw, &ch, NULL, EVAS_TEXTBLOCK_CURSOR_UNDER);
   en->cx = cx + (cw / 2);
   en->cy = cy + (ch / 2);
}

static void
_curs_back(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   evas_textblock_cursor_char_prev(c);
   _curs_update_from_curs(c, o, en);
}

static void
_curs_next(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   evas_textblock_cursor_char_next(c);
   _curs_update_from_curs(c, o, en);
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
_curs_lin_start(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   evas_textblock_cursor_line_char_first(c);
   _curs_update_from_curs(c, o, en);
}

static void
_curs_lin_end(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   evas_textblock_cursor_line_char_last(c);
   _curs_update_from_curs(c, o, en);
}

static void
_curs_start(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   evas_textblock_cursor_paragraph_first(c);
   _curs_update_from_curs(c, o, en);
}

static void
_curs_end(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   evas_textblock_cursor_paragraph_last(c);
   _curs_lin_end(c, o, en);
   _curs_update_from_curs(c, o, en);
}

static void
_curs_jump_line(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en, int ln)
{
   Evas_Coord lx, ly, lw, lh;
   int last = _curs_line_last_get(c, o, en);

   if (ln < 0) ln = 0;
   else
     {
	if (ln > last) ln = last;
     }
   if (!evas_object_textblock_line_number_geometry_get(o, ln, &lx, &ly, &lw, &lh))
     return;
   if (evas_textblock_cursor_char_coord_set(c, en->cx, ly + (lh / 2)))
     return;
   evas_textblock_cursor_line_set(c, ln);
   if (en->cx < (lx + (lw / 2)))
     {
        if (ln == last) _curs_end(c, o, en);
//        evas_textblock_cursor_line_char_last(c);
        _curs_lin_start(c, o, en);
     }
   else
     {
        if (ln == last)
          _curs_end(c, o, en);
        else
          _curs_lin_end(c, o, en);
//        evas_textblock_cursor_line_char_last(c);
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
   _sel_enable(c, o, en);
   if (!evas_textblock_cursor_compare(c, en->sel_end)) return;
   evas_textblock_cursor_copy(c, en->sel_end);
   if (en->selection)
     {
        free(en->selection);
        en->selection = NULL;
     }
   _edje_emit(en->rp->edje, "selection,changed", en->rp->part->name);
}

static void
_sel_preextend(Evas_Textblock_Cursor *c, Evas_Object *o, Entry *en)
{
   if (!en->sel_end) return;
   _sel_enable(c, o, en);
   if (!evas_textblock_cursor_compare(c, en->sel_start)) return;
   evas_textblock_cursor_copy(c, en->sel_start);
   if (en->selection)
     {
        free(en->selection);
        en->selection = NULL;
     }
   _edje_emit(en->rp->edje, "selection,changed", en->rp->part->name);
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
        en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_bg);
        en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_fg);
        if (sel->obj_bg) evas_object_del(sel->obj_bg);
        if (sel->obj_fg) evas_object_del(sel->obj_fg);
        free(sel);
        en->sel = eina_list_remove_list(en->sel, en->sel);
     }
   if (en->have_selection)
     {
        en->have_selection = EINA_FALSE;
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
      range = evas_textblock_cursor_range_geometry_get(en->sel_start, en->sel_end);
   else
      return;
   if (eina_list_count(range) != eina_list_count(en->sel))
     {
        while (en->sel)
          {
             sel = en->sel->data;
             en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_bg);
             en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_fg);
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
                  ob = edje_object_add(en->rp->edje->evas);
                  edje_object_file_set(ob, en->rp->edje->path, en->rp->part->source);
                  evas_object_smart_member_add(ob, smart);
                  evas_object_stack_below(ob, o);
                  evas_object_clip_set(ob, clip);
                  evas_object_pass_events_set(ob, EINA_TRUE);
                  evas_object_show(ob);
                  sel->obj_bg = ob;
                  en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, sel->obj_bg);

                  ob = edje_object_add(en->rp->edje->evas);
                  edje_object_file_set(ob, en->rp->edje->path, en->rp->part->source2);
                  evas_object_smart_member_add(ob, smart);
                  evas_object_stack_above(ob, o);
                  evas_object_clip_set(ob, clip);
                  evas_object_pass_events_set(ob, EINA_TRUE);
                  evas_object_show(ob);
                  sel->obj_fg = ob;
                  en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, sel->obj_fg);
               }
          }
     }
   x = y = w = h = -1;
   evas_object_geometry_get(o, &x, &y, &w, &h);
   if (en->have_selection)
     {
        EINA_LIST_FOREACH(en->sel, l, sel)
          {
             Evas_Textblock_Rectangle *r;

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
   int len;
   int ignored;
   Entry *en;

   en = rp->entry_data;
   if ((rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT) &&
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
   int len;
   int ignored;
   Entry *en;

   en = rp->entry_data;
   ignored = rp->part->ignore_flags & ev->event_flags;
   if ((rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT) &&
       (en->select_allow))
      return;
   if ((!ev->event_flags) || (!ignored))
     {
        n = an->name;
        if (!n) n = "";
        len = 200 + strlen(n);
        buf = alloca(len);
        snprintf(buf, len, "anchor,mouse,up,%i,%s", ev->button, n);
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
   int len;
   int ignored;
   Entry *en;

   en = rp->entry_data;
   if ((rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT) &&
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
   int len;
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
   int len;
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
   Eina_List *l, *ll, *range;
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
                       en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_bg);
                       en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_fg);
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
             range = evas_textblock_cursor_range_geometry_get(an->start, an->end);
             if (eina_list_count(range) != eina_list_count(an->sel))
               {
                  while (an->sel)
                    {
                       sel = an->sel->data;
                       en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_bg);
                       en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_fg);
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
                       ob = edje_object_add(en->rp->edje->evas);
                       edje_object_file_set(ob, en->rp->edje->path, en->rp->part->source5);
                       evas_object_smart_member_add(ob, smart);
                       evas_object_stack_below(ob, o);
                       evas_object_clip_set(ob, clip);
                       evas_object_pass_events_set(ob, EINA_TRUE);
                       evas_object_show(ob);
                       sel->obj_bg = ob;
                       en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, sel->obj_bg);
                       
                       ob = edje_object_add(en->rp->edje->evas);
                       edje_object_file_set(ob, en->rp->edje->path, en->rp->part->source6);
                       evas_object_smart_member_add(ob, smart);
                       evas_object_stack_above(ob, o);
                       evas_object_clip_set(ob, clip);
                       evas_object_pass_events_set(ob, EINA_TRUE);
                       evas_object_show(ob);
                       sel->obj_fg = ob;
                       en->rp->edje->subobjs = eina_list_append(en->rp->edje->subobjs, sel->obj_fg);
                       
                       ob = evas_object_rectangle_add(en->rp->edje->evas);
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
             en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_bg);
             en->rp->edje->subobjs = eina_list_remove(en->rp->edje->subobjs, sel->obj_fg);
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
   Evas_Textblock_Cursor *c1;
   const Evas_Object_Textblock_Node_Format *node;
   Anchor *an = NULL;

   _anchors_clear(c, o, en);
   c1 = evas_object_textblock_cursor_new(o);
   node = evas_textblock_node_format_first_get(o);
   for (; node ; node = evas_textblock_node_format_next_get(node))
     {
        const char *s;

        evas_textblock_cursor_at_format_set(c1, node);
        s = evas_textblock_node_format_text_get(node);
        if (s)
          {
             if ((!strncmp(s, "+ a ", 4)) || (!strncmp(s, "+a ", 3)))
               {
                  an = calloc(1, sizeof(Anchor));
                  if (an)
                    {
                       char *p;

                       an->en = en;
                       p = strstr(s, "href=");
                       if (p)
                         {
                            an->name = strdup(p + 5);
                         }
                       en->anchors = eina_list_append(en->anchors, an);
                       an->start = evas_object_textblock_cursor_new(o);
                       an->end = evas_object_textblock_cursor_new(o);
                       evas_textblock_cursor_copy(c1, an->start);
                       evas_textblock_cursor_copy(c1, an->end);
                    }
               }
             else if ((!strcmp(s, "- a")) || (!strcmp(s, "-a")))
               {
                  /* Close the anchor, if the anchor was without text, free it as well */
                  if (an)
                    {
                       evas_textblock_cursor_at_format_set(an->end, node);
                       if (!evas_textblock_cursor_compare(an->start, an->end))
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
             else if (!strncmp(s, "+ item ", 7))
               {
                  an = calloc(1, sizeof(Anchor));
                  if (an)
                    {
                       char *p;

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
                       evas_textblock_cursor_copy(c1, an->start);
                       evas_textblock_cursor_copy(c1, an->end);
                    }
               }
             else if ((!strcmp(s, "- item")) || (!strcmp(s, "-item")))
               {
                  if (an)
                    {
                       /* 
                          if (!firsttext)
                          {
                          if (an->name) free(an->name);
                          evas_textblock_cursor_free(an->start);
                          evas_textblock_cursor_free(an->end);
                          en->anchors = eina_list_remove(en->anchors, an);
                          free(an);
                          }
                          */
                       an = NULL;
                    }
               }
          }
     }
   evas_textblock_cursor_free(c1);
}


static void
_range_del(Evas_Textblock_Cursor *c __UNUSED__, Evas_Object *o __UNUSED__, Entry *en)
{
   evas_textblock_cursor_range_delete(en->sel_start, en->sel_end);
}

static void
_backspace(Evas_Textblock_Cursor *c, Evas_Object *o __UNUSED__, Entry *en __UNUSED__)
{
   if (evas_textblock_cursor_char_prev(c))
      evas_textblock_cursor_char_delete(c);
}

static void
_delete(Evas_Textblock_Cursor *c, Evas_Object *o __UNUSED__, Entry *en __UNUSED__)
{
   evas_textblock_cursor_char_delete(c);
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
   Evas_Textblock_Cursor *tc;
   if (!rp) return;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_EDITABLE))
      return;
   if (!ev->key) return;

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        Ecore_IMF_Event_Key_Down ecore_ev;
        ecore_imf_evas_event_key_down_wrap(ev, &ecore_ev);
        if (ecore_imf_context_filter_event(en->imf_context,
                                           ECORE_IMF_EVENT_KEY_DOWN,
                                           (Ecore_IMF_Event *)&ecore_ev))
           return;
     }
#endif

   tc = evas_object_textblock_cursor_new(rp->object);
   evas_textblock_cursor_copy(en->cursor, tc);

   control = evas_key_modifier_is_set(ev->modifiers, "Control");
   alt = evas_key_modifier_is_set(ev->modifiers, "Alt");
   shift = evas_key_modifier_is_set(ev->modifiers, "Shift");
   multiline = rp->part->multiline;
   cursor_changed = EINA_FALSE;
   if (!strcmp(ev->key, "Escape"))
     {
        // dead keys here. Escape for now (should emit these)
        _edje_emit(ed, "entry,key,escape", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Up"))
     {
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
               }
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        _edje_emit(ed, "entry,key,up", rp->part->name);
     }
   else if (!strcmp(ev->key, "Down"))
     {
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
               }
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        _edje_emit(ed, "entry,key,down", rp->part->name);
     }
   else if (!strcmp(ev->key, "Left"))
     {
        if (en->select_allow)
          {
             if (shift) _sel_start(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        _curs_back(en->cursor, rp->object, en);
        if (en->select_allow)
          {
             if (shift) _sel_extend(en->cursor, rp->object, en);
          }
        _edje_emit(ed, "entry,key,left", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Right"))
     {
        if (en->select_allow)
          {
             if (shift) _sel_start(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        _curs_next(en->cursor, rp->object, en);
        if (en->select_allow)
          {
             if (shift) _sel_extend(en->cursor, rp->object, en);
          }
        _edje_emit(ed, "entry,key,right", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "BackSpace"))
     {
        if (control)
          {
             // del to start of previous word
          }
        else if ((alt) && (shift))
          {
             // undo last action
          }
        else
          {
             if (en->have_selection)
                _range_del(en->cursor, rp->object, en);
             else
                _backspace(en->cursor, rp->object, en);
          }
        _sel_clear(en->cursor, rp->object, en);
        _curs_update_from_curs(en->cursor, rp->object, en);
        _anchors_get(en->cursor, rp->object, en);
        _edje_emit(ed, "entry,changed", rp->part->name);
        _edje_emit(ed, "entry,key,backspace", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Delete"))
     {
        if (control)
          {
             // del to end of next word
          }
        else if (shift)
          {
             // cut
          }
        else
          {	
             if (en->have_selection)
                _range_del(en->cursor, rp->object, en);
             else
                _delete(en->cursor, rp->object, en);
          }
        _sel_clear(en->cursor, rp->object, en);
        _curs_update_from_curs(en->cursor, rp->object, en);
        _anchors_get(en->cursor, rp->object, en);
        _edje_emit(ed, "entry,changed", rp->part->name);
        _edje_emit(ed, "entry,key,delete", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Home"))
     {
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
   else if (!strcmp(ev->key, "End"))
     {
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
   else if ((control) && (!strcmp(ev->key, "v")))
     {
        _edje_emit(ed, "entry,paste,request", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && ((!strcmp(ev->key, "c") || (!strcmp(ev->key, "Insert")))))
     {
        // FIXME: copy - save selection
        _edje_emit(ed, "entry,copy,notify", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && ((!strcmp(ev->key, "x") || (!strcmp(ev->key, "m")))))
     {
        // FIXME: cut - save selection, delete seletion
        _curs_update_from_curs(en->cursor, rp->object, en);
        _edje_emit(ed, "entry,cut,notify", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && (!strcmp(ev->key, "z")))
     {
        if (shift)
          {
             // redo
          }
        else
          {
             // undo
          }
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && (!strcmp(ev->key, "y")))
     {
        // redo
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((control) && (!strcmp(ev->key, "w")))
     {
        _sel_clear(en->cursor, rp->object, en);
        // select current word
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Tab"))
     {
        if (multiline)
          {
             if (shift)
               {
                  // remove a tab
               }
             else
               {
                  //yy
//                  evas_textblock_cursor_format_prepend(en->cursor, "\t");
                  _text_filter_format_prepend(en, "\t");
                  _curs_update_from_curs(en->cursor, rp->object, en);
                  _anchors_get(en->cursor, rp->object, en);
                  _edje_emit(ed, "entry,changed", rp->part->name);
               }
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        _edje_emit(ed, "entry,key,tab", rp->part->name);
     }
   else if ((!strcmp(ev->key, "ISO_Left_Tab")) && (multiline))
     { 
        // remove a tab
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Prior"))
     {
        if (en->select_allow)
          {
             if (shift) _sel_start(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        _curs_jump_line_by(en->cursor, rp->object, en, -10);
        if (en->select_allow)
          {
             if (shift) _sel_extend(en->cursor, rp->object, en);
          }
        _edje_emit(ed, "entry,key,pgup", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if (!strcmp(ev->key, "Next"))
     {
        if (en->select_allow)
          {
             if (shift) _sel_start(en->cursor, rp->object, en);
             else _sel_clear(en->cursor, rp->object, en);
          }
        _curs_jump_line_by(en->cursor, rp->object, en, 10);
        if (en->select_allow)
          {
             if (shift) _sel_extend(en->cursor, rp->object, en);
          }
        _edje_emit(ed, "entry,key,pgdn", rp->part->name);
        ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
     }
   else if ((!strcmp(ev->key, "Return")) || (!strcmp(ev->key, "KP_Enter")))
     {
        if (multiline)
          {
             if (en->have_selection)
                _range_del(en->cursor, rp->object, en);
             _sel_clear(en->cursor, rp->object, en);
             if (shift)
               {
                  //yy
//                  evas_textblock_cursor_format_prepend(en->cursor, "\n");
                  _text_filter_format_prepend(en, "\n");
               }
             else
               {
                  //yy
//                  evas_textblock_cursor_format_prepend(en->cursor, "ps");
                  _text_filter_format_prepend(en, "ps");
               }
             _curs_update_from_curs(en->cursor, rp->object, en);
             _anchors_get(en->cursor, rp->object, en);
             _edje_emit(ed, "entry,changed", rp->part->name);
             _edje_emit(ed, "cursor,changed", rp->part->name);
             cursor_changed = EINA_TRUE;
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
        _edje_emit(ed, "entry,key,enter", rp->part->name);
     }
   else
     {
        if (ev->string)
          {
             if (en->have_selection)
                _range_del(en->cursor, rp->object, en);
             _sel_clear(en->cursor, rp->object, en);
             //zz
//             evas_textblock_cursor_text_prepend(en->cursor, ev->string);
             _text_filter_text_prepend(en, ev->string);
             _curs_update_from_curs(en->cursor, rp->object, en);
             _anchors_get(en->cursor, rp->object, en);
             _edje_emit(ed, "entry,changed", rp->part->name);
             _edje_emit(ed, "cursor,changed", rp->part->name);
             cursor_changed = EINA_TRUE;
             ev->event_flags |= EVAS_EVENT_FLAG_ON_HOLD;
          }
     }
   if ((evas_textblock_cursor_compare(tc, en->cursor)) && (!cursor_changed))
      _edje_emit(ed, "cursor,changed", rp->part->name);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   evas_textblock_cursor_free(tc);
   _edje_entry_real_part_configure(rp);
}

static void
_edje_key_up_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Edje *ed = data;
   Evas_Event_Key_Up *ev = event_info;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en;
   if (!rp) return;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_EDITABLE))
      return;

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        Ecore_IMF_Event_Key_Up ecore_ev;
        ecore_imf_evas_event_key_up_wrap(ev, &ecore_ev);
        if (ecore_imf_context_filter_event(en->imf_context,
                                           ECORE_IMF_EVENT_KEY_UP,
                                           (Ecore_IMF_Event *)&ecore_ev))
           return;
     }
#endif
}

static void
_edje_part_mouse_down_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Edje_Real_Part *rp = data;
   Evas_Event_Mouse_Down *ev = event_info;
   Entry *en;
   Evas_Coord x, y, w, h;
   //   Eina_Bool multiline;
   Evas_Textblock_Cursor *tc;
   Eina_Bool dosel = EINA_FALSE;
   if (!rp) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
      return;
   if (ev->button == 2)
     {
        _edje_emit(rp->edje, "entry,paste,request", rp->part->name);
        return;
     }
   if (ev->button != 1) return;
   en->select_mod_start = EINA_FALSE;
   en->select_mod_end = EINA_FALSE;
   if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_DEFAULT)
      dosel = EINA_TRUE;
   else if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT)
     {
        if (en->select_allow) dosel = EINA_TRUE;
     }
   if (dosel)
     {
        // double click -> select word
        // triple click -> select line
     }
   tc = evas_object_textblock_cursor_new(rp->object);
   evas_textblock_cursor_copy(en->cursor, tc);
   //   multiline = rp->part->multiline;
   evas_object_geometry_get(rp->object, &x, &y, &w, &h);
   en->cx = ev->canvas.x - x;
   en->cy = ev->canvas.y - y;
   if (!evas_textblock_cursor_char_coord_set(en->cursor, en->cx, en->cy))
     {
        Evas_Coord lx, ly, lw, lh;
        int line;

        line = evas_textblock_cursor_line_coord_set(en->cursor, en->cy);
        if (line == -1)
           _curs_end(en->cursor, rp->object, en);
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
                  if (en->cx <= lx)
                     _curs_lin_start(en->cursor, rp->object, en);
                  else
                     _curs_lin_end(en->cursor, rp->object, en);
               }
          }
     }
   if (dosel)
     {
        if ((en->have_selection) && 
            (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT))
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
                  d = r1->x - en->cx;
                  d1 = d * d;
                  d = (r1->y + (r1->h / 2)) - en->cy;
                  d1 += d * d;
                  d = r2->x + r2->w - 1 - en->cx;
                  d2 = d * d;
                  d = (r2->y + (r2->h / 2)) - en->cy;
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
   if (evas_textblock_cursor_compare(tc, en->cursor))
      _edje_emit(rp->edje, "cursor,changed", rp->part->name);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   evas_textblock_cursor_free(tc);
   _edje_entry_real_part_configure(rp);
}

static void
_edje_part_mouse_up_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Edje_Real_Part *rp = data;
   Evas_Event_Mouse_Up *ev = event_info;
   Entry *en;
   Evas_Coord x, y, w, h;
   Evas_Textblock_Cursor *tc;
   if (ev->button != 1) return;
   if (!rp) return;
   if (ev->event_flags & EVAS_EVENT_FLAG_ON_HOLD) return;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
      return;
   tc = evas_object_textblock_cursor_new(rp->object);
   evas_textblock_cursor_copy(en->cursor, tc);
   evas_object_geometry_get(rp->object, &x, &y, &w, &h);
   en->cx = ev->canvas.x - x;
   en->cy = ev->canvas.y - y;
   if (!evas_textblock_cursor_char_coord_set(en->cursor, en->cx, en->cy))
     {
        Evas_Coord lx, ly, lw, lh;
        int line;

        line = evas_textblock_cursor_line_coord_set(en->cursor, en->cy);
        if (line == -1)
           _curs_end(en->cursor, rp->object, en);
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
                  if (en->cx <= lx)
                     _curs_lin_start(en->cursor, rp->object, en);
                  else
                     _curs_lin_end(en->cursor, rp->object, en);
               }
          }
     }
   if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT)
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
   if (evas_textblock_cursor_compare(tc, en->cursor))
      _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   evas_textblock_cursor_free(tc);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   _edje_entry_real_part_configure(rp);
}

static void
_edje_part_mouse_move_cb(void *data, Evas *e __UNUSED__, Evas_Object *obj __UNUSED__, void *event_info)
{
   Edje_Real_Part *rp = data;
   Evas_Event_Mouse_Move *ev = event_info;
   Entry *en;
   Evas_Coord x, y, w, h;
   Evas_Textblock_Cursor *tc;
   if (!rp) return;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
      return;
   if (en->selecting)
     {
        tc = evas_object_textblock_cursor_new(rp->object);
        evas_textblock_cursor_copy(en->cursor, tc);
        evas_object_geometry_get(rp->object, &x, &y, &w, &h);
        en->cx = ev->cur.canvas.x - x;
        en->cy = ev->cur.canvas.y - y;
        if (!evas_textblock_cursor_char_coord_set(en->cursor, en->cx, en->cy))
          {
             Evas_Coord lx, ly, lw, lh;

             evas_textblock_cursor_line_coord_set(en->cursor, en->cy);
             evas_textblock_cursor_line_geometry_get(en->cursor, &lx, &ly, &lw, &lh);
             if (en->cx <= lx)
                _curs_lin_start(en->cursor, rp->object, en);
             else
                _curs_lin_end(en->cursor, rp->object, en);
          }
        if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_EXPLICIT)
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
           _edje_emit(rp->edje, "cursor,changed", rp->part->name);
        evas_textblock_cursor_free(tc);

#ifdef HAVE_ECORE_IMF
        if (en->imf_context)
          {
             ecore_imf_context_reset(en->imf_context);
             ecore_imf_context_cursor_position_set(en->imf_context,
                                                   evas_textblock_cursor_pos_get(en->cursor));
          }
#endif

        _edje_entry_real_part_configure(rp);
     }
}

static void
_evas_focus_in_cb(void *data, Evas *e, __UNUSED__ void *event_info)
{
   Edje *ed = (Edje *)data;

   if (evas_focus_get(e) == ed->obj) {
        _edje_focus_in_cb(data, NULL, NULL, NULL);
   }
}

static void
_evas_focus_out_cb(void *data, Evas *e, __UNUSED__ void *event_info)
{
   Edje *ed = (Edje *)data;

   if (evas_focus_get(e) == ed->obj) {
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
   evas_event_callback_add(ed->evas, EVAS_CALLBACK_CANVAS_FOCUS_IN, _evas_focus_in_cb, ed);
   evas_event_callback_add(ed->evas, EVAS_CALLBACK_CANVAS_FOCUS_OUT, _evas_focus_out_cb, ed);
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
   if (evas_event_callback_del_full(ed->evas, EVAS_CALLBACK_CANVAS_FOCUS_IN, _evas_focus_in_cb, ed) != ed)
      ERR("could not unregister EVAS_CALLBACK_FOCUS_IN");
   if (evas_event_callback_del_full(ed->evas, EVAS_CALLBACK_CANVAS_FOCUS_OUT, _evas_focus_out_cb, ed) != ed)
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

   en = calloc(1, sizeof(Entry));
   if (!en) return;
   rp->entry_data = en;
   en->rp = rp;

   evas_object_event_callback_add(rp->object, EVAS_CALLBACK_MOUSE_DOWN, _edje_part_mouse_down_cb, rp);
   evas_object_event_callback_add(rp->object, EVAS_CALLBACK_MOUSE_UP, _edje_part_mouse_up_cb, rp);
   evas_object_event_callback_add(rp->object, EVAS_CALLBACK_MOUSE_MOVE, _edje_part_mouse_move_cb, rp);

   if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_DEFAULT)
      en->select_allow = 1;

   if (rp->part->entry_mode == EDJE_ENTRY_EDIT_MODE_PASSWORD)
     {
        Edje_Part_Description_Text *txt;

        txt = (Edje_Part_Description_Text *) rp->chosen_description;

        en->select_allow = 0;
        if (txt && edje_string_get(&txt->text.repch))
           evas_object_textblock_replace_char_set(rp->object, edje_string_get(&txt->text.repch));
        else
           evas_object_textblock_replace_char_set(rp->object, "*");
     }

   en->cursor_bg = edje_object_add(rp->edje->evas);
   edje_object_file_set(en->cursor_bg, rp->edje->path, rp->part->source3);
   evas_object_smart_member_add(en->cursor_bg, rp->edje->obj);
   evas_object_stack_below(en->cursor_bg, rp->object);
   evas_object_clip_set(en->cursor_bg, evas_object_clip_get(rp->object));
   evas_object_pass_events_set(en->cursor_bg, EINA_TRUE);
   rp->edje->subobjs = eina_list_append(rp->edje->subobjs, en->cursor_bg);

   en->cursor_fg = edje_object_add(rp->edje->evas);
   edje_object_file_set(en->cursor_fg, rp->edje->path, rp->part->source4);
   evas_object_smart_member_add(en->cursor_fg, rp->edje->obj);
   evas_object_stack_above(en->cursor_fg, rp->object);
   evas_object_clip_set(en->cursor_fg, evas_object_clip_get(rp->object));
   evas_object_pass_events_set(en->cursor_fg, EINA_TRUE);
   rp->edje->subobjs = eina_list_append(rp->edje->subobjs, en->cursor_fg);

   if (rp->part->entry_mode >= EDJE_ENTRY_EDIT_MODE_EDITABLE)
     {
        evas_object_show(en->cursor_bg);
        evas_object_show(en->cursor_fg);
#ifdef HAVE_ECORE_IMF
        ecore_imf_init();

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

        ecore_imf_context_client_window_set(en->imf_context, rp->object);
        ecore_imf_context_client_canvas_set(en->imf_context, rp->edje->evas);

        ecore_imf_context_retrieve_surrounding_callback_set(en->imf_context, 
                                                            _edje_entry_imf_retrieve_surrounding_cb, rp);
        en->imf_ee_handler_commit = ecore_event_handler_add(ECORE_IMF_EVENT_COMMIT, _edje_entry_imf_event_commit_cb, rp->edje);
        en->imf_ee_handler_delete = ecore_event_handler_add(ECORE_IMF_EVENT_DELETE_SURROUNDING, _edje_entry_imf_event_delete_surrounding_cb, rp);
        en->imf_ee_handler_changed = ecore_event_handler_add(ECORE_IMF_EVENT_PREEDIT_CHANGED, _edje_entry_imf_event_preedit_changed_cb, rp->edje);
        ecore_imf_context_input_mode_set(en->imf_context, 
                                         rp->part->entry_mode == EDJE_ENTRY_EDIT_MODE_PASSWORD ? 
                                         ECORE_IMF_INPUT_MODE_INVISIBLE : ECORE_IMF_INPUT_MODE_FULL);
#endif
     }
done:
   en->cursor = (Evas_Textblock_Cursor *)evas_object_textblock_cursor_get(rp->object);
}

void
_edje_entry_real_part_shutdown(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   rp->entry_data = NULL;
   _sel_clear(en->cursor, rp->object, en);
   _anchors_clear(en->cursor, rp->object, en);
   rp->edje->subobjs = eina_list_remove(rp->edje->subobjs, en->cursor_bg);
   rp->edje->subobjs = eina_list_remove(rp->edje->subobjs, en->cursor_fg);
   evas_object_del(en->cursor_bg);
   evas_object_del(en->cursor_fg);

#ifdef HAVE_ECORE_IMF
   if (rp->part->entry_mode >= EDJE_ENTRY_EDIT_MODE_EDITABLE)
     {
        if (en->imf_context)
          {
             if (en->imf_ee_handler_commit)
               {
                  ecore_event_handler_del(en->imf_ee_handler_commit);
                  en->imf_ee_handler_commit = NULL;
               }

             if (en->imf_ee_handler_delete)
               {
                  ecore_event_handler_del(en->imf_ee_handler_delete);
                  en->imf_ee_handler_delete = NULL;
               }

             if (en->imf_ee_handler_changed) 
               {
                  ecore_event_handler_del(en->imf_ee_handler_changed);
                  en->imf_ee_handler_changed = NULL;
               }

             ecore_imf_context_del(en->imf_context);
             en->imf_context = NULL;
          }

        edje_object_signal_callback_del(rp->edje->obj, "focus,part,in", rp->part->name, _edje_entry_focus_in_cb);
        edje_object_signal_callback_del(rp->edje->obj, "focus,part,out", rp->part->name, _edje_entry_focus_out_cb);
        ecore_imf_shutdown();
     }
#endif

   free(en);
}

void
_edje_entry_real_part_configure(Edje_Real_Part *rp)
{
   Evas_Coord x, y, w, h, xx, yy, ww, hh;
   Entry *en = rp->entry_data;
   if (!en) return;

   _sel_update(en->cursor, rp->object, en);
   _anchors_update(en->cursor, rp->object, en);
   x = y = w = h = -1;
   xx = yy = ww = hh = -1;
   evas_object_geometry_get(rp->object, &x, &y, &w, &h);
   evas_textblock_cursor_geometry_get(en->cursor, &xx, &yy, &ww, &hh, NULL, EVAS_TEXTBLOCK_CURSOR_UNDER);
   if (ww < 1) ww = 1;
   if (hh < 1) ww = 1;
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
}

const char *
_edje_entry_selection_get(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
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
   Entry *en = rp->entry_data;
   if (!en) return NULL;
   // get text - convert to markup
   return evas_object_textblock_text_markup_get(rp->object);
}

void
_edje_entry_text_markup_set(Edje_Real_Part *rp, const char *text)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   // set text as markup
   _sel_clear(en->cursor, rp->object, en);
   evas_object_textblock_text_markup_set(rp->object, text); 

   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);
   _edje_entry_set_cursor_start(rp);
}

void
_edje_entry_text_markup_insert(Edje_Real_Part *rp, const char *text)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   // prepend markup @ cursor pos
   if (en->have_selection)
      _range_del(en->cursor, rp->object, en);
   _sel_clear(en->cursor, rp->object, en);
   //xx
//   evas_object_textblock_text_markup_prepend(en->cursor, text);
   _text_filter_markup_prepend(en, text);
   _curs_update_from_curs(en->cursor, rp->object, en);
   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);
   _edje_emit(rp->edje, "cursor,changed", rp->part->name);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif
   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_set_cursor_start(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   _curs_start(en->cursor, rp->object, en);
}

void
_edje_entry_set_cursor_end(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   _curs_end(en->cursor, rp->object, en);
}

void
_edje_entry_select_none(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   _sel_clear(en->cursor, rp->object, en);
}

void
_edje_entry_select_all(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   _sel_clear(en->cursor, rp->object, en);
   _curs_start(en->cursor, rp->object, en);
   _sel_enable(en->cursor, rp->object, en);
   _sel_start(en->cursor, rp->object, en);
   _curs_end(en->cursor, rp->object, en);
   _sel_extend(en->cursor, rp->object, en);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_select_begin(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   _sel_clear(en->cursor, rp->object, en);
   _sel_enable(en->cursor, rp->object, en);
   _sel_start(en->cursor, rp->object, en);
   _sel_extend(en->cursor, rp->object, en);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_select_extend(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (!en) return;
   _sel_extend(en->cursor, rp->object, en);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   _edje_entry_real_part_configure(rp);
}

const Eina_List *
_edje_entry_anchor_geometry_get(Edje_Real_Part *rp, const char *anchor)
{
   Entry *en = rp->entry_data;
   Eina_List *l;
   Anchor *an;

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
   Entry *en = rp->entry_data;
   Eina_List *l, *anchors = NULL;
   Anchor *an;

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
   Entry *en = rp->entry_data;
   Eina_List *l;
   Anchor *an;

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
   Entry *en = rp->entry_data;
   Eina_List *l, *items = NULL;
   Anchor *an;

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

void
_edje_entry_cursor_geometry_get(Edje_Real_Part *rp, Evas_Coord *cx, Evas_Coord *cy, Evas_Coord *cw, Evas_Coord *ch)
{
   Evas_Coord x, y, w, h, xx, yy, ww, hh;
   Entry *en = rp->entry_data;
   if (!en) return;

   x = y = w = h = -1;
   xx = yy = ww = hh = -1;
   evas_object_geometry_get(rp->object, &x, &y, &w, &h);
   evas_textblock_cursor_geometry_get(en->cursor, &xx, &yy, &ww, &hh, NULL, EVAS_TEXTBLOCK_CURSOR_UNDER);
   if (ww < 1) ww = 1;
   if (hh < 1) ww = 1;
   if (cx) *cx = x + xx;
   if (cy) *cy = y + yy;
   if (cw) *cw = ww;
   if (ch) *ch = hh;
}

void
_edje_entry_select_allow_set(Edje_Real_Part *rp, Eina_Bool allow)
{
   Entry *en = rp->entry_data;
   if (rp->part->select_mode == EDJE_ENTRY_SELECTION_MODE_DEFAULT)
      return;
   en->select_allow = allow;
}

Eina_Bool
_edje_entry_select_allow_get(const Edje_Real_Part *rp)
{
   const Entry *en = rp->entry_data;
   return en->select_allow;
}

void
_edje_entry_select_abort(Edje_Real_Part *rp)
{
   Entry *en = rp->entry_data;
   if (en->selecting)
     {
        en->selecting = EINA_FALSE;

#ifdef HAVE_ECORE_IMF
        if (en->imf_context)
          {
             ecore_imf_context_reset(en->imf_context);
             ecore_imf_context_cursor_position_set(en->imf_context,
                                                   evas_textblock_cursor_pos_get(en->cursor));
          }
#endif

        _edje_entry_real_part_configure(rp);
     }
}

static Evas_Textblock_Cursor *
_cursor_get(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en = rp->entry_data;
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
   Entry *en = rp->entry_data;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   if (!c) return EINA_FALSE;
   if (!evas_textblock_cursor_char_next(c))
     {
        return EINA_FALSE;
     }
   _curs_update_from_curs(c, rp->object, rp->entry_data);
   _sel_update(c, rp->object, rp->entry_data);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
   return EINA_TRUE;
}

Eina_Bool
_edje_entry_cursor_prev(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en = rp->entry_data;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   if (!c) return EINA_FALSE;
   if (!evas_textblock_cursor_char_prev(c))
     {
        if (evas_textblock_cursor_paragraph_prev(c)) goto ok;
        else return EINA_FALSE;
     }
ok:
   _curs_update_from_curs(c, rp->object, rp->entry_data);
   _sel_update(c, rp->object, rp->entry_data);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
   return EINA_TRUE;
}

Eina_Bool
_edje_entry_cursor_up(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en = rp->entry_data;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   Evas_Coord lx, ly, lw, lh, cx, cy, cw, ch;
   int ln;
   if (!c) return EINA_FALSE;
   ln = evas_textblock_cursor_line_geometry_get(c, NULL, NULL, NULL, NULL);
   ln--;
   if (ln < 0) return EINA_FALSE;
   if (!evas_object_textblock_line_number_geometry_get(rp->object, ln, 
                                                       &lx, &ly, &lw, &lh))
      return EINA_FALSE;
   evas_textblock_cursor_char_geometry_get(c, &cx, &cy, &cw, &ch);
   if (!evas_textblock_cursor_char_coord_set(c, cx, ly + (lh / 2)))
     {
        if (cx < (lx +(lw / 2)))
           evas_textblock_cursor_line_char_last(c);
        else
           evas_textblock_cursor_line_char_last(c);
     }
   _curs_update_from_curs(c, rp->object, rp->entry_data);
   _sel_update(c, rp->object, rp->entry_data);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
   return EINA_TRUE;
}

Eina_Bool
_edje_entry_cursor_down(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en = rp->entry_data;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   Evas_Coord lx, ly, lw, lh, cx, cy, cw, ch;
   int ln;
   if (!c) return EINA_FALSE;
   ln = evas_textblock_cursor_line_geometry_get(c, NULL, NULL, NULL, NULL);
   ln++;
   if (!evas_object_textblock_line_number_geometry_get(rp->object, ln, 
                                                       &lx, &ly, &lw, &lh))
      return EINA_FALSE;
   evas_textblock_cursor_char_geometry_get(c, &cx, &cy, &cw, &ch);
   if (!evas_textblock_cursor_char_coord_set(c, cx, ly + (lh / 2)))
     {
        if (cx < (lx +(lw / 2)))
           evas_textblock_cursor_line_char_last(c);
        else
           evas_textblock_cursor_line_char_last(c);
     }
   _curs_update_from_curs(c, rp->object, rp->entry_data);
   _sel_update(c, rp->object, rp->entry_data);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
   return EINA_TRUE;
}

void
_edje_entry_cursor_begin(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en = rp->entry_data;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   if (!c) return;
   evas_textblock_cursor_paragraph_first(c);
   _curs_update_from_curs(c, rp->object, rp->entry_data);
   _sel_update(c, rp->object, rp->entry_data);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_cursor_end(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en = rp->entry_data;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   if (!c) return;
   _curs_end(c, rp->object, rp->entry_data);
   _sel_update(c, rp->object, rp->entry_data);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_cursor_copy(Edje_Real_Part *rp, Edje_Cursor cur, Edje_Cursor dst)
{
   Entry *en = rp->entry_data;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   if (!c) return;
   Evas_Textblock_Cursor *d = _cursor_get(rp, dst);
   if (!d) return;
   evas_textblock_cursor_copy(c, d);
   _curs_update_from_curs(c, rp->object, rp->entry_data);
   _sel_update(c, rp->object, rp->entry_data);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_cursor_line_begin(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en = rp->entry_data;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   if (!c) return;
   evas_textblock_cursor_line_char_first(c);
   _curs_update_from_curs(c, rp->object, rp->entry_data);
   _sel_update(c, rp->object, rp->entry_data);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

   _edje_emit(rp->edje, "cursor,changed", rp->part->name);
   _edje_entry_real_part_configure(rp);
}

void
_edje_entry_cursor_line_end(Edje_Real_Part *rp, Edje_Cursor cur)
{
   Entry *en = rp->entry_data;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);
   if (!c) return;
   evas_textblock_cursor_line_char_last(c);
   _curs_update_from_curs(c, rp->object, rp->entry_data);
   _sel_update(c, rp->object, rp->entry_data);

#ifdef HAVE_ECORE_IMF
   if (en->imf_context)
     {
        ecore_imf_context_reset(en->imf_context);
        ecore_imf_context_cursor_position_set(en->imf_context,
                                              evas_textblock_cursor_pos_get(en->cursor));
     }
#endif

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

const char *
_edje_entry_cursor_content_get(Edje_Real_Part *rp, Edje_Cursor cur)
{
   static char *s = NULL;
   Evas_Textblock_Cursor *c = _cursor_get(rp, cur);

   if (s)
     {
        free(s);
        s = NULL;
     }

   s = evas_textblock_cursor_content_get(c);
   return s;
}

#ifdef HAVE_ECORE_IMF
static Eina_Bool
_edje_entry_imf_retrieve_surrounding_cb(void *data, Ecore_IMF_Context *ctx __UNUSED__, char **text, int *cursor_pos)
{
   Edje_Real_Part *rp = data;
   Entry *en;
   const char *str;

   if (!rp) return EINA_FALSE;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
      return EINA_FALSE;

   if (text)
     {
        str = _edje_entry_text_get(rp);
        *text = str ? strdup(str) : strdup("");
     }

   if (cursor_pos)
     *cursor_pos = evas_textblock_cursor_pos_get(en->cursor);

   return EINA_TRUE;
}

static Eina_Bool
_edje_entry_imf_event_commit_cb(void *data, int type __UNUSED__, void *event)
{
   Edje* ed = data;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en;
   Ecore_IMF_Event_Commit *ev = event;
   int i;

   if (!rp) return ECORE_CALLBACK_PASS_ON;

   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
      return ECORE_CALLBACK_PASS_ON;

   if (en->imf_context != ev->ctx) return ECORE_CALLBACK_PASS_ON;

   if (en->have_selection)
     {
        _range_del(en->cursor, rp->object, en);
        _sel_clear(en->cursor, rp->object, en);
     }

   if (en->have_composition)
     {
        for (i = 0; i < en->comp_len; i++)
           _backspace(en->cursor, rp->object, en);
        en->have_composition = EINA_FALSE;
     }

   //yy
//   evas_textblock_cursor_text_prepend(en->cursor, ev->str);
   _text_filter_text_prepend(en, ev->str);

   _curs_update_from_curs(en->cursor, rp->object, en);
   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);
   _edje_emit(ed, "cursor,changed", rp->part->name);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_edje_entry_imf_event_preedit_changed_cb(void *data, int type __UNUSED__, void *event)
{
   Edje* ed = data;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en;
   int length;
   Ecore_IMF_Event_Commit *ev = event;
   int i;
   char *preedit_string;

   if (!rp) return ECORE_CALLBACK_PASS_ON;

   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
      return ECORE_CALLBACK_PASS_ON;

   if (!en->imf_context) return ECORE_CALLBACK_PASS_ON;

   if (en->imf_context != ev->ctx) return ECORE_CALLBACK_PASS_ON;

   ecore_imf_context_preedit_string_get(en->imf_context, &preedit_string, &length);

   // FIXME : check the maximum length of evas_textblock
   if ( 0 /* check the maximum length of evas_textblock */ )
      return ECORE_CALLBACK_PASS_ON;

   if (en->have_selection)
     {
        _range_del(en->cursor, rp->object, en);
        _sel_clear(en->cursor, rp->object, en);
     }

   if (en->have_composition)
     {
        // delete the composing characters
        for (i = 0;i < en->comp_len; i++)
           _backspace(en->cursor, rp->object, en);
     }

   en->comp_len = length;
   en->have_composition = EINA_TRUE;

   //xx
//   evas_object_textblock_text_markup_prepend(en->cursor, preedit_string);
   _text_filter_markup_prepend(en, preedit_string);

   _curs_update_from_curs(en->cursor, rp->object, en);
   _anchors_get(en->cursor, rp->object, en);
   _edje_emit(rp->edje, "entry,changed", rp->part->name);
   _edje_emit(ed, "cursor,changed", rp->part->name);

   free(preedit_string);

   return ECORE_CALLBACK_DONE;
}

static Eina_Bool
_edje_entry_imf_event_delete_surrounding_cb(void *data, int type __UNUSED__, void *event)
{
   Edje *ed = data;
   Edje_Real_Part *rp = ed->focused_part;
   Entry *en;
   Ecore_IMF_Event_Delete_Surrounding *ev = event;

   if (!rp) return ECORE_CALLBACK_PASS_ON;
   en = rp->entry_data;
   if ((!en) || (rp->part->type != EDJE_PART_TYPE_TEXTBLOCK) ||
       (rp->part->entry_mode < EDJE_ENTRY_EDIT_MODE_SELECTABLE))
      return ECORE_CALLBACK_PASS_ON;

   if (en->imf_context != ev->ctx) return ECORE_CALLBACK_PASS_ON;

   return ECORE_CALLBACK_DONE;
}
#endif

/* vim:set ts=8 sw=3 sts=3 expandtab cino=>5n-2f0^-2{2(0W1st0 :*/
