/*
 * This is the main headerfile
 */

#ifndef MUD_H
#define MUD_H

#include <zlib.h>
#include <pthread.h>
#include <arpa/telnet.h>
#include <sqlite3.h>

#include "list.h"
#include "stack.h"

/************************
 * Standard definitions *
 ************************/

/* define TRUE and FALSE */
#ifndef FALSE
#define FALSE   0
#endif
#ifndef TRUE
#define TRUE    1
#endif

#define eTHIN   0
#define eBOLD   1

/* A few globals */
#define PULSES_PER_SECOND     4                   /* must divide 1000 : 4, 5 or 8 works */
#define MAX_BUFFER         1024                   /* seems like a decent amount         */
#define MAX_OUTPUT         2048                   /* well shoot me if it isn't enough   */
#define MAX_HELP_ENTRY     4096                   /* roughly 40 lines of blocktext      */
#define MUDPORT            9009                   /* just set whatever port you want    */
#define FILE_TERMINATOR    "EOF"                  /* end of file marker                 */
#define COPYOVER_FILE      "../log/copyover.dat"  /* tempfile to store copyover data    */
#define EXE_FILE           "../src/SocketMud"     /* the name of the mud binary         */
#define SQLITE_DB          "../data/socketmud"    /* the name of the mud database       */
#define START_ROOM            1                   /* the room that new players enter    */

/* Connection states */
#define STATE_NEW_NAME         0
#define STATE_NEW_PASSWORD     1
#define STATE_VERIFY_PASSWORD  2
#define STATE_ASK_PASSWORD     3
#define STATE_PLAYING          4
#define STATE_CLOSED           5

/* Thread states - please do not change the order of these states    */
#define TSTATE_LOOKUP          0  /* Socket is in host_lookup        */
#define TSTATE_DONE            1  /* The lookup is done.             */
#define TSTATE_WAIT            2  /* Closed while in thread.         */
#define TSTATE_CLOSED          3  /* Closed, ready to be recycled.   */

/* player levels */
#define LEVEL_GUEST            1  /* Dead players and actual guests  */
#define LEVEL_PLAYER           2  /* Almost everyone is this level   */
#define LEVEL_ADMIN            3  /* Any admin without shell access  */
#define LEVEL_GOD              4  /* Any admin with shell access     */

/* Communication Ranges */
#define COMM_LOCAL             0  /* same room only                  */
#define COMM_AREA              2  /* same area only                  */
#define COMM_WORLD             8  /* everyone                        */
#define COMM_LOG              10  /* admins only                     */

/* directions */
#define DIR_N                  1  /* north                           */
#define DIR_NE                 2  /* northeast                       */
#define DIR_E                  3  /* east                            */
#define DIR_SE                 4  /* southeast                       */
#define DIR_S                  5  /* south                           */
#define DIR_SW                 6  /* southwest                       */
#define DIR_W                  7  /* west                            */
#define DIR_NW                 8  /* northwest                       */
#define DIR_IN                 9  /* in                              */
#define DIR_OUT               10  /* out                             */
#define DIR_UP                11  /* up                              */
#define DIR_DOWN              12  /* down                            */

/* define simple types */
typedef  unsigned char     bool;
typedef  short int         sh_int;
typedef  unsigned short int   bitmask;
typedef  unsigned short int   id;

/******************************
 * End of standard definitons *
 ******************************/

/***********************
 * Defintion of Macros *
 ***********************/

#define UMIN(a, b)		((a) < (b) ? (a) : (b))
#define IS_ADMIN(dMob)          ((dMob->level) > LEVEL_PLAYER ? TRUE : FALSE)

/***********************
 * End of Macros       *
 ***********************/

/******************************
 * New structures             *
 ******************************/

/* type defintions */
typedef struct  dSocket       D_SOCKET;
typedef struct  dMobile       D_MOBILE;
typedef struct  dArea         D_AREA;
typedef struct  dRoom         D_ROOM;
typedef struct  dExit         D_EXIT;
typedef struct  lookup_data   LOOKUP_DATA;
typedef struct  event_data    EVENT_DATA;

