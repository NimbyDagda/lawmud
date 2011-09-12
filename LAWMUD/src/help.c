/*
 * This file contains the dynamic help system.
 * If you wish to update a help file, simply edit
 * the entry in ../help/ and the mud will load the
 * new version next time someone tries to access
 * that help file.
 */
#include <sys/types.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include <dirent.h> 

/* include main header file */
#include "mud.h"

/*
 * Check_help()
 *
 */
bool check_help(D_MOBILE *dMob, char *helpfile)
{
  char buf[MAX_HELP_ENTRY + 80];
  char *entry, *hFile;

  hFile = capitalize(helpfile);
  if ((entry = read_help_entry(hFile)) == NULL)
    return FALSE;

  snprintf(buf, MAX_HELP_ENTRY + 80, "=== %s ===\n\r%s", hFile, entry);
  text_to_mobile(dMob, buf);

  return TRUE;
}

char *read_help_entry(const char *helpfile)
{
  static char entry[MAX_HELP_ENTRY];
  char sql[MAX_BUFFER];
  sqlite3_stmt *rSet = NULL;

  memset(sql, '\0', MAX_BUFFER);

  /* prepare sql to query helplkup table */
  snprintf(sql, MAX_BUFFER, "select help.contents from help inner join helplkup on helplkup.helpid = help.id where helplkup.keyword = '%s';", helpfile);
  sqlite3_prepare_v2(db, sql, sizeof(sql), &rSet, NULL);

  /* execute query and return NULL if help isn't found*/
  if (sqlite3_step(rSet) != SQLITE_ROW)
	  return NULL;

  strcpy(entry, (const char *)sqlite3_column_text(rSet,0));
  sqlite3_reset(rSet);
  sqlite3_finalize(rSet);

  /* return a pointer to the static buffer */
  return entry;
}
