/* roadmap_softkeys.c - manage the softkeys
 *
 * LICENSE:
 *
 *   Copyright 2008 AVi B.S
 *   Copyright 2008 Ehud Shabtai
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
 *   See roadmap_softkeys.h
 */
                                                
#include <string.h>
#include <stdlib.h>

#include "roadmap.h"
#include "roadmap_softkeys.h"
#include "roadmap_lang.h"

#define MAX_SOFTKEYS 30
typedef struct {
	char * name;
	Softkey sk;
}SoftKeyEntry;

SoftKeyEntry right_soft_key[MAX_SOFTKEYS];
SoftKeyEntry left_soft_key[MAX_SOFTKEYS];

 
#ifndef __SYMBIAN32__
int roadmap_softkeys_orientation(){
	return SOFT_KEYS_ON_BOTTOM;
}
#endif

static void queue_softkey (const char *name, Softkey *s, SoftKeyEntry *table) {
   int i;
   BOOL found = 0;
   
   for (i=MAX_SOFTKEYS -1; i>=0; i--) {
   	  
      if (table[i].name && !strcmp(table[i].name, name)){
      	 found = 1;
      	 break;
      }
   }

 
   if (found)
   	return;
   
   for (i=0; i<MAX_SOFTKEYS; i++) {
      if (table[i].name == NULL) break;
   }

   if (i == MAX_SOFTKEYS){
      roadmap_log (ROADMAP_ERROR, "Too many softkeys");
      return;
   }

  
   strcpy(table[i].sk.text,s->text);
   table[i].sk.callback = s->callback;
   table[i].name = strdup(name);
}

static void remove_softkey (const char *name, SoftKeyEntry *table) {

   int i;
   BOOL found = FALSE;
   
   for (i=MAX_SOFTKEYS -1; i>=0; i--) {
   	  
      if (table[i].name && !strcmp(table[i].name, name)) {
      	found = TRUE;
      	break;
      }
   }

   if (!found) {
      roadmap_log (ROADMAP_DEBUG, "Can't find a softkey to remove. softkey: %s", name);
      return;
   }

   table[i].sk.callback = NULL;
   table[i].sk.text[0] = 0;
   if (table[i].name)
      free(table[i].name);
   table[i].name = NULL;
   
   if ( (i!=MAX_SOFTKEYS-1) &&  (table[i+1].name) ){ // this is not the last soft key - the next one isn't null
   		SoftKeyEntry nextSKEntry; 
   		nextSKEntry.name = table[i+1].name;
   		nextSKEntry.sk = table[i+1].sk;
   		table[i].sk.callback = table[i+1].sk.callback; // so we need to copy all the next ones backward by one 
   		strcpy(table[i].sk.text,table[i+1].sk.text);
   		table[i].name = strdup(table[i+1].name);
   		remove_softkey(nextSKEntry.name,table); // recursive function
   }
}

void roadmap_softkeys_set_left_soft_key(const char *name, Softkey *s ){
	queue_softkey(name, s, left_soft_key);
}

void roadmap_softkeys_set_right_soft_key(const char *name, Softkey *s){
	queue_softkey(name, s, right_soft_key);
}

void roamdap_softkeys_chnage_left_text(const char *name, const char *new_text){
   int i;
   BOOL found = FALSE;
   
   for (i=MAX_SOFTKEYS -1; i>=0; i--) {
   	  
      if (left_soft_key[i].name && !strcmp(left_soft_key[i].name, name)) {
      	found = TRUE;
      	break;
      }
   }

   if (!found) {
      return;
   }

    strcpy(left_soft_key[i].sk.text,new_text);
}

void roadmap_softkeys_remove_left_soft_key(const char *name ){
	remove_softkey(name, left_soft_key);
}

void roadmap_softkeys_remove_right_soft_key(const char *name){
	remove_softkey(name, right_soft_key);
}

const char *roadmap_softkeys_get_left_soft_key_text(){
   int i = MAX_SOFTKEYS-1;
   const char * res = NULL;

   while (i>=0) {
      if (left_soft_key[i].name){
      	res = left_soft_key[i].sk.text;
      	if (res) break;
      }
      i--;
   }
   
   if (res)	
   		return roadmap_lang_get(res);
   else
   		return "";
}

const char *roadmap_softkeys_get_right_soft_key_text(){
   int i = MAX_SOFTKEYS-1;
   const char * res = NULL;

   while (i>=0){
   	  if (right_soft_key[i].name){
      	res = right_soft_key[i].sk.text;
      	if (res) break;
   	  }
      i--;
   }
	
	if (res)	
   		return roadmap_lang_get(res);
   	else
   		return "";
}

void roadmap_softkeys_left_softkey_callback(){
   int i = MAX_SOFTKEYS-1;
   RoadMapCallback callback = NULL;

   while (i>=0){
   	  if (left_soft_key[i].name){
      	callback = left_soft_key[i].sk.callback;
      	if (callback) break;
   	  }
      i--;
   }
   
   if (callback)
   	(*callback)();
}

void roadmap_softkeys_right_softkey_callback(){
   int i = MAX_SOFTKEYS-1;
   RoadMapCallback callback = NULL;

   while (i>=0){
   	  if (right_soft_key[i].name){
      	callback = right_soft_key[i].sk.callback;
      	if (callback) break;
   	  }
      i--;
   }
   
   if (callback)
   	(*callback)();
}
