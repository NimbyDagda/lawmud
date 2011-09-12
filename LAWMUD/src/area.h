/* area.h
 *
 * This file contains the structures and types to do with areas
 * and rooms
 *
 */

/* Exits for use in bitmasks */
#define EXIT_NORTH             1
#define EXIT_NORTHEAST         2
#define EXIT_EAST              4
#define EXIT_SOUTHEAST         8
#define EXIT_SOUTH            16
#define EXIT_SOUTHWEST        32
#define EXIT_WEST             64
#define EXIT_NORTHWEST       128
#define EXIT_IN              256
#define EXIT_OUT             512
#define EXIT_UP             1024
#define EXIT_DOWN           2048

/* Column IDs for exit links */
#define COL_NORTH          5
#define COL_NORTHEAST      6
#define COL_EAST           7
#define COL_SOUTHEAST      8
#define COL_SOUTH          9
#define COL_SOUTWEST       10
#define COL_WEST           11
#define COL_NORTHWEST      12
#define COL_IN             13
#define COL_OUT            14
#define COL_UP             15
#define COL_DOWN           16

/* Data structures for areas and rooms */
struct dArea
{
  char          * name;
  id              aid;
  LIST          * rooms;
};

struct dRoom
{
  D_AREA        * area;
  LIST          * mobiles;
  LIST          * events;
  char          * name;
  char          * short_desc;
  char          * full_desc;
  id              rid;
  D_ROOM        * north;
  D_ROOM        * northeast;
  D_ROOM        * east;
  D_ROOM        * southeast;
  D_ROOM        * south;
  D_ROOM        * southwest;
  D_ROOM        * west;
  D_ROOM        * northwest;
  D_ROOM        * in;
  D_ROOM        * out;
  D_ROOM        * up;
  D_ROOM        * down;
};

/***********************************
 * Prototype function declerations *
 ***********************************/

#define  D_A         D_AREA
#define  D_R         D_ROOM

/* area.c */
void  load_areas        ( void );
void  load_rooms        ( D_A *area );
void  link_rooms        ( void );
void  initialize_exits  ( D_R *room );
