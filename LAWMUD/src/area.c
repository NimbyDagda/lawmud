/*
 * area.c
 *
 * This file contains the code to do with areas and rooms
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* main header file */
#include "mud.h"

void load_areas()
{
  D_AREA *new_area;
  sqlite3_stmt *rSet = NULL;

  log_string("Load_all_areas: loading all areas from database.");

  sqlite3_prepare_v2(db, "select * from areas;", 21, &rSet, NULL);
  while (sqlite3_step(rSet) == SQLITE_ROW)
  {
    if ((new_area = malloc(sizeof(*new_area))) == NULL) {
      bug("Load_all_areas: cannot allocate memory for area.");
      abort();
    }
    new_area->aid = sqlite3_column_int(rSet,0);
    new_area->name = strdup((const char *)sqlite3_column_text(rSet,1));
    new_area->rooms = AllocList();

    load_rooms(new_area);
    AttachToList(new_area,darea_list);
  }
  sqlite3_reset(rSet);
  sqlite3_finalize(rSet);
  link_rooms();
}

void load_rooms(D_AREA *area)
{
  D_ROOM *new_room;
  char sql[45];
  sqlite3_stmt *rSet = NULL;

  memset(sql, '\0', sizeof(sql));

  snprintf(sql, sizeof(sql), "select * from rooms where areaid = %d;", area->aid);
  sqlite3_prepare_v2(db, sql, sizeof(sql), &rSet, NULL);
  while (sqlite3_step(rSet) == SQLITE_ROW)
  {
    if ((new_room = malloc(sizeof(*new_room))) == NULL) {
      bug("Load_rooms: cannot allocate memory for room.");
      abort();
    }
    new_room->area = area;
    new_room->rid = sqlite3_column_int(rSet,0);
    new_room->name = strdup((const char *)sqlite3_column_text(rSet,2));
    new_room->events = NULL; /* TODO: when we implement events for rooms */
    new_room->full_desc = strdup((const char *)sqlite3_column_text(rSet,4));
    new_room->short_desc = strdup((const char *)sqlite3_column_text(rSet,3));
    new_room->mobiles = AllocList();
    initialize_exits(new_room);

    AttachToList(new_room,area->rooms);
  }
  sqlite3_reset(rSet);
  sqlite3_finalize(rSet);
}

void link_rooms()
{
  D_ROOM *dRoom;
  D_AREA *dArea;
  ITERATOR aIter;
  ITERATOR rIter;
  char sql[40];
  sqlite3_stmt *rSet = NULL;
  int i, id = 0, rc;

  log_string("Link_rooms: linking all exits in all rooms.");

  AttachIterator(&aIter, darea_list);
  while ((dArea = (D_AREA *) NextInList(&aIter)) !=NULL)
  {
    AttachIterator(&rIter, dArea->rooms);
    while ((dRoom = (D_ROOM *) NextInList(&rIter)) !=NULL)
    {
      memset(sql, '\0', sizeof(sql));
      snprintf(sql, sizeof(sql), "select * from rooms where id = %d;", dRoom->rid);
      sqlite3_prepare_v2(db, sql, 40, &rSet, NULL);
      rc = sqlite3_step(rSet); /* TODO: Need to really check this in case room has ceased to be */
      for ( i = COL_NORTH ; i <= COL_DOWN; i++) {
        id = sqlite3_column_int(rSet,i);
        if (id != 0)
        {
          switch(i)
          {
          case COL_NORTH:
            dRoom->north = find_room(id);
            break;
          case COL_NORTHEAST:
            dRoom->northeast = find_room(id);
            break;
          case COL_EAST:
            dRoom->east = find_room(id);
            break;
          case COL_SOUTHEAST:
            dRoom->southeast = find_room(id);
            break;
          case COL_SOUTH:
            dRoom->south = find_room(id);
            break;
          case COL_SOUTWEST:
            dRoom->southwest = find_room(id);
            break;
          case COL_WEST:
            dRoom->west = find_room(id);
            break;
          case COL_NORTHWEST:
            dRoom->northwest = find_room(id);
            break;
          case COL_IN:
            dRoom->in = find_room(id);
            break;
          case COL_OUT:
            dRoom->out = find_room(id);
            break;
          case COL_UP:
            dRoom->up = find_room(id);
            break;
          case COL_DOWN:
            dRoom->down = find_room(id);
            break;
          }
        }
      }
      sqlite3_reset(rSet);
      sqlite3_finalize(rSet);
    }
    DetachIterator(&rIter);
  }
  DetachIterator(&aIter);
}

void initialize_exits(D_ROOM *room) {
  room->north = NULL;
  room->northeast = NULL;
  room->east = NULL;
  room->southeast = NULL;
  room->south = NULL;
  room->southwest = NULL;
  room->west = NULL;
  room->northwest = NULL;
  room->in = NULL;
  room->out = NULL;
  room->up = NULL;
  room->down = NULL;
}
