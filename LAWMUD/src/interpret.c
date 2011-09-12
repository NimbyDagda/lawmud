/*
 * This file handles command interpreting
 */
#include <sys/types.h>
#include <stdio.h>

/* include main header file */
#include "mud.h"

void handle_cmd_input(D_SOCKET *dsock, char *arg)
{
  D_MOBILE *dMob;
  char command[MAX_BUFFER];
  bool found_cmd = FALSE;
  int i;

  if ((dMob = dsock->player) == NULL)
    return;

  arg = one_arg(arg, command);

  for (i = 0; tabCmd[i].cmd_name[0] != '\0' && !found_cmd; i++)
  {
    if (tabCmd[i].level > dMob->level) continue;

    if (is_prefix(command, tabCmd[i].cmd_name))
    {
      found_cmd = TRUE;
      (*tabCmd[i].cmd_funct)(dMob, arg);
    }
  }

  if (!found_cmd)
    text_to_mobile(dMob, "No such command.\n\r");
}

/*
 * The command table, very simple, but easy to extend.
 */
const struct typCmd tabCmd [] =
{

 /* command          function        Req. Level   */
 /* --------------------------------------------- */

  { "commands",      cmd_commands,   LEVEL_GUEST  },
  { "compress",      cmd_compress,   LEVEL_GUEST  },
  { "copyover",      cmd_copyover,   LEVEL_GOD    },
  { "d",             cmd_down,       LEVEL_GUEST  },
  { "down",          cmd_down,       LEVEL_GUEST  },
  { "e",             cmd_east,       LEVEL_GUEST  },
  { "east",          cmd_east,       LEVEL_GUEST  },
  { "go",            cmd_go,         LEVEL_GOD    },
  { "help",          cmd_help,       LEVEL_GUEST  },
  { "in",            cmd_in,         LEVEL_GUEST  },
  { "l",             cmd_look,       LEVEL_GUEST  },
  { "linkdead",      cmd_linkdead,   LEVEL_ADMIN  },
  { "look",          cmd_look,       LEVEL_GUEST  },
  { "n",             cmd_north,      LEVEL_GUEST  },
  { "ne",            cmd_northeast,  LEVEL_GUEST  },
  { "north",         cmd_north,      LEVEL_GUEST  },
  { "northeast",     cmd_northeast,  LEVEL_GUEST  },
  { "northwest",     cmd_northwest,  LEVEL_GUEST  },
  { "nw",            cmd_northwest,  LEVEL_GUEST  },
  { "out",           cmd_out,        LEVEL_GUEST  },
  { "quit",          cmd_quit,       LEVEL_GUEST  },
  { "s",             cmd_south,      LEVEL_GUEST  },
  { "say",           cmd_say,        LEVEL_GUEST  },
  { "save",          cmd_save,       LEVEL_GUEST  },
  { "se",            cmd_southeast,  LEVEL_GUEST  },
  { "shout",         cmd_shout,      LEVEL_GUEST  },
  { "shutdown",      cmd_shutdown,   LEVEL_GOD    },
  { "south",         cmd_south,      LEVEL_GUEST  },
  { "southeast",     cmd_southeast,  LEVEL_GUEST  },
  { "southwest",     cmd_southwest,  LEVEL_GUEST  },
  { "sw",            cmd_southwest,  LEVEL_GUEST  },
  { "u",             cmd_up,         LEVEL_GUEST  },
  { "up",            cmd_up,         LEVEL_GUEST  },
  { "w",             cmd_west,       LEVEL_GUEST  },
  { "west",          cmd_west,       LEVEL_GUEST  },
  { "who",           cmd_who,        LEVEL_GUEST  },
  { "yell",          cmd_yell,       LEVEL_GUEST  },

  /* end of table */
  { "", 0 }
};
