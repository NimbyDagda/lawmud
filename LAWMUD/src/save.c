#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* main header file */
#include "mud.h"



void save_player(D_MOBILE *dMob)
{
  char pName[MAX_BUFFER];
  char sql[MAX_BUFFER];
  sqlite3_stmt *rSet = NULL;
  int size, i, rc;

  if (!dMob) return;

  memset(sql, '\0', MAX_BUFFER);

  pName[0] = toupper(dMob->name[0]);
  size = strlen(dMob->name);
  for (i = 1; i < size && i < MAX_BUFFER - 1; i++)
    pName[i] = tolower(dMob->name[i]);
  pName[i] = '\0';

  /* prepare sql to query player table */
  snprintf(sql, MAX_BUFFER, "select * from players where name = '%s';", pName);
  sqlite3_prepare_v2(db, sql, sizeof(sql), &rSet, NULL);

  rc = sqlite3_step(rSet);
  sqlite3_reset(rSet);
  sqlite3_finalize(rSet);

  /* existing player */
  if (rc == SQLITE_ROW) {
	  snprintf(sql, MAX_BUFFER, "update players set name = '%s', password = '%s', level = %d, room = %d where name = '%s';", dMob->name, dMob->password, dMob->level, dMob->room->rid, pName);
	  sqlite3_prepare_v2(db, sql, sizeof(sql), &rSet, NULL);
	  if (sqlite3_step(rSet) != SQLITE_DONE) {
		  log_string(sql);
		  log_string(sqlite3_errmsg(db));
		  return;
	  }
	  sqlite3_reset(rSet);
	  sqlite3_finalize(rSet);
  }
  /* new player */
  else if (rc == SQLITE_DONE) {
	  snprintf(sql, MAX_BUFFER, "insert into players (name, password, level, room, id) values ('%s', '%s', %d, %d, NULL);", dMob->name, dMob->password, dMob->level, dMob->room->rid);
	  sqlite3_prepare_v2(db, sql, sizeof(sql), &rSet, NULL);
	  if (sqlite3_step(rSet) != SQLITE_DONE) {
		  log_string(sql);
		  log_string(sqlite3_errmsg(db));
		  return;
	  }
	  sqlite3_reset(rSet);
	  sqlite3_finalize(rSet);
  } else log_string(sqlite3_errmsg(db));
}

D_MOBILE *load_player(char *player)
{
  D_MOBILE *dMob = NULL;
  char sql[MAX_BUFFER];
  char pName[MAX_BUFFER];
  sqlite3_stmt *rSet = NULL;
  int i, size;

  memset(sql, '\0', MAX_BUFFER);

  pName[0] = toupper(player[0]);
  size = strlen(player);
  for (i = 1; i < size && i < MAX_BUFFER - 1; i++)
    pName[i] = tolower(player[i]);
  pName[i] = '\0';

  /* prepare sql to query player table */
  snprintf(sql, MAX_BUFFER, "select * from players where name = '%s';", pName);
  sqlite3_prepare_v2(db, sql, sizeof(sql), &rSet, NULL);

  /* execute query and return NULL if player isn't found*/
  if (sqlite3_step(rSet) != SQLITE_ROW)
  {
    sqlite3_reset(rSet);
    sqlite3_finalize(rSet);
    return NULL;
  }

  /* create new mobile data */
  if (StackSize(dmobile_free) <= 0)
  {
    if ((dMob = malloc(sizeof(*dMob))) == NULL)
    {
      bug("Load_player: Cannot allocate memory.");
      abort();
    }
  }
  else
  {
    dMob = (D_MOBILE *) PopStack(dmobile_free);
  }
  clear_mobile(dMob);

  /* load data */
  dMob->name = strdup((const char *)sqlite3_column_text(rSet,3));
  dMob->password = strdup((const char *)sqlite3_column_text(rSet,4));
  dMob->level = sqlite3_column_int(rSet,2);
  dMob->room = find_room(sqlite3_column_int(rSet,0));

  /* clear result set */
  sqlite3_reset(rSet);
  sqlite3_finalize(rSet);

  if (dMob->room == NULL)
    dMob->room = find_room(START_ROOM);

  return dMob;
}

/*
 * This function loads a players profile, and stores
 * it in a mobile_data... DO NOT USE THIS DATA FOR
 * ANYTHING BUT CHECKING PASSWORDS OR SIMILAR.
 */
D_MOBILE *load_profile(char *player)
{
  D_MOBILE *dMob = NULL;
  char sql[MAX_BUFFER];
  char pName[MAX_BUFFER];
  sqlite3_stmt *rSet = NULL;
  int i, size;

  memset(sql, '\0', MAX_BUFFER);

  pName[0] = toupper(player[0]);
  size = strlen(player);
  for (i = 1; i < size && i < MAX_BUFFER - 1; i++)
    pName[i] = tolower(player[i]);
  pName[i] = '\0';

  /* prepare sql to query player table */
  snprintf(sql, MAX_BUFFER, "select name, password from players where name = '%s';", pName);
  sqlite3_prepare_v2(db, sql, sizeof(sql), &rSet, NULL);

  /* execute query and return NULL if player isn't found*/
  if (sqlite3_step(rSet) != SQLITE_ROW)
  {
    sqlite3_reset(rSet);
    sqlite3_finalize(rSet);
    return NULL;
  }

  /* create new mobile data */
  if (StackSize(dmobile_free) <= 0)
  {
    if ((dMob = malloc(sizeof(*dMob))) == NULL)
    {
      bug("Load_profile: Cannot allocate memory.");
      abort();
    }
  }
  else
  {
    dMob = (D_MOBILE *) PopStack(dmobile_free);
  }
  clear_mobile(dMob);

  /* load data */
  dMob->name = strdup((const char *)sqlite3_column_text(rSet,0));
  dMob->password = strdup((const char *)sqlite3_column_text(rSet,1));

  /* clear result set */
  sqlite3_reset(rSet);
  sqlite3_finalize(rSet);

  return dMob;
}
