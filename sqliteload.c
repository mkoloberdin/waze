/* sqliteload.c - Load a CSV file into a sqlite database
 *
 * LICENSE:
 *
 * Copyright 2002 Pascal F. Martin
 *
 * This file is part of RoadMap.
 *
 * RoadMap is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * RoadMap is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with RoadMap; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * DESCRIPTION:
 *
 * This program allows for the loading of data in the CSV format to
 * an sqlite database.
 *
 * The CSV format expected is an inital line that identifies the columns
 * followed by record lines, where each record line contains a record to
 * be loaded into the sqlite database.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <errno.h>

#include <sqlite.h>


static const char SINGLE_QUOTE[] = "'";

static int verbose = 0;
static const char *argv0 = NULL;
static sqlite *load_db = NULL;


static void usage (const char *object, int line, const char *error) {

   if (object == NULL) {
      fprintf(stderr, "** %s\n", error);
   } else if (line <= 0) {
      fprintf(stderr, "** %s: %s\n", object, error);
   } else {
      fprintf(stderr, "** %s, line %d: %s\n", object, line, error);
   }

   fprintf(stderr,
           "Usage: %s [--verbose[=N]] [--clean] database table csv-file..\n",
           argv0);

   if (load_db != NULL) {
      sqlite_close(load_db);
   }
   exit(1);
}


static void execute_sql (const char *table, const char *query) {

   char *error = NULL;

   if(verbose > 1) {
      printf("-- SQL: %s\n", query);
   }

   if (sqlite_exec(load_db, query, NULL, NULL, &error) != SQLITE_OK) {
      usage(table, 0, error);
   }
}


static void delete_from_table (const char *table) {

   char query[1024];

   if (verbose) {
      printf("-- Deleting existing data from table %s ..\n", table);
   }

   sprintf(query, "delete from %s", table);
   execute_sql(table, query);
}


static void remove_end_of_line(char *line) {

   char *from = strchr(line, '\n');
   if (from != NULL) *from = 0;
}


static void load_csv_file (const char *table, const char *csv) {

   int   line;
   char  data[0x10000];
   char  query[0x20000];
   char *values;
   char *from;
   char *end;
   char *next;
   FILE *file = fopen (csv, "r");


   if (verbose) {
      printf("-- Loading file %s to table %s ..\n", csv, table);
   }

   if (file == NULL) {
      usage(csv, 0, strerror(errno));
   }

   data[0] = 0;
   fgets(data, sizeof(data), file);
   remove_end_of_line(data);

   sprintf (query, "replace into %s (%s) values ('", table, data);
   values = query + strlen(query);

   line = 1;

   while (! feof(file)) {

      ++line;

      data[0] = 0;
      fgets(data, sizeof(data), file);
      if (data[0] < ' ') continue;

      remove_end_of_line(data);

      /* Translate the CSV format into SQL syntax (replace double
       * quotes with simple quote, or add the simple quotes).
       */
      from = data;
      *values = 0;

      for (from = data;; from = next + 1) {

         /* Skip the spaces before the value (help "formatting"
          * the csv file).
          */
         while (*from == ' ') ++from;

         if (*from == 0) break; /* End of the data. */

         if (*from == '"') {

            ++from;
            end = strchr(from, '"');
            if (end == NULL) {
               usage (csv, line, "missing end quote");
            }
            next = strchr(end, ',');

         } else {

            end = strchr(from, ',');
            next = end;
            if (end == NULL) end = from + strlen(from);

            /* Remove trailing spaces (help "formatting"
             * the csv file).
             */
            while (*(end-1) == ' ') --end;
         }

         *end = 0;

         /* Escape the single quotes. */
         for (end = strchr(from, SINGLE_QUOTE[0]);
              end != NULL; end = strchr(from, SINGLE_QUOTE[0])) {

            *end = 0;
            strcat (values, from);
            strcat (values, "''");
            from = end + 1;
         }

         strcat (values, from);

         if (next == NULL) break;

         strcat(values, "','");
      }

      strcat(values, "')");

      execute_sql(table, query);
   }

   fclose(file);
}


int main (int argc, char *argv[]) {

   int i;
   int start = 1;
   int delete_existing_data = 0;

   char *error = NULL;

   argv0 = argv[0];

   if (argc < 4) {
      usage (NULL, 0, "Missing argument.");
   }

   if (strcmp(argv[start], "--verbose") == 0) {
      verbose = 1;
      ++start;
   }
   else if (strncmp(argv[start], "--verbose=", 10) == 0) {
      verbose = atoi(argv[start] + 10);
      ++start;
   }

   if (strcmp(argv[start], "--clean") == 0) {
      delete_existing_data = 1;
      ++start;
   }

   load_db = sqlite_open(argv[start], 0, &error);
   if (load_db == NULL) {
      usage(argv[start], 0, error);
   }

   if (delete_existing_data) {
      delete_from_table(argv[start+1]);
   }

   for (i = start+2; i < argc; ++i) {
      load_csv_file(argv[start+1], argv[i]);
   }

   sqlite_close(load_db);

   return 0;
}

