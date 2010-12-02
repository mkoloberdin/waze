	/* roadmap_history.c - manage the roadmap address history.
	 *
	 * LICENSE:
	 *
	 *   Copyright 2002 Pascal F. Martin
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
	 *   See roadmap_history.h
	 */

	#include <string.h>
	#include <stdlib.h>

	#include "roadmap.h"
	#include "roadmap_config.h"
	#include "roadmap_path.h"
	#include "roadmap_file.h"
	#include "roadmap_utf8.h"

	#include "roadmap_history.h"


	static int RoadMapHistoryCategories[128];


	static RoadMapConfigDescriptor RoadMapConfigHistoryDepth =
	                        ROADMAP_CONFIG_ITEM("History", "Depth");

	struct roadmap_history_log_entry {

	   struct roadmap_history_log_entry *before;
	   struct roadmap_history_log_entry *after;

	   char category;
	   char data[1];
	};


	static int RoadMapHistoryChanged = 0;
	static int RoadMapHistoryCount = 0;
	static struct roadmap_history_log_entry *RoadMapLatest = NULL;
	static struct roadmap_history_log_entry *RoadMapOldest = NULL;
	static void roadmap_history_get_from_entry (struct roadmap_history_log_entry *entry, int argc, char *argv[]);

	 void roadmap_history_remove_entry
	               (struct roadmap_history_log_entry *entry) {

	    if (entry->after != NULL) {
	        entry->after->before = entry->before;
	    } else {
	        if (entry != RoadMapLatest) {
	            roadmap_log (ROADMAP_FATAL, "invalid lastest entry");
	        }
	        RoadMapLatest = entry->before;
	    }

	    if (entry->before != NULL) {
	        entry->before->after = entry->after;
	    } else {
	        if (entry != RoadMapOldest) {
	            roadmap_log (ROADMAP_FATAL, "invalid oldest entry");
	        }
	        RoadMapOldest = entry->after;
	    }

	    RoadMapHistoryCount -= 1;
	    RoadMapHistoryChanged = 1;
	}