/* the actual structures */
struct dSocket
{
  D_MOBILE      * player;
  LIST          * events;
  char          * hostname;
  char            inbuf[MAX_BUFFER];
  char            outbuf[MAX_OUTPUT];
  char            next_command[MAX_BUFFER];
  bool            bust_prompt;
  sh_int          lookup_status;
  sh_int          state;
  sh_int          control;
  sh_int          top_output;
  unsigned char   compressing;                 /* MCCP support */
  z_stream      * out_compress;                /* MCCP support */
  unsigned char * out_compress_buf;            /* MCCP support */
};

struct dMobile
{
  D_SOCKET      * socket;
  LIST          * events;
  char          * name;
  char          * password;
  D_ROOM        * room;
  sh_int          level;
};

struct lookup_data
{
  D_SOCKET       * dsock;   /* the socket we wish to do a hostlookup on */
  char           * buf;     /* the buffer it should be stored in        */
};

struct typCmd
{
  char      * cmd_name;
  void     (* cmd_funct)(D_MOBILE *dMob, char *arg);
  sh_int      level;
};

typedef struct buffer_type
{
  char   * data;        /* The data                      */
  int      len;         /* The current len of the buffer */
  int      size;        /* The allocated size of data    */
} BUFFER;

/* here we include external structure headers */
#include "event.h"
#include "area.h"

/******************************
 * End of new structures      *
 ******************************/

/***************************
 * Global Variables        *
 ***************************/

extern  STACK       *   dsock_free;       /* the socket free list               */
extern  LIST        *   dsock_list;       /* the linked list of active sockets  */
extern  STACK       *   dmobile_free;     /* the mobile free list               */
extern  LIST        *   dmobile_list;     /* the mobile list of active mobiles  */
extern  LIST        *   darea_list;       /* the area list of loaded areas      */
extern  const struct    typCmd tabCmd[];  /* the command table                  */
extern  bool            shut_down;        /* used for shutdown                  */
extern  char        *   greeting;         /* the welcome greeting               */
extern  char        *   motd;             /* the MOTD help file                 */
extern  int             control;          /* boot control socket thingy         */
extern  time_t          current_time;     /* let's cut down on calls to time()  */
extern  sqlite3     *   db;               /* the connection to the sqlite3 db   */

/*************************** 
 * End of Global Variables *
 ***************************/

/***********************
 *    MCCP support     *
 ***********************/

extern const unsigned char compress_will[];
extern const unsigned char compress_will2[];

#define TELOPT_COMPRESS       85
#define TELOPT_COMPRESS2      86
#define COMPRESS_BUF_SIZE   8192

/***********************
 * End of MCCP support *
 ***********************/

/***********************************
 * Prototype function declerations *
 ***********************************/

/* more compact */
#define  D_S         D_SOCKET
#define  D_M         D_MOBILE
#define  D_R         D_ROOM

#define  buffer_new(size)             __buffer_new     ( size)
#define  buffer_strcat(buffer,text)   __buffer_strcat  ( buffer, text )

char  *crypt                  ( const char *key, const char *salt );

/*
 * socket.c
 */
int   init_socket             ( void );
bool  new_socket              ( int sock );
void  close_socket            ( D_S *dsock, bool reconnect );
bool  read_from_socket        ( D_S *dsock );
bool  text_to_socket          ( D_S *dsock, const char *txt );  /* sends the output directly */
void  text_to_buffer          ( D_S *dsock, const char *txt );  /* buffers the output        */
void  text_to_mobile          ( D_M *dMob, const char *txt );   /* buffers the output        */
void  next_cmd_from_buffer    ( D_S *dsock );
bool  flush_output            ( D_S *dsock );
void  handle_new_connections  ( D_S *dsock, char *arg );
void  clear_socket            ( D_S *sock_new, int sock );
void  recycle_sockets         ( void );
void *lookup_address          ( void *arg );

