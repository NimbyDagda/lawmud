/*
 * This file contains all sorts of utility functions used
 * all sorts of places in the code.
 */
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>

/* include main header file */
#include "mud.h"

char * greeting; /* the welcome greeting              */
char * motd; /* the MOTD help file                */

/*
 * Check to see if a given name is
 * legal, returning FALSE if it
 * fails our high standards...
 */
bool check_name(const char *name) {
  int size, i;

  if ((size = strlen(name)) < 3 || size > 12)
    return FALSE;

  for (i = 0; i < size; i++)
    if (!isalpha(name[i]))
      return FALSE;

  return TRUE;
}

void clear_mobile(D_MOBILE *dMob) {
  memset(dMob, 0, sizeof(*dMob));

  dMob->name = NULL;
  dMob->password = NULL;
  dMob->level = LEVEL_PLAYER;
  dMob->room = NULL;
  dMob->events = AllocList();
}

void free_mobile(D_MOBILE *dMob) {
  EVENT_DATA *pEvent;
  ITERATOR Iter;

  DetachFromList(dMob, dmobile_list);
  if (dMob->room != NULL) {
    DetachFromList(dMob, dMob->room->mobiles);
    dMob->room = NULL;
  }

  if (dMob->socket)
    dMob->socket->player = NULL;

  AttachIterator(&Iter, dMob->events);
  while ((pEvent = (EVENT_DATA *) NextInList(&Iter)) != NULL)
    dequeue_event(pEvent);
  DetachIterator(&Iter);
  FreeList(dMob->events);

  /* free allocated memory */
  free(dMob->name);
  free(dMob->password);

  PushStack(dMob, dmobile_free);
}

void communicate(D_MOBILE *dMob, char *txt, int range) {
  D_MOBILE *xMob;
  D_ROOM *room;
  ITERATOR Iter;
  ITERATOR Iter2;
  char buf[MAX_BUFFER];
  char message[MAX_BUFFER];

  switch (range) {
  default:
    bug("Communicate: Bad Range %d.", range);
    return;
  case COMM_LOCAL: /* say */
    snprintf(message, MAX_BUFFER, "%s says '%s'.\n\r", dMob->name, txt);
    snprintf(buf, MAX_BUFFER, "You say '%s'.\n\r", txt);
    text_to_mobile(dMob, buf);
    AttachIterator(&Iter, dMob->room->mobiles);
    while ((xMob = (D_MOBILE *) NextInList(&Iter)) != NULL) {
      if (xMob == dMob)
        continue;
      text_to_mobile(xMob, message);
    }
    DetachIterator(&Iter);
    break;
  case COMM_AREA: /* yell */
    snprintf(message, MAX_BUFFER, "%s yells '%s'.\n\r", dMob->name, txt);
    snprintf(buf, MAX_BUFFER, "You yell '%s'.\n\r", txt);
    text_to_mobile(dMob, buf);
    AttachIterator(&Iter, dMob->room->area->rooms);
    while ((room = (D_ROOM *) NextInList(&Iter)) != NULL) {
      AttachIterator(&Iter2, room->mobiles);
      while ((xMob = (D_MOBILE *) NextInList(&Iter2)) != NULL) {
        if (xMob == dMob)
          continue;
        text_to_mobile(xMob, message);
      }
      DetachIterator(&Iter2);
    }
    DetachIterator(&Iter);
    break;
  case COMM_WORLD: /* shout */
    snprintf(message, MAX_BUFFER, "%s shouts '%s'.\n\r", dMob->name, txt);
    snprintf(buf, MAX_BUFFER, "You shout '%s'.\n\r", txt);
    text_to_mobile(dMob, buf);
    AttachIterator(&Iter, dmobile_list);
    while ((xMob = (D_MOBILE *) NextInList(&Iter)) != NULL) {
      if (xMob == dMob)
        continue;
      text_to_mobile(xMob, message);
    }
    DetachIterator(&Iter);
    break;
  case COMM_LOG:
    snprintf(message, MAX_BUFFER, "[LOG: %s]\n\r", txt);
    AttachIterator(&Iter, dmobile_list);
    while ((xMob = (D_MOBILE *) NextInList(&Iter)) != NULL) {
      if (!IS_ADMIN(xMob))
        continue;
      text_to_mobile(xMob, message);
    }
    DetachIterator(&Iter);
    break;
  }
}

/*
 * Loading of help files, areas, etc, at boot time.
 */