static void roadmap_history_add_entry (char category, const char *data) {

	   struct roadmap_history_log_entry *entry = NULL;
	   BOOL needToRemoveEntry = FALSE;
	   char * streetNum;
	   char * streetName;
	   char * cityName;
	   char * stateName;
	   char * nick;
	   char * latitude;
	   char * longitutde;
	   char * dataArgs[7];
	   char * newData;
	   struct roadmap_history_log_entry * tempEntry = NULL;

	   if (RoadMapLatest != NULL) {

	      if ((RoadMapLatest->category == category) &&
	          (strcasecmp (data, RoadMapLatest->data) == 0)) {
	         return; /* Same entry as before. */
	      }

	      for (entry = RoadMapLatest->before;
	           entry != NULL;
	           entry = entry->before) {

	        if ((entry->category == category) &&
	            (strcasecmp (data, entry->data) == 0)) {

	            roadmap_history_remove_entry (entry);
	            break;
	        }
	      }
	   }

	   if (entry == NULL) {

	       entry = malloc (strlen(data) +
	                       sizeof(struct roadmap_history_log_entry));
	       roadmap_check_allocated(entry);

	       entry->category = category;

	       strcpy (entry->data, data);
	   }


	   roadmap_history_get_from_entry(entry,7,dataArgs); // start of code added by me ( dan )
	   streetNum = strdup(dataArgs[0]);
	   streetName = strdup(dataArgs[1]);
	   cityName = strdup(dataArgs[2]);
	   stateName = strdup(dataArgs[3]);
	   nick = strdup(dataArgs[4]);
	   latitude = strdup(dataArgs[5]);
	   longitutde = strdup(dataArgs[6]);
	   for (tempEntry = RoadMapLatest; tempEntry != NULL; tempEntry = tempEntry->before)
	   {
	    	char * dataArgsTemp[7];
	    	roadmap_history_get_from_entry(tempEntry,7,dataArgsTemp);
	    	if( (strcasecmp(streetNum,dataArgsTemp[0])==0) && //same street num
	    	    (strcasecmp(streetName,dataArgsTemp[1])==0) && // same street name
	    	    (strcasecmp(cityName,dataArgsTemp[2])==0) && // same city name
	    	    (strcasecmp(stateName,dataArgsTemp[3])==0) ) // same state name
	    	    {
	    	        // this patch takes care of the problem when there was a favorite without coordinates ( old version )
	    	        // and each time its address was searched again. after this patch it will have coordinates
	    			if((tempEntry->category=='F')&&(strcmp(dataArgsTemp[5],"")==0)&&(strcmp(latitude,"")!=0) )
	    			{ // favorite with                   no latitude,                 new one with latitude:    need to remove and readd with latitude
		    		    newData = (char *) malloc ( strlen(tempEntry->data) + strlen(latitude) + strlen(longitutde)+2*strlen(","));
		    		    strcpy(newData,tempEntry->data);  // build the new string
		    		    strcat(newData,",");
		    		    strcat(newData,latitude);
		    		    strcat(newData,",");
			    	    strcat(newData,longitutde);
			    		roadmap_history_remove_entry(tempEntry);
			    		free(tempEntry);
			    		roadmap_history_add_entry('F',newData);
			    		free(newData);
			    		break;
	    			}

	    			// this patch takes care of the problem where a POI would be added again if clicked on in history
	    			if((tempEntry->category=='A')&&(strcmp(dataArgsTemp[4],"")!=0)&&(category=='A')) // nick exists and it's in history - its a POI.
	    			{                                               // so we need to add it to the top, and not add the current one
						//move this entry to top
						roadmap_history_remove_entry(tempEntry);
						roadmap_history_add_entry('A',tempEntry->data);
			    		free(tempEntry);
			    		needToRemoveEntry = TRUE;
			    		break;
	    			}
	    		}
	   }
       // free the relevant resources, end of code added by me ( dan )
	   free(streetNum); free(streetName); free(cityName); free(stateName); free(nick); free(latitude); free(longitutde);

	   entry->before = RoadMapLatest;
	   entry->after  = NULL;

	   if (RoadMapLatest != NULL) {
	       RoadMapLatest->after = entry;
	   } else {
	       RoadMapOldest = entry;
	   }

	   RoadMapLatest = entry;
	   RoadMapHistoryCount += 1;

	   if (RoadMapHistoryCount >
	       roadmap_config_get_integer(&RoadMapConfigHistoryDepth)) {

	      entry = RoadMapOldest;
	      if (entry->category == 'A'){
	         roadmap_history_remove_entry (entry);
	         free (entry);
	      }
	   }
	   else if (needToRemoveEntry==TRUE)
	   {
	   	   roadmap_history_remove_entry (entry);
	   	   free(entry);
	   }
	   RoadMapHistoryChanged = 1;
	}


	static void roadmap_history_get_from_entry
	               (struct roadmap_history_log_entry *entry,
	                int argc,
	                char *argv[]) {

	   static char data[1024];

	   int   i;
	   char *p;


	   if (entry != NULL) {

	      strncpy_safe (data, entry->data, sizeof(data));

	      argv[0] = data;
	      p = strchr (data, ',');

	      for (i = 1; i < argc && p != NULL; ++i) {

	         *p = 0;
	         argv[i] = ++p;
	         p = strchr (p, ',');
	      }

	   } else {

	      i = 0;
	   }

	   while (i < argc) argv[i++] = "";
	}


	static void roadmap_history_save_entries
	               (FILE *file, struct roadmap_history_log_entry *entry) {

	   if (entry->before != NULL) {
	      roadmap_history_save_entries (file, entry->before);
	   }
	   fprintf (file, "%c,%s\n", entry->category, entry->data);
	}


	void roadmap_history_initialize (void) {

	   roadmap_config_declare
	      ("preferences", &RoadMapConfigHistoryDepth, "30", NULL);
	}


	void roadmap_history_load (void) {

	   static int loaded = 0;

	   FILE *file;
	   char *p;
	   char  line[1024];


	   if (loaded) return;

	   file = roadmap_file_fopen (roadmap_path_user(), "history", "sr");

	   if (file != NULL) {

	      while (! feof(file)) {

	         if (fgets (line, sizeof(line), file) == NULL) break;

	         p = roadmap_config_extract_data (line, sizeof(line));

	         if (p == NULL) continue;

	         if ((p[1] != ',') || (p[0] < 'A') || (p[0] > 'Z')) {

	             /* Compatibility wih existing history files. */
	             roadmap_history_add_entry ('A', p);

	         } else {
	             roadmap_history_add_entry (p[0], p+2);
	         }
	      }

	      fclose (file);
	   }

	   loaded = 1;
	   RoadMapHistoryChanged = 0;
	}


	void roadmap_history_declare (char category, int argv) {

	   if (category < 'A' || category > 'Z') {
	      roadmap_log (ROADMAP_FATAL, "invalid category '%c'", category);
	   }
	   if (argv <= 0) {
	      roadmap_log (ROADMAP_FATAL, "invalid count for category '%c'", category);
	   }
	   //if (RoadMapHistoryCategories[(int)category] > 0) {
	      //roadmap_log (ROADMAP_FATAL, "category '%c' declared twice", category);
	  // }
	   RoadMapHistoryCategories[(int)category] = argv;
	}

	void remove_commas(char *str){
	   char * pch;

	   if (!str)
	      return;
	   pch=strchr(str,',');
	   while (pch!=NULL)
	   {
	       str[pch-str] = '.';
	       pch=strchr(pch+1,',');
	   }
	}

	void roadmap_history_add (char category, const char *argv[]) {

	   char data[1024];
	   int i;
	   unsigned length;

	   int argc = RoadMapHistoryCategories[(int)category];

	   if (argc <= 0) {
	      roadmap_log (ROADMAP_FATAL, "category '%c' was not declared", category);
	   }

	   length = 1;
	   data[0] = 0;

	   for (i = 0; i < argc; ++i) {

	      length += strlen(argv[i]) + 1; /* Length in bytes, not in UTF characters */
	      if (length >= sizeof(data)) {
	         roadmap_log (ROADMAP_FATAL, "history entry is too long");
	      }

	      strcat (data, ",");
	      remove_commas((char *)argv[i]);
	      strcat (data, argv[i]);
	   }
	   roadmap_history_add_entry (category, data+1);
	}

	void roadmap_history_update(void *cursor, char category, char *argv[]){
	   char data[1024];
	   int i;
	   unsigned length;
	   int argc;
	   struct roadmap_history_log_entry *entry = cursor;

         if (cursor == NULL)
            return;

	      argc = RoadMapHistoryCategories[(int)category];

	      if (argc <= 0) {
	         roadmap_log (ROADMAP_FATAL, "category '%c' was not declared", category);
	      }

	      length = 1;
	      data[0] = 0;

	      for (i = 0; i < argc; ++i) {

	         length += strlen(argv[i]) + 1; /* Length in bytes, not in UTF characters */
	         if (length >= sizeof(data)) {
	            roadmap_log (ROADMAP_FATAL, "history entry is too long");
	         }

	         strcat (data, ",");
	         remove_commas((char *)argv[i]);
	         strcat (data, argv[i]);
	      }
	      strcpy (entry->data, data+1);
	      RoadMapHistoryChanged = 1;
	}

	void *roadmap_history_latest (char category) {

	   return roadmap_history_before (category, NULL);
	}



	void *roadmap_history_before (char category, void *cursor) {

	   struct roadmap_history_log_entry fake;
	   struct roadmap_history_log_entry *entry;

	   if (cursor == NULL) {
	      fake.before = RoadMapLatest;
	      entry = &fake;
	   } else {
	      entry = (struct roadmap_history_log_entry *)cursor;
	   }

	   /* Move backward until we find an entry of the requested category.
	    * If we cannot find any, don't move.
	    */
	   while (entry != NULL) {

	      if (entry->before == NULL) {
	         return cursor; /* Nothing before this starting point. */
	      }
	      entry = entry->before;
	      if (entry->category == category) break;
	   }

	   if (entry == &fake) {
	      return NULL;
	   }
	   return entry;
	}


	void *roadmap_history_after  (char category, void *cursor) {

	   struct roadmap_history_log_entry *entry =
	             (struct roadmap_history_log_entry *)cursor;


	   /* We move forward until we found an entry of the same category.  */

	   while (entry != NULL) {
	      entry = entry->after;
	      if (entry != NULL && entry->category == category) break;
	   }
	   return entry;
	}


	void roadmap_history_get (char category, void *cursor, char *argv[]) {

	   int argc = RoadMapHistoryCategories[(int)category];

	   if (argc <= 0) {
	      roadmap_log (ROADMAP_FATAL, "category '%c' was not declared", category);
	   }

	   roadmap_history_get_from_entry (cursor, argc, argv);
	}


	void roadmap_history_purge (int count) {

	   struct roadmap_history_log_entry *entry;

	   while (RoadMapHistoryCount > count) {
	       entry = RoadMapOldest;
	       roadmap_history_remove_entry (entry);
	       free(entry);
	       RoadMapHistoryChanged = 1;
	   }
	}


	void roadmap_history_save (void) {

	   FILE *file;

	   if (RoadMapLatest == NULL) return; /* Nothing to save. */

	   if (! RoadMapHistoryChanged) return; /* Nothing new to save. */

	   file = roadmap_file_fopen (roadmap_path_user(), "history", "w");

	   if (file != NULL) {

	      roadmap_history_save_entries (file, RoadMapLatest);

	      fclose (file);
	   }
	   RoadMapHistoryChanged = 0;
	}

	void roadmap_history_delete_entry  (void* cursor)
	{
		struct roadmap_history_log_entry *entry = cursor;
		roadmap_history_remove_entry(entry);
	}