/*
 * interpret.c
 */
void  handle_cmd_input        ( D_S *dsock, char *arg );

/*
 * io.c
 */
void    log_string            ( const char *txt, ... );
void    bug                   ( const char *txt, ... );

/* 
 * strings.c
 */
char   *one_arg               ( char *fStr, char *bStr );
char   *strdup                ( const char *s );
int     strcasecmp            ( const char *s1, const char *s2 );
bool    is_prefix             ( const char *aStr, const char *bStr );
char   *capitalize            ( char *txt );
BUFFER *__buffer_new          ( int size );
void    __buffer_strcat       ( BUFFER *buffer, const char *text );
void    buffer_free           ( BUFFER *buffer );
void    buffer_clear          ( BUFFER *buffer );
int     bprintf               ( BUFFER *buffer, char *fmt, ... );

/*
 * help.c
 */
bool  check_help              ( D_M *dMob, char *helpfile );
char  *read_help_entry        ( const char *helpfile );     /* pointer         */

/*
 * utils.c
 */
bool  check_name              ( const char *name );
void  clear_mobile            ( D_M *dMob );
void  free_mobile             ( D_M *dMob );
void  communicate             ( D_M *dMob, char *txt, int range );
void  load_muddata            ( bool fCopyOver );
char *get_time                ( void );
void  copyover_recover        ( void );
D_M  *check_reconnect         ( char *player );
D_R  *find_room               ( int id );
void  walk_mobile             ( D_M *mob, D_R *room, int dir );
void  move_mobile             ( D_M *mob, D_R *room );
void  look_room               ( D_M *mob );
int   arrival_direction       ( D_R *newRoom, int oldRoom );
void  get_exits               ( D_R *dRoom, BUFFER *buf );

/*
 * action_safe.c
 */
void  cmd_say                 ( D_M *dMob, char *arg );
void  cmd_yell                ( D_M *dMob, char *arg );
void  cmd_shout               ( D_M *dMob, char *arg );
void  cmd_go                  ( D_M *dMob, char *arg );
void  cmd_look                ( D_M *dMob, char *arg );
void  cmd_quit                ( D_M *dMob, char *arg );
void  cmd_shutdown            ( D_M *dMob, char *arg );
void  cmd_commands            ( D_M *dMob, char *arg );
void  cmd_who                 ( D_M *dMob, char *arg );
void  cmd_help                ( D_M *dMob, char *arg );
void  cmd_compress            ( D_M *dMob, char *arg );
void  cmd_save                ( D_M *dMob, char *arg );
void  cmd_copyover            ( D_M *dMob, char *arg );
void  cmd_linkdead            ( D_M *dMob, char *arg );
void  cmd_north               ( D_M *dMob, char *arg );
void  cmd_northeast           ( D_M *dMob, char *arg );
void  cmd_east                ( D_M *dMob, char *arg );
void  cmd_southeast           ( D_M *dMob, char *arg );
void  cmd_south               ( D_M *dMob, char *arg );
void  cmd_southwest           ( D_M *dMob, char *arg );
void  cmd_west                ( D_M *dMob, char *arg );
void  cmd_northwest           ( D_M *dMob, char *arg );
void  cmd_in                  ( D_M *dMob, char *arg );
void  cmd_out                 ( D_M *dMob, char *arg );
void  cmd_up                  ( D_M *dMob, char *arg );
void  cmd_down                ( D_M *dMob, char *arg );

/*
 * mccp.c
 */
bool  compressStart           ( D_S *dsock, unsigned char teleopt );
bool  compressEnd             ( D_S *dsock, unsigned char teleopt, bool forced );

/*
 * save.c
 */
void  save_player             ( D_M *dMob );
D_M  *load_player             ( char *player );
D_M  *load_profile            ( char *player );

/*******************************
 * End of prototype declartion *
 *******************************/

#endif  /* MUD_H */