void load_muddata(bool fCopyOver) {
  char sql[MAX_BUFFER];
  sqlite3_stmt *rSet = NULL;

  memset(sql, '\0', MAX_BUFFER);

  /* load greeting */
  snprintf(sql, MAX_BUFFER, "select value from muddata where key = 'greeting';");
  sqlite3_prepare_v2(db, sql, sizeof(sql), &rSet, NULL);
  if (sqlite3_step(rSet) != SQLITE_ROW) {
    log_string("Couldn't load greeting!");
  }
  greeting = strdup((const char *) sqlite3_column_text(rSet, 0));
  sqlite3_reset(rSet);
  sqlite3_finalize(rSet);

  /* load motd */
  snprintf(sql, MAX_BUFFER, "select value from muddata where key = 'motd';");
  sqlite3_prepare_v2(db, sql, sizeof(sql), &rSet, NULL);
  if (sqlite3_step(rSet) != SQLITE_ROW) {
    log_string("Couldn't load motd!");
  }
  motd = strdup((const char *) sqlite3_column_text(rSet, 0));
  sqlite3_reset(rSet);
  sqlite3_finalize(rSet);

  /* load areas */
  load_areas();

  /* copyover */
  if (fCopyOver)
    copyover_recover();
}

char *get_time() {
  static char buf[16];
  char *strtime;
  int i;

  strtime = ctime(&current_time);
  for (i = 0; i < 15; i++)
    buf[i] = strtime[i + 4];
  buf[15] = '\0';

  return buf;
}

/* Recover from a copyover - load players */
void copyover_recover() {
  D_MOBILE *dMob;
  D_SOCKET *dsock;
  FILE *fp;
  char name[100];
  char host[MAX_BUFFER];
  int desc;

  log_string("Copyover recovery initiated");

  if ((fp = fopen(COPYOVER_FILE, "r")) == NULL) {
    log_string("Copyover file not found. Exitting.");
    exit(1);
  }

  /* In case something crashes - doesn't prevent reading */
  unlink(COPYOVER_FILE);

  for (;;) {
    fscanf(fp, "%d %s %s\n", &desc, name, host);
    if (desc == -1)
      break;

    dsock = malloc(sizeof(*dsock));
    clear_socket(dsock, desc);

    dsock->hostname = strdup(host);
    AttachToList(dsock, dsock_list);

    /* load player data */
    if ((dMob = load_player(name)) != NULL) {
      /* attach to socket */
      dMob->socket = dsock;
      dsock->player = dMob;

      /* attach to mobile list */
      AttachToList(dMob, dmobile_list);

      /* initialize events on the player */
      init_events_player(dMob);
    } else /* ah bugger */
    {
      close_socket(dsock, FALSE);
      continue;
    }

    /* Write something, and check if it goes error-free */
    if (!text_to_socket(dsock,
        "\n\r <*>  And before you know it, everything has changed  <*>\n\r")) {
      close_socket(dsock, FALSE);
      continue;
    }

    /* make sure the socket can be used */
    dsock->bust_prompt = TRUE;
    dsock->lookup_status = TSTATE_DONE;
    dsock->state = STATE_PLAYING;

    /* negotiate compression */
    text_to_buffer(dsock, (char *) compress_will2);
    text_to_buffer(dsock, (char *) compress_will);
  }
  fclose(fp);
}

D_MOBILE *check_reconnect(char *player) {
  D_MOBILE *dMob;
  ITERATOR Iter;

  AttachIterator(&Iter, dmobile_list);
  while ((dMob = (D_MOBILE *) NextInList(&Iter)) != NULL) {
    if (!strcasecmp(dMob->name, player)) {
      if (dMob->socket)
        close_socket(dMob->socket, TRUE);

      break;
    }
  }
  DetachIterator(&Iter);

  return dMob;
}

D_ROOM *find_room(int id) {
  D_ROOM *dRoom;
  D_AREA *dArea;
  ITERATOR aIter;
  ITERATOR rIter;
  bool found = FALSE;

  AttachIterator(&aIter, darea_list);
  while ((dArea = (D_AREA *) NextInList(&aIter)) != NULL) {
    AttachIterator(&rIter, dArea->rooms);
    while ((dRoom = (D_ROOM *) NextInList(&rIter)) != NULL) {
      if (dRoom->rid == id) {
        found = TRUE;
        break;
      }
    }
    DetachIterator(&rIter);
    if (found)
      break;
  }
  DetachIterator(&aIter);

  if (!found)
    return NULL;

  return dRoom;
}

