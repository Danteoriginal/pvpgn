/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */
#ifndef INCLUDED_ANONGAME_TYPES
#define INCLUDED_ANONGAME_TYPES

#ifdef JUST_NEED_TYPES
# ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#   if HAVE_SYS_TIME_H
#    include <sys/time.h>
#   else
#    include <time.h>
#   endif
# endif
# include "game.h"
# include "common/queue.h"
# include "channel.h"
# include "account.h"
# include "quota.h"
# include "character.h"
# include "versioncheck.h"
# include "connection.h"
# ifdef WITH_BITS
#   include "bits.h"
#   include "bits_ext.h"
# endif
#else
# define JUST_NEED_TYPES
# ifdef TIME_WITH_SYS_TIME
#  include <sys/time.h>
#  include <time.h>
# else
#   if HAVE_SYS_TIME_H
#    include <sys/time.h>
#   else
#    include <time.h>
#   endif
# endif
# include "game.h"
# include "common/queue.h"
# include "channel.h"
# include "account.h"
# include "quota.h"
# include "character.h"
# include "versioncheck.h"
# include "connection.h"
# ifdef WITH_BITS
#   include "bits.h"
#   include "bits_ext.h"
# endif
# undef JUST_NEED_TYPES
#endif

#include "compat/uint.h"

typedef struct
{
    int				 currentplayers;
    int				 totalplayers;
    struct connection *	         player[8];
    t_account *			 account[8];
    int				 result[8];
} t_anongameinfo;

typedef struct
{
    t_anongameinfo *		 info;
    int				 count;
    t_uint32		  	 id;
    t_uint32		  	 race;
    t_uint32		  	 handle;
    unsigned int		 addr;	
    char			 loaded;
    char			 joined;
    t_uint8 		  	 playernum;
    t_uint8			 type;
} t_anongame;

typedef struct
{
	struct connection *c;
	t_uint32 map_prefs;
} t_matchdata;

typedef struct
{
	int atid;
	int count;
	t_uint32 map_prefs;
} t_atcountinfo;

#endif

/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_ANONGAME_PROTOS
#define INCLUDED_ANONGAME_PROTOS

#define JUST_NEED_TYPES
#include "common/packet.h"
#include "common/queue.h"
#include "channel.h"
#include "game.h"
#include "account.h"
#include "common/list.h"
#include "character.h"
#include "versioncheck.h"
#include "timer.h"
#include "connection.h"
#include "anongame.h"
#undef JUST_NEED_TYPES


extern t_anongameinfo * anongameinfo_create(int totalplayers);
extern void anongameinfo_destroy(t_anongameinfo * i);


extern void anongame_set_info(t_anongame * a, t_anongameinfo * i);
extern t_anongameinfo * anongame_get_info(t_anongame * a);

extern void anongame_set_player(t_anongame *a, int plnum, t_connection *c);
extern t_connection * anongame_get_player(t_anongame *a, int plnum);
extern t_account * anongame_get_account(t_anongame *a, int plnum);

extern void anongame_set_count(t_anongame *a, int count);
extern int anongame_get_count(t_anongame *a);

extern void anongame_set_addr(t_anongame *a, unsigned int addr);
extern unsigned int anongame_get_addr(t_anongame *a);

extern int anongame_get_totalplayers(t_anongame *a);

extern void anongame_set_currentplayers(t_anongame *a, int players);
extern int anongame_get_currentplayers(t_anongame *a);

extern void anongame_set_id(t_anongame *a, t_uint32 id);
extern t_uint32 anongame_get_id(t_anongame *a);

extern void anongame_set_race(t_anongame *a, t_uint32 race);
extern t_uint32 anongame_get_race(t_anongame *a);

extern void anongame_set_playernum(t_anongame *a, t_uint8 plnum);
extern t_uint8 anongame_get_playernum(t_anongame *a);

extern void anongame_set_type(t_anongame *a, t_uint8 type);
extern t_uint8 anongame_get_type(t_anongame *a);

extern void anongame_set_handle(t_anongame *a, t_uint32 handle);
extern t_uint32 anongame_get_handle(t_anongame *a);

extern void anongame_set_joined(t_anongame *a, char joined);
extern char anongame_get_joined(t_anongame *a);

extern void anongame_set_loaded(t_anongame *a, char loaded);
extern char anongame_get_loaded(t_anongame *a);

extern void anongame_set_result(t_anongame *a, int result);
extern int anongame_get_result(t_anongame *a, int plnum);

extern int anongame_maplists_create(void);
extern void anongame_maplists_destroy(void);

extern int anongame_totalplayers(t_uint8 gametype);
extern char anongame_arranged(t_uint8 gametype);
extern void handle_anongame_search(t_connection * c, t_packet const * packet);
extern int handle_anongame_join(t_connection * c);
extern int handle_w3route_packet(t_connection * c, t_packet const * const packet);
extern int anongame_unqueue_player(t_connection * c, t_uint8 gametype);
extern int anongame_unqueue_team(t_connection * c, t_uint8 gametype);

extern int anongame_matchlists_create();
extern int anongame_matchlists_destroy();

extern int anongame_stats(t_connection * c);

#endif
#endif
