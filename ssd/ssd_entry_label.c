/* ssd_entry_label.c
 *
 * LICENSE:
 *
 *   Copyright 2010 Alex Agranovich (AGA),  Waze Ltd.
 *
 *   This file is part of RoadMap.
 *
 *   RoadMap is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   RoadMap is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with RoadMap; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * SYNOPSYS:
 *
 *   See ssd_entry_label.h.
 */

#include "ssd_entry.h"
#include "ssd_entry_label.h"
#include "ssd_container.h"
#include "ssd_text.h"
#include "roadmap_screen.h"
#include "roadmap_lang.h"


static const char *get_value (SsdWidget widget);
static int set_value (SsdWidget widget, const char *value );
static int edit_callback( SsdWidget widget, const char *new_value );
static SsdWidget create_widget ( const char *name, const char* label_text, int label_font, int label_width,
                                    int height, int flags, const char *background_text, const char *messagebox_text );

/***********************************************************
 *  Name        : ssd_entry_label_new
 *  Purpose     : Labeled entry initializer
 *  Params      :
 *              :
 *  Returns     :
 *  Notes       :
 */
SsdWidget ssd_entry_label_new ( const char *name, const char* label_text, int label_font, int label_width,
                                                         int height, int flags, const char *background_text )
{
   SsdWidget entry_label;
   entry_label = create_widget( name, label_text, label_font, label_width, height, flags, background_text, NULL );
   return entry_label;
}


SsdWidget ssd_entry_label_new_confirmed( const char *name, const char* label_text, int label_font, int label_width,
                                                         int height, int flags, const char *background_text, const char *messagebox_text )
{
   SsdWidget entry_label;
   entry_label = create_widget( name, label_text, label_font, label_width, height, flags, background_text, messagebox_text );
   return entry_label;
}


void ssd_entry_label_set_entry_flags( SsdWidget widget, int entry_flags )
{
   SsdWidget entry = ssd_widget_get( widget, SSD_ENTRY_LABEL_ENTRY );
   entry->flags = entry_flags;
}

void ssd_entry_label_set_text_flags( SsdWidget widget, int text_flags )
{
   SsdWidget entry = ssd_widget_get( widget, SSD_ENTRY_LABEL_ENTRY );
   SsdWidget text = ssd_widget_get( entry, "Text" );
   text->flags = text_flags;
}

void ssd_entry_label_set_label_flags( SsdWidget widget, int label_flags )
{
   SsdWidget label = ssd_widget_get( widget, SSD_ENTRY_LABEL_LBL );
   label->flags = label_flags;

}

SsdWidget ssd_entry_label_get_entry( SsdWidget entry_label )
{
   SsdWidget entry = ssd_widget_get( entry_label, SSD_ENTRY_LABEL_ENTRY );
   return entry;
}

void ssd_entry_label_set_kb_params( SsdWidget widget, const char* kb_title, const char* kb_label,
                                    const char* kb_note, CB_OnKeyboardDone kb_post_done_cb, int kb_flags )
{
   SsdWidget entry = ssd_widget_get( widget, SSD_ENTRY_LABEL_ENTRY );
   ssd_entry_set_kb_params( entry, kb_title, kb_label, kb_note, kb_post_done_cb, kb_flags );
}

void ssd_entry_label_set_kb_title( SsdWidget widget, const char* kb_title )
{
   SsdWidget entry = ssd_widget_get( widget, SSD_ENTRY_LABEL_ENTRY );
   ssd_entry_set_kb_title( entry, kb_title );
}

void ssd_entry_label_set_editbox_title( SsdWidget widget, const char* editbox_title )
{
   SsdWidget entry = ssd_widget_get( widget, SSD_ENTRY_LABEL_ENTRY );
   ssd_entry_set_editbox_title( entry, editbox_title );
}

void ssd_entry_label_set_label_color( SsdWidget widget, const char* color )
{
   SsdWidget label = ssd_widget_get( widget, SSD_ENTRY_LABEL_LBL );
   ssd_text_set_color( label, color );
}