void walk_mobile(D_MOBILE *mob, D_ROOM *room, int dir) {
  ITERATOR Iter;
  D_MOBILE *xMob;
  int oldRoom = 0;
  int arrDir = 0;
  char message[MAX_BUFFER];
  char direction[28];

  oldRoom = mob->room->rid;

  memset(direction, '\0', 28);
  /* Tell everyone that the mobile is leaving the room. */
  switch (dir) {
  case DIR_N:
    snprintf(direction, 28, "leaves to the north.\n\r");
    break;
  case DIR_NE:
    snprintf(direction, 28, "leaves to the northeast.\n\r");
    break;
  case DIR_E:
    snprintf(direction, 28, "leaves to the east.\n\r");
    break;
  case DIR_SE:
    snprintf(direction, 28, "leaves to the southeast.\n\r");
    break;
  case DIR_S:
    snprintf(direction, 28, "leaves to the south.\n\r");
    break;
  case DIR_SW:
    snprintf(direction, 28, "leaves to the southwest.\n\r");
    break;
  case DIR_W:
    snprintf(direction, 28, "leaves to the west.\n\r");
    break;
  case DIR_NW:
    snprintf(direction, 28, "leaves to the northwest.\n\r");
    break;
  case DIR_IN:
    snprintf(direction, 28, "goes inside.\n\r");
    break;
  case DIR_OUT:
    snprintf(direction, 28, "goes outside.\n\r");
    break;
  case DIR_UP:
    snprintf(direction, 28, "heads upwards.\n\r");
    break;
  case DIR_DOWN:
    snprintf(direction, 28, "heads downwards.\n\r");
    break;
  }

  snprintf(message, MAX_BUFFER, "%s %s", mob->name, direction);

  AttachIterator(&Iter, mob->room->mobiles);
  while ((xMob = (D_MOBILE *) NextInList(&Iter)) != NULL) {
    if (xMob == mob)
      continue;
    text_to_mobile(xMob, message);
  }
  DetachIterator(&Iter);

  /* actually move the mobile */
  move_mobile(mob, room);

  /* reset message buffers */
  memset(direction, '\0', 28);
  memset(message, '\0', MAX_BUFFER);

  /* find out what direction the mobile arrived from */
  arrDir = arrival_direction(room, oldRoom);

  /* Tell everyone in new room that the mobile has arrived. */
  if (arrDir == 0) {
    snprintf(message, MAX_BUFFER, "You notice %s appear as if from nowhere.\n\r", mob->name);
  } else {
    switch (arrDir) {
    case DIR_N:
      snprintf(direction, 28, "the north.\n\r");
      break;
    case DIR_NE:
      snprintf(direction, 28, "the northeast.\n\r");
      break;
    case DIR_E:
      snprintf(direction, 28, "the east.\n\r");
      break;
    case DIR_SE:
      snprintf(direction, 28, "the southeast.\n\r");
      break;
    case DIR_S:
      snprintf(direction, 28, "the south.\n\r");
      break;
    case DIR_SW:
      snprintf(direction, 28, "the southwest.\n\r");
      break;
    case DIR_W:
      snprintf(direction, 28, "the west.\n\r");
      break;
    case DIR_NW:
      snprintf(direction, 28, "the northwest.\n\r");
      break;
    case DIR_IN:
      snprintf(direction, 28, "inside.\n\r");
      break;
    case DIR_OUT:
      snprintf(direction, 28, "outside.\n\r");
      break;
    case DIR_UP:
      snprintf(direction, 28, "above.\n\r");
      break;
    case DIR_DOWN:
      snprintf(direction, 28, "below.\n\r");
      break;
    }
    snprintf(message, MAX_BUFFER, "%s arrives from %s", mob->name, direction);
  }
  AttachIterator(&Iter, room->mobiles);
  while ((xMob = (D_MOBILE *) NextInList(&Iter)) != NULL) {
    if (xMob == mob)
      continue;
    text_to_mobile(xMob, message);
  }
  DetachIterator(&Iter);
}

void move_mobile(D_MOBILE *mob, D_ROOM *room) {
  /* remove mobile from current room */
  DetachFromList(mob, mob->room->mobiles);
  /* point mobile at new room */
  mob->room = room;
  /* move mobile into new room */
  AttachToList(mob, room->mobiles);
  /* have a look around */
  look_room(mob);
}