void ssd_entry_label_set_label_offset( SsdWidget widget, int offset )
{
   SsdWidget offset_cnt = ssd_widget_get( widget, SSD_ENTRY_LABEL_OFFSET_CNT );
   offset_cnt->size.width = ADJ_SCALE( offset );
}


static SsdWidget create_widget ( const char *name, const char* label_text, int label_font, int label_width,
                                                         int height, int flags, const char *background_text, const char *messagebox_text )
{
   SsdWidget group, entry, entry_cnt;
   SsdWidget offset_cnt, label_cnt, label;

   group = ssd_container_new ( name, NULL, SSD_MAX_SIZE, ADJ_SCALE( height ), SSD_WIDGET_SPACE|flags );
   ssd_widget_set_color ( group, NULL, NULL );
   label_cnt = ssd_container_new ( SSD_ENTRY_LABEL_LBL_CNT, NULL, ADJ_SCALE( label_width ), SSD_MIN_SIZE, SSD_ALIGN_VCENTER );
   ssd_widget_set_color ( label_cnt, NULL, NULL );
   /* Offset for the label. Can be adjusted after dialog creation */
   offset_cnt = ssd_container_new ( SSD_ENTRY_LABEL_OFFSET_CNT, NULL, 1, ADJ_SCALE( height ), SSD_ALIGN_VCENTER );
   ssd_widget_set_color ( offset_cnt, NULL, NULL );
   ssd_widget_add( label_cnt, offset_cnt );

   label = ssd_text_new( SSD_ENTRY_LABEL_LBL, label_text, label_font,
                                                         SSD_TEXT_LABEL | SSD_ALIGN_VCENTER | SSD_TEXT_NORMAL_FONT );
   ssd_text_set_color(label, SSD_CONTAINER_TEXT_COLOR);
   ssd_widget_add( label_cnt, label );
   ssd_widget_add( group, label_cnt );

   // Separation space
   ssd_dialog_add_hspace( group, ADJ_SCALE( SSD_ENTRY_LABEL_SEP_SPACE ), SSD_ALIGN_VCENTER );

   entry_cnt = ssd_container_new ( SSD_ENTRY_LABEL_ENTRY_CNT, NULL, SSD_MAX_SIZE, ADJ_SCALE( height ),
                                                                                 SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT );
   ssd_widget_set_color ( entry_cnt, NULL, NULL );
   if ( messagebox_text )
   {
      entry = ssd_confirmed_entry_new ( SSD_ENTRY_LABEL_ENTRY, "", SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE, 0, SSD_MAX_SIZE,
                                                                                 ADJ_SCALE( height ), messagebox_text, background_text );
   }
   else
   {
      entry = ssd_entry_new ( SSD_ENTRY_LABEL_ENTRY, "", SSD_ALIGN_VCENTER|SSD_WIDGET_SPACE, 0, SSD_MAX_SIZE,
                                                                                      ADJ_SCALE( height ), background_text );
   }
   // Right margin space
   ssd_dialog_add_hspace( entry_cnt, ADJ_SCALE( SSD_ENTRY_LABEL_RIGHT_MARGIN ), SSD_ALIGN_VCENTER|SSD_ALIGN_RIGHT );
   ssd_widget_add( entry_cnt, entry );
   ssd_widget_add( group, entry_cnt );

   group->get_value = get_value;
   group->set_value = set_value;
   group->callback = edit_callback;

   return group;
}


static int edit_callback( SsdWidget widget, const char *new_value )
{
   SsdWidget entry = ssd_widget_get( widget, SSD_ENTRY_LABEL_ENTRY );
   SsdWidget text = ssd_widget_get( entry, "text_box" );
   return text->callback( text, new_value );
}

static const char *get_value (SsdWidget widget) {
   SsdWidget entry = ssd_widget_get( widget, SSD_ENTRY_LABEL_ENTRY );
   return ssd_widget_get_value ( entry, "Text" );
}

static int set_value ( SsdWidget widget, const char *value ) {
   int ret_val = ssd_widget_set_value( widget, SSD_ENTRY_LABEL_ENTRY, value );
   return ret_val;
}