void look_room(D_MOBILE *mob) {
  D_ROOM *dRoom;
  BUFFER *buf = buffer_new(MAX_BUFFER);
  ITERATOR Iter;
  D_MOBILE *xMob;
  int i = 0, size;

  dRoom = mob->room;
  /* show room name and description */
  bprintf(buf, "#G%s#n\n\r", dRoom->name);
  bprintf(buf, "#b%s#n\n\r", dRoom->full_desc);

  /* show other people present in the room */
  size = dRoom->mobiles->_size;
  if (size != 1) {
    AttachIterator(&Iter, dRoom->mobiles);
    while ((xMob = (D_MOBILE *) NextInList(&Iter)) != NULL) {
      if (xMob == mob)
        continue;
      i++;
      if ((size == 2) && (i == 1))
        bprintf(buf, "#y%s is standing here.#n\n\r", xMob->name);
      else if ((i < size) && (i == 1))
        bprintf(buf, "#y%s#n", xMob->name);
      else if ((i < size) && (size == i + 1))
        bprintf(buf, "#y and %s are standing here.#n\n\r", xMob->name);
      else
        bprintf(buf, "#y, %s#n", xMob->name);
    }
    DetachIterator(&Iter);
  }

  /* show exits */
  get_exits(dRoom, buf);

  /* send it all to the mob */
  text_to_mobile(mob, buf->data);
  buffer_free(buf);
}

int arrival_direction(D_ROOM *newRoom, int oldRoom) {
  int direction = 0;

  if ((newRoom->north != NULL) && (newRoom->north->rid == oldRoom))
    direction = DIR_N;
  else if ((newRoom->northeast != NULL) && (newRoom->northeast->rid == oldRoom))
    direction = DIR_NE;
  else if ((newRoom->east != NULL) && (newRoom->east->rid == oldRoom))
    direction = DIR_E;
  else if ((newRoom->southeast != NULL) && (newRoom->southeast->rid == oldRoom))
    direction = DIR_SE;
  else if ((newRoom->south != NULL) && (newRoom->south->rid == oldRoom))
    direction = DIR_S;
  else if ((newRoom->southwest != NULL) && (newRoom->southwest->rid == oldRoom))
    direction = DIR_SW;
  else if ((newRoom->west != NULL) && (newRoom->west->rid == oldRoom))
    direction = DIR_W;
  else if ((newRoom->northwest != NULL) && (newRoom->northwest->rid == oldRoom))
    direction = DIR_NW;
  else if ((newRoom->in != NULL) && (newRoom->in->rid == oldRoom))
    direction = DIR_IN;
  else if ((newRoom->out != NULL) && (newRoom->out->rid == oldRoom))
    direction = DIR_OUT;
  else if ((newRoom->up != NULL) && (newRoom->up->rid == oldRoom))
    direction = DIR_UP;
  else if ((newRoom->down != NULL) && (newRoom->down->rid == oldRoom))
    direction = DIR_DOWN;

  return direction;
}

void get_exits(D_ROOM *dRoom, BUFFER *buf)
{
  bitmask exits = 0;

  if (dRoom->north != NULL)
    exits += EXIT_NORTH;
  if (dRoom->northeast != NULL)
    exits += EXIT_NORTHEAST;
  if (dRoom->east != NULL)
    exits += EXIT_EAST;
  if (dRoom->southeast != NULL)
    exits += EXIT_SOUTHEAST;
  if (dRoom->south != NULL)
    exits += EXIT_SOUTH;
  if (dRoom->southwest != NULL)
    exits += EXIT_SOUTHWEST;
  if (dRoom->west != NULL)
    exits += EXIT_WEST;
  if (dRoom->northwest != NULL)
    exits += EXIT_NORTHWEST;
  if (dRoom->in != NULL)
    exits += EXIT_IN;
  if (dRoom->out != NULL)
    exits += EXIT_OUT;
  if (dRoom->up != NULL)
    exits += EXIT_UP;
  if (dRoom->down != NULL)
    exits += EXIT_DOWN;

  if (exits == 0)
    bprintf(buf, "#PThere are no exits from this room.#n\n\r");
  else {
    bprintf(buf, "#PExits :");
    if (exits & EXIT_NORTH)
      bprintf(buf, " North");
    if (exits & EXIT_NORTHEAST)
      bprintf(buf, " Northeast");
    if (exits & EXIT_EAST)
      bprintf(buf, " East");
    if (exits & EXIT_SOUTHEAST)
      bprintf(buf, " Southeast");
    if (exits & EXIT_SOUTH)
      bprintf(buf, " South");
    if (exits & EXIT_SOUTHWEST)
      bprintf(buf, " Southwest");
    if (exits & EXIT_WEST)
      bprintf(buf, " West");
    if (exits & EXIT_NORTHWEST)
      bprintf(buf, " Northwest");
    if (exits & EXIT_IN)
      bprintf(buf, " In");
    if (exits & EXIT_OUT)
      bprintf(buf, " Out");
    if (exits & EXIT_UP)
      bprintf(buf, " Up");
    if (exits & EXIT_DOWN)
      bprintf(buf, " Down");
    bprintf(buf, "#n\n\r");
  }
}
