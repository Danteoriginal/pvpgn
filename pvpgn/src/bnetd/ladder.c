/*
 * Copyright (C) 1999  Rob Crittenden (rcrit@greyoak.com)
 * Copyright (C) 1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 1999,2000  D.Moreaux (vapula@linuxbe.org)
 *
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
#define LADDER_INTERNAL_ACCESS
#include "common/setup_before.h"
#ifdef HAVE_STDDEF_H
# include <stddef.h>
#else
# ifndef NULL
#  define NULL ((void *)0)
# endif
#endif
#ifdef STDC_HEADERS
# include <stdlib.h>
#else
# ifdef HAVE_MALLOC_H
#  include <malloc.h>
# endif
#endif
#ifdef HAVE_STRING_H
# include <string.h>
#else
# ifdef HAVE_STRINGS_H
#  include <strings.h>
# endif
#endif
#include <errno.h>
#include <compat/strerror.h>
#include "common/field_sizes.h"
#include "account.h"
#include "common/eventlog.h"
#include "common/util.h"
#include "game.h"
#include "common/tag.h"
#include "common/list.h"
#include "common/bnettime.h"
#include "prefs.h"
#include "common/hashtable.h"
#include "ladder_calc.h"
#include "ladder.h"
#include "compat/strcasecmp.h"
#include "compat/strncasecmp.h"
#include "ladder_binary.h"
#include "account.h"
#include "common/bnet_protocol.h"
#include "common/setup_after.h"

#define MaxRankKeptInLadder 1000

/* for War3 XP computations */
static t_xpcalc *xpcalc;
static t_xplevel_entry * xplevels;

const char * WAR3_solo_file = "WAR3_solo";
const char * W3XP_solo_file = "W3XP_solo";
const char * WAR3_team_file = "WAR3_team";
const char * W3XP_team_file = "W3XP_team";
const char * WAR3_ffa_file  = "WAR3_ffa";
const char * W3XP_ffa_file  = "W3XP_ffa";
const char * WAR3_at_file   = "WAR3_atteam";
const char * W3XP_at_file   = "W3XP_atteam";
const char * std_end   = ".dat";
const char * xml_end   = ".xml";

char * WAR3_solo_filename, * WAR3_team_filename, * WAR3_ffa_filename, * WAR3_at_filename;
char * W3XP_solo_filename, * W3XP_team_filename, * W3XP_ffa_filename, * W3XP_at_filename;

t_ladder WAR3_solo_ladder, WAR3_team_ladder, WAR3_ffa_ladder, WAR3_at_ladder;
t_ladder W3XP_solo_ladder, W3XP_team_ladder, W3XP_ffa_ladder, W3XP_at_ladder;
t_ladder STAR_active_rating,          STAR_active_wins,          STAR_active_games, 
         STAR_current_rating,         STAR_current_wins,         STAR_current_games;
t_ladder SEXP_active_rating,          SEXP_active_wins,          SEXP_active_games, 
         SEXP_current_rating,         SEXP_current_wins,         SEXP_current_games;
t_ladder W2BN_active_rating,          W2BN_active_wins,          W2BN_active_games,
         W2BN_active_rating_ironman,  W2BN_active_wins_ironman,  W2BN_active_games_ironman;
t_ladder W2BN_current_rating,         W2BN_current_wins,         W2BN_current_games,
         W2BN_current_rating_ironman, W2BN_current_wins_ironman, W2BN_current_games_ironman;

t_ladder_internal * last_internal = NULL;
t_ladder	     * last_ladder   = NULL;
int 		       last_rank     = 0;

/*
 * Make the current ladder statistics the active ones.
 */

extern int ladderlist_make_all_active(void)
{
	ladder_make_active(ladder_cr(CLIENTTAG_STARCRAFT,ladder_id_normal), ladder_ar(CLIENTTAG_STARCRAFT,ladder_id_normal));
	ladder_make_active(ladder_cw(CLIENTTAG_STARCRAFT,ladder_id_normal), ladder_aw(CLIENTTAG_STARCRAFT,ladder_id_normal));
	ladder_make_active(ladder_cg(CLIENTTAG_STARCRAFT,ladder_id_normal), ladder_ag(CLIENTTAG_STARCRAFT,ladder_id_normal));
	ladder_make_active(ladder_cr(CLIENTTAG_BROODWARS,ladder_id_normal), ladder_ar(CLIENTTAG_BROODWARS,ladder_id_normal));
	ladder_make_active(ladder_cw(CLIENTTAG_BROODWARS,ladder_id_normal), ladder_aw(CLIENTTAG_BROODWARS,ladder_id_normal));
	ladder_make_active(ladder_cg(CLIENTTAG_BROODWARS,ladder_id_normal), ladder_ag(CLIENTTAG_BROODWARS,ladder_id_normal));
	ladder_make_active(ladder_cr(CLIENTTAG_WARCIIBNE,ladder_id_normal), ladder_ar(CLIENTTAG_WARCIIBNE,ladder_id_normal));
	ladder_make_active(ladder_cw(CLIENTTAG_WARCIIBNE,ladder_id_normal), ladder_aw(CLIENTTAG_WARCIIBNE,ladder_id_normal));
	ladder_make_active(ladder_cg(CLIENTTAG_WARCIIBNE,ladder_id_normal), ladder_ag(CLIENTTAG_WARCIIBNE,ladder_id_normal));
	ladder_make_active(ladder_cr(CLIENTTAG_WARCIIBNE,ladder_id_ironman),ladder_ar(CLIENTTAG_WARCIIBNE,ladder_id_ironman));
	ladder_make_active(ladder_cw(CLIENTTAG_WARCIIBNE,ladder_id_ironman),ladder_aw(CLIENTTAG_WARCIIBNE,ladder_id_ironman));
	ladder_make_active(ladder_cg(CLIENTTAG_WARCIIBNE,ladder_id_ironman),ladder_ag(CLIENTTAG_WARCIIBNE,ladder_id_ironman));
	ladder_update_all_accounts();
    return 0;
}


/*
 * Prepare an account for first ladder play if necessary.
 */
extern int ladder_init_account(t_account * account, char const * clienttag, t_ladder_id id)
{
    char const * tname;
	int uid;
    
    if (!account)
    {
	eventlog(eventlog_level_error,"ladder_init_account","got NULL account");
	return -1;
    }
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"ladder_init_account","got bad clienttag");
	return -1;
    }
    
    if (account_get_ladder_rating(account,clienttag,id)==0)
    {
	if (account_get_ladder_wins(account,clienttag,id)+
	    account_get_ladder_losses(account,clienttag,id)>0) /* no ladder games so far... */
	{
	    eventlog(eventlog_level_warn,"ladder_init_account","account for \"%s\" (%s) has %u wins and %u losses but has zero rating",(tname = account_get_name(account)),clienttag,account_get_ladder_wins(account,clienttag,id),account_get_ladder_losses(account,clienttag,id));
	    account_unget_name(tname);
	    return -1;
	}
	account_adjust_ladder_rating(account,clienttag,id,prefs_get_ladder_init_rating());
	
	uid = account_get_uid(account);

	war3_ladder_add(ladder_cr(clienttag,id),uid,0,account_get_ladder_rating(account,clienttag,id),account,0,clienttag);
	war3_ladder_add(ladder_cw(clienttag,id),uid,0,0,account,0,clienttag);
	war3_ladder_add(ladder_cg(clienttag,id),uid,0,0,account,0,clienttag);

	eventlog(eventlog_level_info,"ladder_init_account","initialized account for \"%s\" for \"%s\" ladder",(tname = account_get_name(account)),clienttag);
	account_unget_name(tname);
    }
    
    return 0;
}


/*
 * Update player ratings, rankings, etc due to game results.
 */
extern int ladder_update(char const * clienttag, t_ladder_id id, unsigned int count, t_account * * players, t_game_result * results, t_ladder_info * info, t_ladder_option opns)
{
    unsigned int curr;
    t_account *  sorted[8];
    unsigned int winners=0;
    unsigned int losers=0;
    unsigned int draws=0;
	int uid;
    
    if (count<2 || count>8)
    {
	eventlog(eventlog_level_error,"ladder_update","got invalid player count %u",count);
	return -1;
    }
    if (!players)
    {
	eventlog(eventlog_level_error,"ladder_update","got NULL players");
	return -1;
    }
    if (!results)
    {
	eventlog(eventlog_level_error,"ladder_update","got NULL results");
	return -1;
    }
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"ladder_update","got bad clienttag");
	return -1;
    }
    if (!info)
    {
	eventlog(eventlog_level_error,"ladder_update","got NULL info");
	return -1;
    }
    
    for (curr=0; curr<count; curr++)
    {
	if (!players[curr])
	{
	    eventlog(eventlog_level_error,"ladder_update","got NULL player[%u] (of %u)",curr,count);
	    return -1;
	}
	
	switch (results[curr])
	{
	case game_result_win:
	    winners++;
	    break;
	case game_result_loss:
	    losers++;
	    break;
	case game_result_draw:
	    draws++;
	    break;
	case game_result_disconnect:
	    if (opns&ladder_option_disconnectisloss)
		losers++;
	    break;
	default:
	    eventlog(eventlog_level_error,"ladder_update","bad results[%u]=%u",curr,(unsigned int)results[curr]);
	    return -1;
	}
    }
    
    if (draws>0)
    {
	if (draws!=count)
	{
	    eventlog(eventlog_level_error,"ladder_update","some, but not all players had a draw count=%u (winners=%u losers=%u draws=%u)",count,winners,losers,draws);
	    return -1;
	}
	
	return 0; /* no change in case of draw */
    }
    if (winners!=1 || losers<1)
    {
	eventlog(eventlog_level_error,"ladder_update","missing winner or loser for count=%u (winners=%u losers=%u draws=%u)",count,winners,losers,draws);
	return -1;
    }
    
    if (ladder_calc_info(clienttag,id,count,players,sorted,results,info)<0)
    {
	eventlog(eventlog_level_error,"ladder_update","unable to calculate info from game results");
	return -1;
    }
    
    for (curr=0; curr<count; curr++)
	{
	  int won,wins,games;
	  t_account * account;

	  if (results[curr]==game_result_win) won=1; else won=0;
	  
	  account = players[curr];
	  uid = account_get_uid(account);
	  wins = account_get_ladder_wins(account,clienttag,id);
	  games = wins + account_get_ladder_losses(account,clienttag,id) + account_get_ladder_disconnects(account,clienttag,id);
	  
	  account_adjust_ladder_rating(account,clienttag,id,info[curr].adj);
	  
	  war3_ladder_update(ladder_cr(clienttag,id),uid,won,account_get_ladder_rating(account,clienttag,id),players[curr],0);

	  if (results[curr]!=game_result_draw)
	        war3_ladder_update(ladder_cg(clienttag,id),uid,info[curr].adj,wins,players[curr],0);
		
	  if (results[curr]==game_result_win)
		war3_ladder_update(ladder_cw(clienttag,id),uid,info[curr].adj,games,players[curr],0);
	}
	
	ladder_update_all_accounts();
    
    return 0;
}


extern int ladder_check_map(char const * mapname, t_game_maptype maptype, char const * clienttag)
{
    if (!mapname)
    {
	eventlog(eventlog_level_error,"ladder_check_map","got NULL mapname");
	return -1;
    }
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"ladder_check_map","got bad clienttag");
	return -1;
    }
    
    eventlog(eventlog_level_debug,"ladder_check_map","checking mapname \"%s\" maptype=%d",mapname,(int)maptype);
    if (maptype==game_maptype_ladder) /* FIXME: what about Ironman? */
	return 1;
    
    return 0;
}


extern t_account * ladder_get_account_by_rank(unsigned int rank, t_ladder_sort lsort, t_ladder_time ltime, char const * clienttag, t_ladder_id id)
{
    int dummy;
    
    if (rank<1)
    {
	eventlog(eventlog_level_error,"ladder_get_account_by_rank","got zero rank");
	return NULL;
    }
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"ladder_get_account_by_rank","got bad clienttag");
	return NULL;
    }
    
    switch (lsort)
    {
    case ladder_sort_highestrated:
		if (ltime == ladder_time_current)
			return ladder_get_account(ladder_cr(clienttag,id),rank,&dummy,clienttag);
		else
			return ladder_get_account(ladder_ar(clienttag,id),rank,&dummy,clienttag);
	break;
    case ladder_sort_mostwins:
		if (ltime == ladder_time_current)
			return ladder_get_account(ladder_cw(clienttag,id),rank,&dummy,clienttag);
		else
			return ladder_get_account(ladder_aw(clienttag,id),rank,&dummy,clienttag);
	break;
    case ladder_sort_mostgames:
		if (ltime == ladder_time_current)
			return ladder_get_account(ladder_cg(clienttag,id),rank,&dummy,clienttag);
		else
			return ladder_get_account(ladder_ag(clienttag,id),rank,&dummy,clienttag);
	break;
    default:
	eventlog(eventlog_level_error,"ladder_get_account_by_rank","got bad ladder sort %u",(unsigned int)lsort);
	return NULL;
    }
}


extern unsigned int ladder_get_rank_by_account(t_account * account, t_ladder_sort lsort, t_ladder_time ltime, char const * clienttag, t_ladder_id id)
{
    int uid;

    if (!account)
    {
	eventlog(eventlog_level_error,"ladder_get_rank_by_account","got NULL account");
	return 0;
    }
    if (!clienttag || strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"ladder_get_rank_by_account","got bad clienttag");
	return 0;
    }
    
	uid = account_get_uid(account);

    switch (lsort)
    {
    case ladder_sort_highestrated:
		if (ltime == ladder_time_current)
			return ladder_get_rank(ladder_cr(clienttag,id),uid,0,clienttag);
		else
			return ladder_get_rank(ladder_ar(clienttag,id),uid,0,clienttag);
	break;
    case ladder_sort_mostwins:
		if (ltime == ladder_time_current)
			return ladder_get_rank(ladder_cw(clienttag,id),uid,0,clienttag);
		else
			return ladder_get_rank(ladder_aw(clienttag,id),uid,0,clienttag);
	break;
    case ladder_sort_mostgames:
		if (ltime == ladder_time_current)
			return ladder_get_rank(ladder_cg(clienttag,id),uid,0,clienttag);
		else
			return ladder_get_rank(ladder_ag(clienttag,id),uid,0,clienttag);
	break;
    default:
	eventlog(eventlog_level_error,"ladder_get_rank_by_account","got bad ladder sort %u",(unsigned int)lsort);
	return 0;
    }
    
    return 0;
}

int in_same_team(t_account * acc1, t_account * acc2, unsigned int tc1, unsigned int tc2, char const * clienttag)
{
   char const * teammembers1;
   char const * teammembers2;
   teammembers1 = account_get_atteammembers(acc1,tc1,clienttag);
   teammembers2 = account_get_atteammembers(acc2,tc2,clienttag);
   if ((teammembers1 == NULL) || (teammembers2 == NULL))
   {
      if (teammembers1 != NULL) return 1; else return 0;
   }
   if (strcmp(teammembers1, teammembers2)==0) return 1;
   else return 0;
}

extern t_ladder * solo_ladder(char const * clienttag)
{
  if (strcmp(clienttag,CLIENTTAG_WARCRAFT3)==0) 
    return &WAR3_solo_ladder;
  else 
    return &W3XP_solo_ladder;
}

extern t_ladder * team_ladder(char const * clienttag)
{
  if (strcmp(clienttag,CLIENTTAG_WARCRAFT3)==0) 
    return &WAR3_team_ladder;
  else 
    return &W3XP_team_ladder;
}

extern t_ladder * ffa_ladder(char const * clienttag)
{
  if (strcmp(clienttag,CLIENTTAG_WARCRAFT3)==0) 
    return &WAR3_ffa_ladder;
  else 
    return &W3XP_ffa_ladder;
}

extern t_ladder * at_ladder(char const * clienttag)
{
  if (strcmp(clienttag,CLIENTTAG_WARCRAFT3)==0) 
    return &WAR3_at_ladder;
  else 
    return &W3XP_at_ladder;
}

 extern t_ladder * ladder_ar(char const * clienttag, t_ladder_id ladder_id)
 {
   if (strcmp(clienttag, CLIENTTAG_STARCRAFT)==0)
	   return &STAR_active_rating;
   else if (strcmp(clienttag, CLIENTTAG_BROODWARS)==0)
	   return &SEXP_active_rating;
   else if (strcmp(clienttag, CLIENTTAG_WARCIIBNE)==0)
   {
           if (ladder_id == ladder_id_normal)
	           return &W2BN_active_rating;
	   else
	           return &W2BN_active_rating_ironman;
   }
   else return NULL;
 }

 extern t_ladder * ladder_aw(char const * clienttag, t_ladder_id ladder_id)
 {
   if (strcmp(clienttag, CLIENTTAG_STARCRAFT)==0)
	   return &STAR_active_wins;
   else if (strcmp(clienttag, CLIENTTAG_BROODWARS)==0)
	   return &SEXP_active_wins;
   else if (strcmp(clienttag, CLIENTTAG_WARCIIBNE)==0)
   {
           if (ladder_id == ladder_id_normal)
	           return &W2BN_active_wins;
	   else
	           return &W2BN_active_wins_ironman;
   }
   else return NULL;
 }

 extern t_ladder * ladder_ag(char const * clienttag, t_ladder_id ladder_id)
 {
   if (strcmp(clienttag, CLIENTTAG_STARCRAFT)==0)
	   return &STAR_active_games;
   else if (strcmp(clienttag, CLIENTTAG_BROODWARS)==0)
	   return &SEXP_active_games;
   else if (strcmp(clienttag, CLIENTTAG_WARCIIBNE)==0)
   {
           if (ladder_id == ladder_id_normal)
	           return &W2BN_active_games;
	   else
	           return &W2BN_active_games_ironman;
   }
   else return NULL;
 }

 extern t_ladder * ladder_cr(char const * clienttag, t_ladder_id ladder_id)
{
   if (strcmp(clienttag, CLIENTTAG_STARCRAFT)==0)
	   return &STAR_current_rating;
   else if (strcmp(clienttag, CLIENTTAG_BROODWARS)==0)
	   return &SEXP_current_rating;
   else if (strcmp(clienttag, CLIENTTAG_WARCIIBNE)==0)
   {
	   if (ladder_id == ladder_id_normal)
		   return &W2BN_current_rating;
	   else
		   return &W2BN_current_rating_ironman;
   }
   else return NULL;
 }

 extern t_ladder * ladder_cw(char const * clienttag, t_ladder_id ladder_id)
{
   if (strcmp(clienttag, CLIENTTAG_STARCRAFT)==0)
	   return &STAR_current_wins;
   else if (strcmp(clienttag, CLIENTTAG_BROODWARS)==0)
	   return &SEXP_current_wins;
   else if (strcmp(clienttag, CLIENTTAG_WARCIIBNE)==0)
   {
	   if (ladder_id == ladder_id_normal)
		   return &W2BN_current_wins;
	   else
		   return &W2BN_current_wins_ironman;
   }
   else return NULL;
 }

 extern t_ladder * ladder_cg(char const * clienttag, t_ladder_id ladder_id)
{
   if (strcmp(clienttag, CLIENTTAG_STARCRAFT)==0)
	   return &STAR_current_games;
   else if (strcmp(clienttag, CLIENTTAG_BROODWARS)==0)
	   return &SEXP_current_games;
   else if (strcmp(clienttag, CLIENTTAG_WARCIIBNE)==0)
   {
	   if (ladder_id == ladder_id_normal)
		   return &W2BN_current_games;
	   else
		   return &W2BN_current_games_ironman;
   }
   else return NULL;
 }

char const * ladder_get_clienttag(t_ladder * ladder)
{
  return ladder->clienttag;
}

t_binary_ladder_types w3_ladder_to_binary_ladder_types(t_ladder * ladder)
{
  return ladder->type;
}

t_ladder * binary_ladder_types_to_w3_ladder(t_binary_ladder_types type)
{
  t_ladder * ladder;

  switch (type)
  {
    case WAR3_SOLO:
	ladder = &WAR3_solo_ladder;
	break;
    case WAR3_TEAM:
	ladder = &WAR3_team_ladder;
	break;
    case WAR3_FFA:
	ladder = &WAR3_ffa_ladder;
	break;
    case WAR3_AT:
	ladder = &WAR3_at_ladder;
	break;
    case W3XP_SOLO:
	ladder = &W3XP_solo_ladder;
	break;
    case W3XP_TEAM:
	ladder = &W3XP_team_ladder;
	break;
    case W3XP_FFA:
	ladder = &W3XP_ffa_ladder;
	break;
    case W3XP_AT:
	ladder = &W3XP_at_ladder;
	break;
	case STAR_AR  : 
	ladder = &STAR_active_rating;
	break;
	case STAR_AW  : 
	ladder = &STAR_active_wins;
	break;
	case STAR_AG  : 
	ladder = &STAR_active_games;
	break;
	case STAR_CR  : 
	ladder = &STAR_current_rating;
	break;
	case STAR_CW  : 
	ladder = &STAR_current_wins;
	break;
	case STAR_CG  : 
	ladder = &STAR_current_games;
	break;
	case SEXP_AR  : 
	ladder = &SEXP_active_rating;
	break;
	case SEXP_AW  : 
	ladder = &SEXP_active_wins;
	break;
	case SEXP_AG  : 
	ladder = &SEXP_active_games;
	break;
	case SEXP_CR  : 
	ladder = &SEXP_current_rating;
	break;
	case SEXP_CW  : 
	ladder = &SEXP_current_wins;
	break;
	case SEXP_CG  : 
	ladder = &SEXP_current_games;
	break;
	case W2BN_AR  : 
	ladder = &W2BN_active_rating;
	break;
	case W2BN_AW  : 
	ladder = &W2BN_active_wins;
	break;
	case W2BN_AG  : 
	ladder = &W2BN_active_games;
	break;
	case W2BN_CR  : 
	ladder = &W2BN_current_rating;
	break;
	case W2BN_CW  : 
	ladder = &W2BN_current_wins;
	break;
	case W2BN_CG  : 
	ladder = &W2BN_current_games;
	break;
	case W2BN_ARI : 
	ladder = &W2BN_active_rating_ironman;
	break;
	case W2BN_AWI : 
	ladder = &W2BN_active_wins_ironman;
	break;
	case W2BN_AGI : 
	ladder = &W2BN_active_games_ironman;
	break;
	case W2BN_CRI : 
	ladder = &W2BN_current_rating_ironman;
	break;
	case W2BN_CWI : 
	ladder = &W2BN_current_wins_ironman;
	break;
	case W2BN_CGI : 
	ladder = &W2BN_current_games_ironman;
	break;

    default:
	eventlog(eventlog_level_error,__FUNCTION__,"got invalid ladder type %d",type);
	return NULL;
  }
  return ladder;
}

void ladder_init(t_ladder *ladder,t_binary_ladder_types type, char const * clienttag, t_ladder_id ladder_id)
 {
    ladder->first = NULL;
    ladder->last  = NULL;
    ladder->dirty = 1;
    ladder->type = type;
	ladder->clienttag = clienttag;
	ladder->ladder_id = ladder_id;
 }

void ladder_destroy(t_ladder *ladder)
{
  t_ladder_internal *pointer;

  while (ladder->first!=NULL)
  {
    pointer = ladder->first;
    ladder->first = pointer->prev;
    free((void *)pointer);
  }
}

 extern int war3_ladder_add(t_ladder *ladder, int uid, int xp, int level, t_account *account, unsigned int teamcount,char const * clienttag)
  {

   t_ladder_internal *ladder_entry;
   ladder_entry = malloc(sizeof(t_ladder_internal));

   if (ladder_entry != NULL)
   {
      ladder_entry->uid       = uid;
      ladder_entry->xp        = xp;
      ladder_entry->level     = level;
      ladder_entry->account   = account;
      ladder_entry->teamcount = teamcount;
      ladder_entry->prev      = NULL;
      ladder_entry->next      = NULL;

      if (ladder->first == NULL)
      { // ladder empty, so just insert element
         ladder->first = ladder_entry;
         ladder->last  = ladder_entry;
      }
      else
      { // already elements in list
        // determine if first or last user in ladder has XP
        // closest to current, cause should be the shorter way to insert
        // new user
            if ((ladder->first->xp - xp) >= (xp - ladder->last->xp))
            // we should enter from last to first
            {
              t_ladder_internal *search;
              search = ladder->last;
              while ((search != NULL) && (search->level < level))
              {
                search = search->next;
              }
	      while ((search != NULL) && (search->level == level) && (search->xp < xp))
	      {
                search = search->next;
	      }

	      if (teamcount!=0) // this only happens for atteams
		{
		  t_ladder_internal *teamsearch;
		  t_ladder_internal *teamfound;

		  teamsearch = search;
		  teamfound = NULL;

		  while ((teamsearch != NULL) && (teamsearch->xp == xp) && (teamsearch->level == level))
		    {
		      if (in_same_team(account,teamsearch->account,teamcount,teamsearch->teamcount,clienttag))
			{
			  teamfound = teamsearch;
			  break;
			}
		      teamsearch = teamsearch->next;
		    }
		  if (teamfound!=NULL) search = teamfound;

		}

              if (search == NULL)
              {
                ladder->first->next = ladder_entry;
                ladder_entry->prev  = ladder->first;
                ladder->first       = ladder_entry;
              }
              else
              {
                 ladder_entry->next = search;
                 ladder_entry->prev = search->prev;
                 if (search == ladder->last)
                 // enter at end of list
                 {
                  search->prev = ladder_entry;
                  ladder->last = ladder_entry;
                 }
                 else
                 {
                   search->prev->next = ladder_entry;
                   search->prev       = ladder_entry;
                 }
              }
            }
            else
            // start from first and the search towards last
            {
              t_ladder_internal *search;
              search = ladder->first;
	      while ((search != NULL) && (search->level > level))
	      {
		search = search->prev;
	      }
              while ((search != NULL) && (search->level == level) && (search->xp > xp))
              {
                search = search->prev;
              }

	      if (teamcount!=0) // this only happens for atteams
		{
		  t_ladder_internal *teamsearch;
		  t_ladder_internal *teamfound;

		  teamsearch = search;
		  teamfound = NULL;

		  while ((teamsearch != NULL) && (teamsearch->xp == xp))
		    {
		      if (in_same_team(account,teamsearch->account,teamcount,teamsearch->teamcount,clienttag))
			{
			  teamfound = teamsearch;
			  break;
			}
		      teamsearch = teamsearch->prev;
		    }
		  if (teamfound!=NULL) search = teamfound;

		}


              if (search == NULL)
              {
                ladder->last->prev = ladder_entry;
                ladder_entry->next = ladder->last;
                ladder->last       = ladder_entry;
              }
              else
              {
                 ladder_entry->prev = search;
                 ladder_entry->next = search->next;
                 if (search == ladder->first)
                 // enter at beginning of list
                 {
                  search->next  = ladder_entry;
                  ladder->first = ladder_entry;
                 }
                 else
                 {
                   search->next->prev = ladder_entry;
                   search->next       = ladder_entry;
                 }
              }
            }
       }
       ladder->dirty = 1;
       return 0;
   }
   else
   {
     eventlog(eventlog_level_error,"war3_ladder_add","could not add account to ladder");
     return -1;
   }
 }

 extern int war3_ladder_update(t_ladder *ladder, int uid, int xp, int level, t_account *account, unsigned int teamcount)
 {
   t_ladder_internal *search, *newpos;
   search = ladder->first;
   while (search && ((search->uid != uid) || (search->teamcount!=teamcount))) { search = search->prev; }
   if (search != NULL)
   {
     search->xp += xp;
     search->level = level;

     // make sure, we don't get negative XP
     if (search->xp < 0) search->xp = 0;

     if (xp>0)
     // XP gained.... maybe getting better ranking
     {
        newpos = search->next;
	while ((newpos != NULL) && (newpos->level < level))
	{
	  newpos = newpos->next;
	}
        while ((newpos != NULL) && (newpos->level == level) && (newpos->xp < search->xp))
        {
          newpos = newpos->next;
        }
        if (newpos != search->next)
        // so we really have to change ranking now
        {
	  // we are changing something in the ladder
          ladder->dirty = 1;
          // first close gap, where we've been...
          search->next->prev = search->prev;
          if (search->prev != NULL) 
            search->prev->next = search->next;
          else
            ladder->last = search->next;
          // and then move to new position
          search->next = newpos;
          if (newpos == NULL)
          {
             search->prev = ladder->first;
             ladder->first->next = search;
             ladder->first       = search;
          }
          else
          {
            search->prev = newpos->prev;
            newpos->prev->next = search;
            newpos->prev = search;
          }
        }
     }
     if (xp<0)
     // XP lost.... maybe ranking gets worse
     {
        newpos = search->prev;
	while ((newpos != NULL) && (newpos->level > level))
	{
	  newpos = newpos->prev;
	}
        while ((newpos != NULL) && (newpos->level == level) && (newpos->xp > search->xp))
        {
          newpos = newpos->prev;
        }
        if (newpos != search->prev)
        // so we really have to change ranking now
        {
	  // we are changing something in the ladder
          ladder->dirty = 1;
          // first close gap, where we've been...
          search->prev->next = search->next;
          if (search->next != NULL) 
            search->next->prev = search->prev;
          else
            ladder->first = search->prev;
          // and then move to new position
          search->prev = newpos;
          if (newpos == NULL)
          {
             search->next = ladder->last;
             ladder->last->prev = search;
             ladder->last       = search;
          }
          else
          {
            search->next = newpos->next;
            newpos->next->prev = search;
            newpos->next = search;
          }
        }
     }
     return 0;
   }
   else
   return -1;
 }

 extern int ladder_get_rank(t_ladder *ladder, int uid, unsigned int teamcount, char const * clienttag)
 {
   int ranking = 1;
   t_ladder_internal *search;
   search = ladder->first;
   while ((search!=NULL) && ((search->uid != uid) || (search->teamcount!=teamcount))) 
   { 
     search = search->prev; 
     ranking++;
     if (ladder==at_ladder(clienttag))
     {
       // if in the same team as previous account
       if ((search) && (search->next) 
           && in_same_team(search->account,search->next->account,teamcount,search->next->teamcount,clienttag))
	       ranking--;
     }
   }
   if (search != NULL)
   {
     return ranking;
   }
   else
   {
     return 0;
   }
 }

t_ladder_internal * ladder_get_rank_internal(t_ladder * ladder,int rank, char const * clienttag)
{
  int ranking;
  t_ladder_internal *search;

  // this should be a huge speedup when getting many subsequent entries from ladder
  // like used in saving of binary ladders
  if ((last_ladder == ladder) && (last_rank < rank) && (last_internal != NULL))
    {
      ranking = last_rank;
      search = last_internal;
    }
  else
    {
      ranking = 1;
      search = ladder->first;
    }

  while ((search!=NULL) && (ranking<rank)) 
  { 
     search = search->prev; 
     ranking++;
     if (ladder == at_ladder(clienttag))
     {
       if ((search) && (search->next)
	  && in_same_team(search->account,search->next->account,search->teamcount,search->next->teamcount,clienttag))
	      ranking--;
     }
  }
  last_ladder   = ladder;
  last_internal = search;
  last_rank     = rank;
  return search;
}

extern t_account * ladder_get_account(t_ladder *ladder, int rank, unsigned int * teamcount,char const * clienttag)
{
  t_ladder_internal *search;

  search = ladder_get_rank_internal(ladder,rank,clienttag);

  if (search) 
  {
    *teamcount = search->teamcount;
    return search->account;
  }
  else
  {
    *teamcount = 0;
    return NULL;
  }
}

extern int ladder_update_accounts(t_ladder *ladder, int (*set_fct)(), int (*get_fct1)())
{
    t_ladder_internal *pointer, *tmp_pointer;
    t_account *account;
    char const * clienttag;
    int rank = 1;
    int update = 0;

    if (ladder->dirty == 1)
    {
      if ((set_fct!=NULL) && (get_fct1!=NULL))
      {
      clienttag = ladder_get_clienttag(ladder);
      pointer = ladder->first;
      while (pointer!=NULL)
      {
         account = pointer->account;
         if (rank <= MaxRankKeptInLadder) 
         {
           if (ladder->ladder_id == ladder_id_none) //war3/w3xp
		   {
             if ((ladder!=at_ladder(clienttag)))
	         {
               if ((*get_fct1)(account,clienttag)!=rank)
 	           {	
	             (*set_fct)(account,clienttag,rank);
	             update++;	
	           }
	         }
	         else
	         {
	           if ((*get_fct1)(account,pointer->teamcount,clienttag)!=rank)
	           {
	             (*set_fct)(account,pointer->teamcount,clienttag,rank);
	             update++;
	             // if in the same team as previous account
	             if ((pointer) && (pointer->next) && 
                     in_same_team(pointer->account,pointer->next->account,pointer->teamcount,pointer->next->teamcount,clienttag))
	               rank--;
               }
	         }
		   }
		   else //other clienttags...
		   {
		     if ((*get_fct1)(account,clienttag,ladder->ladder_id)!=rank)
                     {	
		       (*set_fct)(account,clienttag,ladder->ladder_id,rank);
	               update++;	
		     }
		   }

	   pointer=pointer->prev;
	   rank++;
	  
         }
         else
	 {
	   // leave while loop
           break;
	 }
      }
      while (pointer!=NULL)
      {
        // all accounts following now are out of the ladder range we keep track of....
	    // so set rank to 0 and remove account from ladder
        if (ladder->ladder_id == ladder_id_none) //war3/w3xp
		{
	      if (ladder!=at_ladder(clienttag))
	        { if ((*get_fct1)(account,clienttag)!=0) (*set_fct)(account,clienttag,0);}
	      else
	        { if ((*get_fct1)(account,pointer->teamcount,clienttag)!=0) (*set_fct)(account,pointer->teamcount,clienttag,0);}
		}
		else
		{ if ((*get_fct1)(account,clienttag,ladder->ladder_id)!=0) (*set_fct)(account,clienttag,ladder->ladder_id,0);}
	// remove account from ladder
	if (pointer->next!=NULL) pointer->next->prev = pointer->prev;
	if (pointer->prev!=NULL) pointer->prev->next = pointer->next;
	if (ladder->last == pointer)  ladder->last  = pointer->next;
	if (ladder->first == pointer) ladder->first = pointer->prev;
	tmp_pointer = pointer->prev;
	free((void *)pointer);
	pointer = tmp_pointer;
      }
      }
      binary_ladder_save(w3_ladder_to_binary_ladder_types(ladder),4,&ladder_get_from_ladder);
      if (update != 0)
        eventlog(eventlog_level_info,"ladder_update_accounts","updated %u accounts for clienttag %s",update,ladder->clienttag);
    }
    ladder->dirty = 0;
    return 0;
}

extern int ladder_update_all_accounts(void)
{
  eventlog(eventlog_level_info,"ladder_update_all_accounts","updating ranking for all accounts");
  ladder_update_accounts(&WAR3_solo_ladder,&account_set_solorank,   &account_get_solorank);
  ladder_update_accounts(&WAR3_team_ladder,&account_set_teamrank,   &account_get_teamrank);
  ladder_update_accounts(&WAR3_ffa_ladder, &account_set_ffarank,    &account_get_ffarank);
  ladder_update_accounts(&WAR3_at_ladder,  &account_set_atteamrank, &account_get_atteamrank);
  ladder_update_accounts(&W3XP_solo_ladder,&account_set_solorank,   &account_get_solorank);
  ladder_update_accounts(&W3XP_team_ladder,&account_set_teamrank,   &account_get_teamrank);
  ladder_update_accounts(&W3XP_ffa_ladder, &account_set_ffarank,    &account_get_ffarank);
  ladder_update_accounts(&W3XP_at_ladder,  &account_set_atteamrank, &account_get_atteamrank);
  ladder_update_accounts(&STAR_current_rating, &account_set_ladder_rank, &account_get_ladder_rank);
  ladder_update_accounts(&STAR_current_wins,  NULL,                      NULL);
  ladder_update_accounts(&STAR_current_games, NULL,                      NULL);
  ladder_update_accounts(&SEXP_current_rating, &account_set_ladder_rank, &account_get_ladder_rank);
  ladder_update_accounts(&SEXP_current_wins,  NULL,                      NULL);
  ladder_update_accounts(&SEXP_current_games, NULL,                      NULL);
  ladder_update_accounts(&STAR_active_rating, &account_set_ladder_active_rank, &account_get_ladder_active_rank);
  ladder_update_accounts(&STAR_active_wins,   NULL,                      NULL);
  ladder_update_accounts(&STAR_active_games,  NULL,                      NULL);
  ladder_update_accounts(&SEXP_active_rating, &account_set_ladder_active_rank, &account_get_ladder_active_rank);
  ladder_update_accounts(&SEXP_active_wins,   NULL,                      NULL);
  ladder_update_accounts(&SEXP_active_games,  NULL,                      NULL);
  ladder_update_accounts(&W2BN_current_rating, &account_set_ladder_rank, &account_get_ladder_rank);
  ladder_update_accounts(&W2BN_current_wins,  NULL,                      NULL);
  ladder_update_accounts(&W2BN_current_games, NULL,                      NULL);
  ladder_update_accounts(&W2BN_active_rating, &account_set_ladder_rank, &account_get_ladder_rank);
  ladder_update_accounts(&W2BN_active_wins,   NULL,                      NULL);
  ladder_update_accounts(&W2BN_active_games,  NULL,                      NULL);
  ladder_update_accounts(&W2BN_current_rating_ironman, &account_set_ladder_rank, &account_get_ladder_rank);
  ladder_update_accounts(&W2BN_current_wins_ironman,  NULL,                      NULL);
  ladder_update_accounts(&W2BN_current_games_ironman, NULL,                      NULL);
  ladder_update_accounts(&W2BN_active_rating_ironman, &account_set_ladder_rank, &account_get_ladder_rank);
  ladder_update_accounts(&W2BN_active_wins_ironman,  NULL,                      NULL);
  ladder_update_accounts(&W2BN_active_games_ironman, NULL,                      NULL);

  eventlog(eventlog_level_info,"ladder_update_all_accounts","finished updating ranking for all accounts");
  return 0;
}

t_binary_ladder_load_result binary_load(t_binary_ladder_types type)
{
  t_binary_ladder_load_result result;
  t_ladder * ladder;

  result = binary_ladder_load(type,4,&ladder_put_into_ladder);
  if (result == illegal_checksum)
  {
    char const * clienttag;
	t_ladder_id ladder_id;

    ladder = binary_ladder_types_to_w3_ladder(type);
	clienttag = ladder_get_clienttag(ladder);
	ladder_id = ladder->ladder_id;
    ladder_destroy(ladder);
    ladder_init(ladder,type,clienttag,ladder_id);
  }  
  return result;
}

extern void ladders_load_accounts_to_ladderlists(void)
{
  // use new binary ladder here
  t_entry   * curr;
  t_account * account;
  int teamcount,xp;

  t_binary_ladder_load_result war3_solo_res, war3_team_res, war3_ffa_res, war3_at_res;
  t_binary_ladder_load_result w3xp_solo_res, w3xp_team_res, w3xp_ffa_res, w3xp_at_res;
  t_binary_ladder_load_result star_ar_res, star_aw_res, star_ag_res, star_cr_res, star_cw_res, star_cg_res;
  t_binary_ladder_load_result sexp_ar_res, sexp_aw_res, sexp_ag_res, sexp_cr_res, sexp_cw_res, sexp_cg_res;
  t_binary_ladder_load_result w2bn_cr_res, w2bn_cw_res, w2bn_cg_res, w2bn_cri_res, w2bn_cwi_res, w2bn_cgi_res;
  t_binary_ladder_load_result w2bn_ar_res, w2bn_aw_res, w2bn_ag_res, w2bn_ari_res, w2bn_awi_res, w2bn_agi_res;

  war3_solo_res = binary_load(WAR3_SOLO);
  war3_team_res = binary_load(WAR3_TEAM);
  war3_ffa_res  = binary_load(WAR3_FFA);
  war3_at_res   = binary_load(WAR3_AT);
  w3xp_solo_res = binary_load(W3XP_SOLO);
  w3xp_team_res = binary_load(W3XP_TEAM);
  w3xp_ffa_res  = binary_load(W3XP_FFA);
  w3xp_at_res   = binary_load(W3XP_AT);
  star_ar_res   = binary_load(STAR_AR);
  star_aw_res   = binary_load(STAR_AW);
  star_ag_res   = binary_load(STAR_AG);
  star_cr_res   = binary_load(STAR_CR);
  star_cw_res   = binary_load(STAR_CW);
  star_cg_res   = binary_load(STAR_CG);
  sexp_ar_res   = binary_load(SEXP_AR);
  sexp_aw_res   = binary_load(SEXP_AW);
  sexp_ag_res   = binary_load(SEXP_AG);
  sexp_cr_res   = binary_load(SEXP_CR);
  sexp_cw_res   = binary_load(SEXP_CW);
  sexp_cg_res   = binary_load(SEXP_CG);
  w2bn_cr_res   = binary_load(W2BN_CR);
  w2bn_cw_res   = binary_load(W2BN_CW);
  w2bn_cg_res   = binary_load(W2BN_CG);
  w2bn_cri_res  = binary_load(W2BN_CRI);
  w2bn_cwi_res  = binary_load(W2BN_CWI);
  w2bn_cgi_res  = binary_load(W2BN_CGI);
  w2bn_ar_res   = binary_load(W2BN_AR);
  w2bn_aw_res   = binary_load(W2BN_AW);
  w2bn_ag_res   = binary_load(W2BN_AG);
  w2bn_ari_res  = binary_load(W2BN_ARI);
  w2bn_awi_res  = binary_load(W2BN_AWI);
  w2bn_agi_res  = binary_load(W2BN_AGI);

  if ((war3_solo_res + war3_team_res + war3_ffa_res + war3_at_res +
       w3xp_solo_res + w3xp_team_res + w3xp_ffa_res + w3xp_at_res +
       star_ar_res   + star_aw_res   + star_ag_res  + star_cr_res + star_cw_res + star_cg_res +
       sexp_ar_res   + sexp_aw_res   + sexp_ag_res  + sexp_cr_res + sexp_cw_res + sexp_cg_res +
       w2bn_cr_res   + w2bn_cw_res   + w2bn_cg_res  + 
       w2bn_cri_res  + w2bn_cwi_res  + w2bn_cgi_res +
       w2bn_ar_res   + w2bn_aw_res   + w2bn_ag_res  + 
       w2bn_ari_res  + w2bn_awi_res  + w2bn_agi_res ) == load_success)
  {
    eventlog(eventlog_level_trace,__FUNCTION__,"everything went smooth... taking shortcut");
    return;
  }
      
  HASHTABLE_TRAVERSE(accountlist(),curr)
    {
      if ((account=((t_account *)entry_get_data(curr))))
	  {
	      int rating, wins;
	      int uid = account_get_uid(account);

	      if ((war3_solo_res!=load_success) && ((xp = account_get_soloxp(account,CLIENTTAG_WARCRAFT3))))
		  {
		  war3_ladder_add(&WAR3_solo_ladder,
				  uid, xp,
				  account_get_sololevel(account,CLIENTTAG_WARCRAFT3),
				  account,0,CLIENTTAG_WARCRAFT3);
		  }
	      if ((war3_team_res!=load_success) && ((xp = account_get_teamxp(account,CLIENTTAG_WARCRAFT3))))
		  {
		  war3_ladder_add(&WAR3_team_ladder,
				  uid, xp,
				  account_get_teamlevel(account,CLIENTTAG_WARCRAFT3),
				  account,0,CLIENTTAG_WARCRAFT3 );

		  }
	      if ((war3_ffa_res!=load_success) && ((xp = account_get_ffaxp(account,CLIENTTAG_WARCRAFT3))))
		  {
		  war3_ladder_add(&WAR3_ffa_ladder,
				  uid, xp,
				  account_get_ffalevel(account,CLIENTTAG_WARCRAFT3),
				  account,0,CLIENTTAG_WARCRAFT3 );
		  }
	      // user is part of a team
	      if ((war3_at_res!=load_success) && ((teamcount = account_get_atteamcount(account,CLIENTTAG_WARCRAFT3))))
		  {
		  int counter;
		  // for each team he is in...
		  for (counter=1; counter<=teamcount; counter++)
		    {
		      // make sure... user has ranking... and team is valid...
		      if ((xp = account_get_atteamxp(account,counter,CLIENTTAG_WARCRAFT3)) && 
				   account_get_atteammembers(account,counter,CLIENTTAG_WARCRAFT3))
			  {
			  war3_ladder_add(&WAR3_at_ladder,
					  uid, xp,
					  account_get_atteamlevel(account,counter,CLIENTTAG_WARCRAFT3),
					  account,
					  counter,CLIENTTAG_WARCRAFT3);
			  }
		    }	   
		  }
	      if ((w3xp_solo_res!=load_success) && ((xp = account_get_soloxp(account,CLIENTTAG_WAR3XP))))
		  {
		  war3_ladder_add(&W3XP_solo_ladder,
				  uid, xp,
				  account_get_sololevel(account,CLIENTTAG_WAR3XP),
				  account,0,CLIENTTAG_WAR3XP);
		  }
	      if ((w3xp_team_res!=load_success) && ((xp = account_get_teamxp(account,CLIENTTAG_WAR3XP))))
		  {
		  war3_ladder_add(&W3XP_team_ladder,
				  uid, xp,
				  account_get_teamlevel(account,CLIENTTAG_WAR3XP),
				  account,0,CLIENTTAG_WAR3XP );

		  }
	      if ((w3xp_ffa_res!=load_success) && ((xp = account_get_ffaxp(account,CLIENTTAG_WAR3XP))))
		  {
		  war3_ladder_add(&W3XP_ffa_ladder,
				  uid, xp,
				  account_get_ffalevel(account,CLIENTTAG_WAR3XP),
				  account,0,CLIENTTAG_WAR3XP );
		  }
	      // user is part of a team
	      if ((w3xp_at_res!=load_success) && ((teamcount = account_get_atteamcount(account,CLIENTTAG_WAR3XP))))
	 	  {
		  int counter;
		  // for each team he is in...
		  for (counter=1; counter<=teamcount; counter++)
		    {

		      // make sure... user has ranking... and team is valid...
		      if ((xp = account_get_atteamxp(account,counter,CLIENTTAG_WAR3XP)) && 
				   account_get_atteammembers(account,counter,CLIENTTAG_WAR3XP))
			  {
			  war3_ladder_add(&W3XP_at_ladder,
					  uid, xp,
					  account_get_atteamlevel(account,counter,CLIENTTAG_WAR3XP),
					  account,
					  counter,CLIENTTAG_WAR3XP);
			  }
		    }	   
		  }

		  if ((rating = account_get_ladder_rating(account,CLIENTTAG_STARCRAFT,ladder_id_normal))>0)
		  {
		    wins   = account_get_ladder_wins(account,CLIENTTAG_STARCRAFT,ladder_id_normal);
		    
		    if (star_cr_res!=load_success)
	     	    {
		       war3_ladder_add(&STAR_current_rating, uid,wins,rating,account,0,CLIENTTAG_STARCRAFT);
	 	    }
		    
		    if (star_cw_res!=load_success)
	     	    {
	 	       war3_ladder_add(&STAR_current_wins, uid,rating,wins,account,0,CLIENTTAG_STARCRAFT);
	 	    }
		    
		    if (star_cg_res!=load_success)
	     	    {
	 	      int games = wins +
		                  account_get_ladder_losses(account,CLIENTTAG_STARCRAFT,ladder_id_normal)+
	                          account_get_ladder_disconnects(account,CLIENTTAG_STARCRAFT,ladder_id_normal);

	 	      war3_ladder_add(&STAR_current_games, uid,rating,games,account,0,CLIENTTAG_STARCRAFT);
	     	    }
		  }

		  if ((rating = account_get_ladder_active_rating(account,CLIENTTAG_STARCRAFT,ladder_id_normal))>0)
		  {
		    wins   = account_get_ladder_active_wins(account,CLIENTTAG_STARCRAFT,ladder_id_normal);
		    
	 	    if (star_ar_res!=load_success)
	 	    {
		      war3_ladder_add(&STAR_active_rating, uid,wins,rating,account,0,CLIENTTAG_STARCRAFT);
	 	    }
		    
		    if (star_aw_res!=load_success)
	     	    {
		      war3_ladder_add(&STAR_active_wins, uid,rating,wins,account,0,CLIENTTAG_STARCRAFT);
	 	    }
		    
		    if (star_ag_res!=load_success)
	     	    {
		      int games = wins +
			     	  account_get_ladder_active_losses(account,CLIENTTAG_STARCRAFT,ladder_id_normal)+
	                     	  account_get_ladder_active_disconnects(account,CLIENTTAG_STARCRAFT,ladder_id_normal);

	 	      war3_ladder_add(&STAR_active_games, uid,rating,games,account,0,CLIENTTAG_STARCRAFT);
	     	    }
		  }

		  if ((rating = account_get_ladder_rating(account,CLIENTTAG_BROODWARS,ladder_id_normal))>0)
		  {
		    wins = account_get_ladder_wins(account,CLIENTTAG_BROODWARS,ladder_id_normal);
		    
		    if (sexp_cr_res!=load_success)
	     	    {
		      war3_ladder_add(&SEXP_current_rating, uid,0*wins,rating,account,0,CLIENTTAG_BROODWARS);
	 	    }
		    
		    if (sexp_cw_res!=load_success)
	     	    {
	 	      war3_ladder_add(&SEXP_current_wins, uid,rating,wins,account,0,CLIENTTAG_BROODWARS);
	 	    }
		    
		    if (sexp_cg_res!=load_success)
	     	    {
	 	      int games = wins +
		 		  account_get_ladder_losses(account,CLIENTTAG_BROODWARS,ladder_id_normal)+
	         	          account_get_ladder_disconnects(account,CLIENTTAG_BROODWARS,ladder_id_normal);

	 	      war3_ladder_add(&SEXP_current_games, uid,rating,games,account,0,CLIENTTAG_BROODWARS);
	     	    }
		  }

		  if ((rating = account_get_ladder_active_rating(account,CLIENTTAG_BROODWARS,ladder_id_normal))>0)
		  {
		    wins = account_get_ladder_active_wins(account,CLIENTTAG_BROODWARS,ladder_id_normal);
		    
	 	    if (sexp_ar_res!=load_success)
	 	    {
		      war3_ladder_add(&SEXP_active_rating, uid,wins,rating,account,0,CLIENTTAG_BROODWARS);
	 	    }
		    
		    if (sexp_aw_res!=load_success)
	     	    {
		      war3_ladder_add(&SEXP_active_wins, uid,rating,wins,account,0,CLIENTTAG_BROODWARS);
	 	    }
		    
		    if (sexp_ag_res!=load_success)
	     	    {
		      int games = wins +
			     	  account_get_ladder_active_losses(account,CLIENTTAG_BROODWARS,ladder_id_normal)+
	                     	  account_get_ladder_active_disconnects(account,CLIENTTAG_BROODWARS,ladder_id_normal);

	 	       war3_ladder_add(&SEXP_active_games, uid,rating,games,account,0,CLIENTTAG_BROODWARS);
	     	    }
		  }

		  if ((rating = account_get_ladder_rating(account,CLIENTTAG_WARCIIBNE,ladder_id_normal))>0)
		  {
		    wins = account_get_ladder_wins(account,CLIENTTAG_WARCIIBNE,ladder_id_normal);
		    
		    if (w2bn_cr_res!=load_success)
	     	    {
		      war3_ladder_add(&W2BN_current_rating, uid,wins,rating,account,0,CLIENTTAG_WARCIIBNE);
	 	    }
		    
		    if (w2bn_cw_res!=load_success)
	     	    {
	 	      war3_ladder_add(&W2BN_current_wins, uid,rating,wins,account,0,CLIENTTAG_WARCIIBNE);
	 	    }
		    
		    if (w2bn_cg_res!=load_success)
	     	    {
	 	      int games = wins +
		 		  account_get_ladder_losses(account,CLIENTTAG_WARCIIBNE,ladder_id_normal)+
	         	          account_get_ladder_disconnects(account,CLIENTTAG_WARCIIBNE,ladder_id_normal);

	 	      war3_ladder_add(&W2BN_current_games, uid,rating,games,account,0,CLIENTTAG_WARCIIBNE);
	     	    }
		  }

		  if ((rating = account_get_ladder_active_rating(account,CLIENTTAG_WARCIIBNE,ladder_id_normal))>0)
		  {
		    wins = account_get_ladder_active_wins(account,CLIENTTAG_WARCIIBNE,ladder_id_normal);
		    
		    if (w2bn_ar_res!=load_success)
	     	    {
		      war3_ladder_add(&W2BN_active_rating, uid,wins,rating,account,0,CLIENTTAG_WARCIIBNE);
	 	    }
		    
		    if (w2bn_aw_res!=load_success)
	     	    {
	 	      war3_ladder_add(&W2BN_active_wins, uid,rating,wins,account,0,CLIENTTAG_WARCIIBNE);
	 	    }
		    
		    if (w2bn_ag_res!=load_success)
	     	    {
	 	      int games = wins +
		 		  account_get_ladder_active_losses(account,CLIENTTAG_WARCIIBNE,ladder_id_normal)+
	         	          account_get_ladder_active_disconnects(account,CLIENTTAG_WARCIIBNE,ladder_id_normal);

	 	      war3_ladder_add(&W2BN_active_games, uid,rating,games,account,0,CLIENTTAG_WARCIIBNE);
	     	    }
		  }

		  if ((rating = account_get_ladder_rating(account,CLIENTTAG_WARCIIBNE,ladder_id_ironman))>0)
		  {
	            wins = account_get_ladder_wins(account,CLIENTTAG_WARCIIBNE,ladder_id_ironman);
		    
		    if (w2bn_cri_res!=load_success)
	     	    {
		      war3_ladder_add(&W2BN_current_rating_ironman, uid,wins,rating,account,0,CLIENTTAG_WARCIIBNE);	
	 	    }
		    
		    if (w2bn_cwi_res!=load_success)
	     	    {
	 	      war3_ladder_add(&W2BN_current_wins_ironman, uid,rating,wins,account,0,CLIENTTAG_WARCIIBNE);
	 	    }
		    
		    if (w2bn_cgi_res!=load_success)
	     	    {
	 	      int games = wins +
		 		  account_get_ladder_losses(account,CLIENTTAG_WARCIIBNE,ladder_id_ironman)+
	         	          account_get_ladder_disconnects(account,CLIENTTAG_WARCIIBNE,ladder_id_ironman);

	 	      war3_ladder_add(&W2BN_current_games_ironman, uid,rating,games,account,0,CLIENTTAG_WARCIIBNE);
	     	    }
		  }

		  if ((rating = account_get_ladder_active_rating(account,CLIENTTAG_WARCIIBNE,ladder_id_ironman))>0)
		  {
		    wins = account_get_ladder_active_wins(account,CLIENTTAG_WARCIIBNE,ladder_id_ironman);
		    
		    if (w2bn_ari_res!=load_success)
	     	    {
		      war3_ladder_add(&W2BN_active_rating_ironman, uid,wins,rating,account,0,CLIENTTAG_WARCIIBNE);
	 	    }
		    
		    if (w2bn_awi_res!=load_success)
	     	    {
	 	      war3_ladder_add(&W2BN_active_wins_ironman, uid,rating,wins,account,0,CLIENTTAG_WARCIIBNE);
	 	    }
		    
		    if (w2bn_agi_res!=load_success)
	     	    {
	 	      int games = account_get_ladder_active_wins(account,CLIENTTAG_WARCIIBNE,ladder_id_ironman)+
		 		  account_get_ladder_active_losses(account,CLIENTTAG_WARCIIBNE,ladder_id_ironman)+
	         	          account_get_ladder_active_disconnects(account,CLIENTTAG_WARCIIBNE,ladder_id_ironman);

	 	      war3_ladder_add(&W2BN_active_games_ironman, uid,rating, games,account,0,CLIENTTAG_WARCIIBNE);
	     	    }
		  }

	   }
    }
}


int standard_writer(FILE * fp, t_ladder * ladder,char const * clienttag)
{
  t_ladder_internal * pointer;
  unsigned int rank=0;

  pointer = ladder->first;
  while (pointer != NULL)
  {
     rank++;

     if (ladder==at_ladder(clienttag))
     {
       // if in the same team as previous account
       if ((pointer) && (pointer->next) 
           && in_same_team(pointer->account,pointer->next->account,pointer->teamcount,pointer->next->teamcount,clienttag))
	       rank--;
       else
	 // other team... so write all team members names, xp and rank to file
	 fprintf(fp,"%s,%u,%u\n",account_get_atteammembers(pointer->account,pointer->teamcount,clienttag),pointer->xp,rank);
     }
     else
     // write username, xp and rank to file
     fprintf(fp,"%s,%u,%u\n",account_get_name(pointer->account),pointer->xp,rank);

     pointer=pointer->prev;
  }
  return 0;
}

int XML_writer(FILE * fp, t_ladder * ladder, char const * clienttag)
  /* XML Ladder files
   * added by jfro 
   * 1/2/2003
   */
{
  t_ladder_internal * pointer;
  unsigned int rank=0;
  unsigned int level;
  unsigned int wins;
  unsigned int losses;
  unsigned int orc_wins,human_wins,undead_wins,nightelf_wins,random_wins;
  unsigned int orc_losses,human_losses,undead_losses,nightelf_losses,random_losses;
  char *members;
  char *member;

  fprintf(fp,"<?xml version=\"1.0\"?>\n<ladder>\n");
  pointer = ladder->first;
  while (pointer != NULL)
  {
     rank++;

     if (ladder==at_ladder(clienttag))
     {
       // if in the same team as previous account
       if ((pointer) && (pointer->next) 
           && in_same_team(pointer->account,pointer->next->account,pointer->teamcount,pointer->next->teamcount,clienttag))
	       rank--;
       else
	 {
	   // other team... so write all team members names, xp and rank to file
	   fprintf(fp,"\t<team>\n");
	   if (account_get_atteammembers(pointer->account,pointer->teamcount,clienttag)==NULL)
	     {
	       eventlog(eventlog_level_error,"XML_writer","got invalid team, skipping");
	       pointer=pointer->prev;
	       continue;
	     }
	   
	   if ((members = strdup(account_get_atteammembers(pointer->account,pointer->teamcount,clienttag))))
	     {
	       for ( member = strtok(members," ");
		     member;
		     member = strtok(NULL," "))
		 fprintf(fp,"\t\t<member>%s</member>\n",member);
	       free(members);
	       fprintf(fp,"\t\t<xp>%u</xp>\n\t\t<rank>%u</rank>\n\t</team>\n",pointer->xp,rank);
	     }
	 }
     }
     else {
         if (ladder==solo_ladder(clienttag)) {
           level = account_get_sololevel(pointer->account,clienttag);
           wins = account_get_solowin(pointer->account,clienttag);
           losses = account_get_sololoss(pointer->account,clienttag);
         }
         else if (ladder==team_ladder(clienttag)) {
           level = account_get_teamlevel(pointer->account,clienttag);
           wins = account_get_teamwin(pointer->account,clienttag);
           losses = account_get_teamloss(pointer->account,clienttag);
         }
         else if (ladder==ffa_ladder(clienttag)) {
           level = account_get_ffalevel(pointer->account,clienttag);
           wins = account_get_ffawin(pointer->account,clienttag);
           losses = account_get_ffaloss(pointer->account,clienttag);
         }
         else {
           level = 0;
           wins = 0;
           losses = 0;
         }
         orc_wins = account_get_racewin(pointer->account,W3_RACE_ORCS,clienttag);
         orc_losses = account_get_raceloss(pointer->account,W3_RACE_ORCS,clienttag);
         undead_wins = account_get_racewin(pointer->account,W3_RACE_UNDEAD,clienttag);
         undead_losses = account_get_raceloss(pointer->account,W3_RACE_UNDEAD,clienttag);
         human_wins = account_get_racewin(pointer->account,W3_RACE_HUMANS,clienttag);
         human_losses = account_get_raceloss(pointer->account,W3_RACE_HUMANS,clienttag);
         nightelf_wins = account_get_racewin(pointer->account,W3_RACE_NIGHTELVES,clienttag);
         nightelf_losses = account_get_raceloss(pointer->account,W3_RACE_NIGHTELVES,clienttag);
         random_wins = account_get_racewin(pointer->account,W3_RACE_RANDOM,clienttag);
         random_losses = account_get_raceloss(pointer->account,W3_RACE_RANDOM,clienttag);
     // write username, xp and rank to file and everyhing else needed for nice ladder pages
	 fprintf(fp,"\t<player>\n\t\t<name>%s</name>\n\t\t<level>%u</level>\n\t\t<xp>%u</xp>\n",
                    account_get_name(pointer->account),level,pointer->xp);
	 fprintf(fp,"\t\t<wins>%u</wins>\n\t\t<losses>%u</losses>\n\t\t<rank>%u</rank>\n",
                    wins,losses,rank);
	 fprintf(fp,"\t\t<races>\n\t\t\t<orc>\n\t\t\t\t<wins>%u</wins>\n\t\t\t\t<losses>%u</losses>\n\t\t\t</orc>\n",
                    orc_wins,orc_losses);
	 fprintf(fp,"\t\t\t<human>\n\t\t\t\t<wins>%u</wins>\n\t\t\t\t<losses>%u</losses>\n\t\t\t</human>\n",
                    human_wins,human_losses);
	 fprintf(fp,"\t\t\t<nightelf>\n\t\t\t\t<wins>%u</wins>\n\t\t\t\t<losses>%u</losses>\n\t\t\t</nightelf>\n",
		    nightelf_wins,nightelf_losses);
	 fprintf(fp,"\t\t\t<undead>\n\t\t\t\t<wins>%u</wins>\n\t\t\t\t<losses>%u</losses>\n\t\t\t</undead>\n",
		    undead_wins,undead_losses);
	 fprintf(fp,"\t\t\t<random>\n\t\t\t\t<wins>%u</wins>\n\t\t\t\t<losses>%u</losses>\n\t\t\t</random>\n\t\t</races>\n\t</player>\n",
                    random_wins,random_losses);
     }
     pointer=pointer->prev;
  }
  fprintf(fp,"</ladder>\n");
  return 0;
}


extern int ladder_write_to_file(char const * filename, t_ladder * ladder, char const * clienttag)
{
  FILE * fp;
  
  if (!filename)
  {
    eventlog(eventlog_level_error,"ladder_write_to_file","got NULL filename");
    return -1;
  }
  
  if (!(fp = fopen(filename,"w")))
  { 
     eventlog(eventlog_level_error,"ladder_write_to_file","could not open file \"%s\" for writing (fopen: %s)",filename,strerror(errno)); 
     return -1;
  }

  if (prefs_get_XML_output_ladder())
    XML_writer(fp,ladder,clienttag);
  else
    standard_writer(fp,ladder,clienttag);

  fclose(fp);
  return 0;
}

extern int ladders_write_to_file()
{
  
  eventlog(eventlog_level_info,"ladders_write_to_file","flushing ladders to disc");
  ladder_write_to_file(WAR3_solo_filename, &WAR3_solo_ladder,CLIENTTAG_WARCRAFT3);
  ladder_write_to_file(WAR3_team_filename, &WAR3_team_ladder,CLIENTTAG_WARCRAFT3);
  ladder_write_to_file(WAR3_ffa_filename,  &WAR3_ffa_ladder, CLIENTTAG_WARCRAFT3);
  ladder_write_to_file(WAR3_at_filename,   &WAR3_at_ladder,  CLIENTTAG_WARCRAFT3);
  ladder_write_to_file(W3XP_solo_filename, &W3XP_solo_ladder,   CLIENTTAG_WAR3XP);
  ladder_write_to_file(W3XP_team_filename, &W3XP_team_ladder,   CLIENTTAG_WAR3XP);
  ladder_write_to_file(W3XP_ffa_filename,  &W3XP_ffa_ladder,    CLIENTTAG_WAR3XP);
  ladder_write_to_file(W3XP_at_filename,   &W3XP_at_ladder,     CLIENTTAG_WAR3XP);
  return 0;
}

extern char * create_filename(const char * path, const char * filename, const char * ending)
{
  char * result;
  if (!(result = malloc(strlen(path)+1+strlen(filename)+strlen(ending)+1)))
  {
    eventlog(eventlog_level_error,"create_filename","could not allocate name for filename %s%s",filename,ending);
    return NULL;
  }
  else
  {
    sprintf(result,"%s/%s%s",path,filename,ending);
    return result;
  }
}

static void dispose_filename(char * filename)
{
  if (filename) free(filename);
}

void create_filenames(void)
{
  if (prefs_get_XML_output_ladder())
  {
    WAR3_solo_filename = create_filename(prefs_get_ladderdir(),WAR3_solo_file,xml_end);
    WAR3_team_filename = create_filename(prefs_get_ladderdir(),WAR3_team_file,xml_end);
    WAR3_ffa_filename  = create_filename(prefs_get_ladderdir(),WAR3_ffa_file,xml_end);
    WAR3_at_filename   = create_filename(prefs_get_ladderdir(),WAR3_at_file,xml_end);
    W3XP_solo_filename = create_filename(prefs_get_ladderdir(),W3XP_solo_file,xml_end);
    W3XP_team_filename = create_filename(prefs_get_ladderdir(),W3XP_team_file,xml_end);
    W3XP_ffa_filename  = create_filename(prefs_get_ladderdir(),W3XP_ffa_file,xml_end);
    W3XP_at_filename   = create_filename(prefs_get_ladderdir(),W3XP_at_file,xml_end);
  }
  else
  {
    WAR3_solo_filename = create_filename(prefs_get_ladderdir(),WAR3_solo_file,std_end);
    WAR3_team_filename = create_filename(prefs_get_ladderdir(),WAR3_team_file,std_end);
    WAR3_ffa_filename  = create_filename(prefs_get_ladderdir(),WAR3_ffa_file,std_end);
    WAR3_at_filename   = create_filename(prefs_get_ladderdir(),WAR3_at_file,std_end);
    W3XP_solo_filename = create_filename(prefs_get_ladderdir(),W3XP_solo_file,std_end);
    W3XP_team_filename = create_filename(prefs_get_ladderdir(),W3XP_team_file,std_end);
    W3XP_ffa_filename  = create_filename(prefs_get_ladderdir(),W3XP_ffa_file,std_end);
    W3XP_at_filename   = create_filename(prefs_get_ladderdir(),W3XP_at_file,std_end);
  }
}

extern void ladders_init(void)
{
  eventlog(eventlog_level_info,"ladders_init","initializing war3 ladders");
  ladder_init(&WAR3_solo_ladder,    WAR3_SOLO, CLIENTTAG_WARCRAFT3,ladder_id_none);
  ladder_init(&WAR3_team_ladder,    WAR3_TEAM, CLIENTTAG_WARCRAFT3,ladder_id_none);
  ladder_init(&WAR3_ffa_ladder,     WAR3_FFA,  CLIENTTAG_WARCRAFT3,ladder_id_none);
  ladder_init(&WAR3_at_ladder,      WAR3_AT,   CLIENTTAG_WARCRAFT3,ladder_id_none);
  ladder_init(&W3XP_solo_ladder,    W3XP_SOLO, CLIENTTAG_WAR3XP,ladder_id_none);
  ladder_init(&W3XP_team_ladder,    W3XP_TEAM, CLIENTTAG_WAR3XP,ladder_id_none);
  ladder_init(&W3XP_ffa_ladder,     W3XP_FFA,  CLIENTTAG_WAR3XP,ladder_id_none);
  ladder_init(&W3XP_at_ladder,      W3XP_AT,   CLIENTTAG_WAR3XP,ladder_id_none);
  ladder_init(&STAR_active_rating,  STAR_AR,   CLIENTTAG_STARCRAFT,ladder_id_normal);
  ladder_init(&STAR_active_wins,    STAR_AW,   CLIENTTAG_STARCRAFT,ladder_id_normal);
  ladder_init(&STAR_active_games,   STAR_AG,   CLIENTTAG_STARCRAFT,ladder_id_normal);
  ladder_init(&STAR_current_rating, STAR_CR,   CLIENTTAG_STARCRAFT,ladder_id_normal);
  ladder_init(&STAR_current_wins,   STAR_CW,   CLIENTTAG_STARCRAFT,ladder_id_normal);
  ladder_init(&STAR_current_games,  STAR_CG,   CLIENTTAG_STARCRAFT,ladder_id_normal);
  ladder_init(&SEXP_active_rating,  SEXP_AR,   CLIENTTAG_BROODWARS,ladder_id_normal);
  ladder_init(&SEXP_active_wins,    SEXP_AW,   CLIENTTAG_BROODWARS,ladder_id_normal);
  ladder_init(&SEXP_active_games,   SEXP_AG,   CLIENTTAG_BROODWARS,ladder_id_normal);
  ladder_init(&SEXP_current_rating, SEXP_CR,   CLIENTTAG_BROODWARS,ladder_id_normal);
  ladder_init(&SEXP_current_wins,   SEXP_CW,   CLIENTTAG_BROODWARS,ladder_id_normal);
  ladder_init(&SEXP_current_games,  SEXP_CG,   CLIENTTAG_BROODWARS,ladder_id_normal);
  ladder_init(&W2BN_current_rating,         W2BN_CR,   CLIENTTAG_WARCIIBNE,ladder_id_normal);
  ladder_init(&W2BN_current_wins,           W2BN_CW,   CLIENTTAG_WARCIIBNE,ladder_id_normal);
  ladder_init(&W2BN_current_games,          W2BN_CG,   CLIENTTAG_WARCIIBNE,ladder_id_normal);
  ladder_init(&W2BN_current_rating_ironman, W2BN_CRI,  CLIENTTAG_WARCIIBNE,ladder_id_ironman);
  ladder_init(&W2BN_current_wins_ironman,   W2BN_CWI,  CLIENTTAG_WARCIIBNE,ladder_id_ironman);
  ladder_init(&W2BN_current_games_ironman,  W2BN_CGI,  CLIENTTAG_WARCIIBNE,ladder_id_ironman);
  ladder_init(&W2BN_active_rating,          W2BN_AR,   CLIENTTAG_WARCIIBNE,ladder_id_normal);
  ladder_init(&W2BN_active_wins,            W2BN_AW,   CLIENTTAG_WARCIIBNE,ladder_id_normal);
  ladder_init(&W2BN_active_games,           W2BN_AG,   CLIENTTAG_WARCIIBNE,ladder_id_normal);
  ladder_init(&W2BN_active_rating_ironman,  W2BN_ARI,  CLIENTTAG_WARCIIBNE,ladder_id_ironman);
  ladder_init(&W2BN_active_wins_ironman,    W2BN_AWI,  CLIENTTAG_WARCIIBNE,ladder_id_ironman);
  ladder_init(&W2BN_active_games_ironman,   W2BN_AGI,  CLIENTTAG_WARCIIBNE,ladder_id_ironman);

  create_filenames();
}

void dispose_filenames(void)
{
  dispose_filename(WAR3_solo_filename);
  dispose_filename(WAR3_team_filename);
  dispose_filename(WAR3_ffa_filename);
  dispose_filename(WAR3_at_filename);
  dispose_filename(W3XP_solo_filename);
  dispose_filename(W3XP_team_filename);
  dispose_filename(W3XP_ffa_filename);
  dispose_filename(W3XP_at_filename);
}

extern void ladders_destroy(void)
{
  eventlog(eventlog_level_info,"ladders_destroy","destroying war3 ladders");
  ladder_destroy(&WAR3_solo_ladder);
  ladder_destroy(&WAR3_team_ladder);
  ladder_destroy(&WAR3_ffa_ladder);
  ladder_destroy(&WAR3_at_ladder);
  ladder_destroy(&W3XP_solo_ladder);
  ladder_destroy(&W3XP_team_ladder);
  ladder_destroy(&W3XP_ffa_ladder);
  ladder_destroy(&W3XP_at_ladder);
  ladder_destroy(&STAR_active_rating);
  ladder_destroy(&STAR_active_wins);
  ladder_destroy(&STAR_active_games);
  ladder_destroy(&STAR_current_rating);
  ladder_destroy(&STAR_current_wins);
  ladder_destroy(&STAR_current_games);
  ladder_destroy(&SEXP_active_rating);
  ladder_destroy(&SEXP_active_wins);
  ladder_destroy(&SEXP_active_games);
  ladder_destroy(&SEXP_current_rating);
  ladder_destroy(&SEXP_current_wins);
  ladder_destroy(&SEXP_current_games);
  ladder_destroy(&W2BN_current_rating);
  ladder_destroy(&W2BN_current_wins);
  ladder_destroy(&W2BN_current_games);
  ladder_destroy(&W2BN_current_rating_ironman);
  ladder_destroy(&W2BN_current_wins_ironman);
  ladder_destroy(&W2BN_current_games_ironman);
  ladder_destroy(&W2BN_active_rating);
  ladder_destroy(&W2BN_active_wins);
  ladder_destroy(&W2BN_active_games);
  ladder_destroy(&W2BN_active_rating_ironman);
  ladder_destroy(&W2BN_active_wins_ironman);
  ladder_destroy(&W2BN_active_games_ironman);
  dispose_filenames();
}

extern void ladder_reload_conf(void)
{
  dispose_filenames();
  create_filenames();
}

int ladder_get_from_ladder(t_binary_ladder_types type, int rank,int * results)
{
  t_ladder * ladder;
  t_ladder_internal * internal;

  if (!(results))
  {
    eventlog(eventlog_level_error,__FUNCTION__,"got NULL results");
    return -1;
  }

  if (!(ladder = binary_ladder_types_to_w3_ladder(type)))
    return -1;

  if (!(internal = ladder_get_rank_internal(ladder,rank,ladder_get_clienttag(ladder))))
  {
    return -1;
  }
 
  results[0] = internal->uid;
  results[1] = internal->xp;
  results[2] = internal->level;
  results[3] = internal->teamcount;

  return 0;
}

int ladder_put_into_ladder(t_binary_ladder_types type, int * values)
{
  t_ladder * ladder;
  t_account * acct;

  if (!(values))
  {
    eventlog(eventlog_level_error,__FUNCTION__,"got NULL values");
    return -1;
  }

  if (!(ladder = binary_ladder_types_to_w3_ladder(type)))
    return -1;

  if ((acct = accountlist_find_account_by_uid(values[0])))
    war3_ladder_add(ladder,values[0],values[1],values[2],acct,values[3],ladder_get_clienttag(ladder));
  else
    eventlog(eventlog_level_error,__FUNCTION__,"no account with this UID - skip");

  return 0;
}

extern int ladder_make_active(t_ladder *current, t_ladder *active)
{
  t_ladder_id id;
  char const * clienttag;
  t_binary_ladder_types type;

  id = current->ladder_id;
  clienttag = current->clienttag;
  type = current->type;

  ladder_destroy(active);
  ladder_init(active,type,clienttag,id);

  active->first = current->first;
  active->last = current->last;
  active->dirty = 1;

  ladder_init(current,type,clienttag,id);  

  return 0;
}

extern int ladder_createxptable(const char *xplevelfile, const char *xpcalcfile)
{
   FILE *fd1, *fd2;
   char buffer[256];
   char *p;
   int len,i ,j;
   int level, startxp, neededxp, mingames, calctype;
   float lossfactor;
   int minlevel, leveldiff, higher_xpgained, higher_xplost, lower_xpgained, lower_xplost  = 10;
   
   if (xplevelfile == NULL || xpcalcfile == NULL) {
      eventlog(eventlog_level_error, "ladder_createxptable", "got NULL filename(s)");
      return -1;
   }
   
   /* first lets open files */
   if ((fd1 = fopen(xplevelfile, "rt")) == NULL) {
      eventlog(eventlog_level_error, "ladder_createxptable", "could not open XP level file : \"%s\"", xplevelfile);
      return -1;
   }
   
   if ((fd2 = fopen(xpcalcfile, "rt")) == NULL) {
      eventlog(eventlog_level_error, "ladder_createxptable", "could not open XP calc file : \"%s\"", xpcalcfile);
      fclose(fd1);
      return -1;
   }
   
   /* then lets allocate mem for all the arrays */
   if ((xpcalc = malloc(sizeof(t_xpcalc) * W3_XPCALC_TYPES)) == NULL) {
      eventlog(eventlog_level_error, "ladder_createxptable", "could not allocate for calc types");
      fclose(fd1); fclose(fd2);
      return -1;
   }
   
   memset(xpcalc, 0, sizeof(t_xpcalc) * W3_XPCALC_TYPES);

   for(i=0; i<W3_XPCALC_TYPES;i++) {
      if ((xpcalc[i].xpchart = malloc(sizeof(t_xpcalc_entry) * (W3_XPCALC_MAXLEVELDIFF +1 ))) == NULL) {
	 eventlog(eventlog_level_error, "ladder_createxptable", "could not allocate for XP charts");
	 ladder_destroyxptable();
	 fclose(fd2); fclose(fd1);
	 return -1;
      }
      memset(xpcalc[i].xpchart, 0, sizeof(t_xpcalc_entry) * (W3_XPCALC_MAXLEVELDIFF + 1));
   }
   
   if ((xplevels = malloc(sizeof(t_xplevel_entry) * W3_XPCALC_MAXLEVEL)) == NULL) {
      eventlog(eventlog_level_error, "ladder_createxptable", "coould not allocate for XP levels");
      ladder_destroyxptable();
      fclose(fd2); fclose(fd1);
      return -1;
   }
   
   memset(xplevels, 0, sizeof(t_xplevel_entry) * W3_XPCALC_MAXLEVEL);
   
   /* finally, lets read from the files */
   
   while(fgets(buffer, 256, fd1)) {
      len = strlen(buffer);
      if (len < 2) continue;
      if (buffer[len - 1] == '\n') buffer[len - 1] = '\0';
      
      /* support comments */
      for(p=buffer; *p && *p != '#'; p++);
      if (*p == '#') *p = '\0';
      
      if (sscanf(buffer, "%d %d %d %f %d", &level, &startxp, &neededxp, &lossfactor, &mingames) != 5) 
	continue;
      
      if (level < 1 || level > W3_XPCALC_MAXLEVEL) { /* invalid level */
	 eventlog(eventlog_level_error, "ladder_createxptable", "read INVALID player level : %d", level);
	 continue;
      }
      
      level--; /* the index in a C array starts from 0 */
      xplevels[level].startxp = startxp;
      xplevels[level].neededxp = neededxp;
      xplevels[level].lossfactor = lossfactor * 100; /* we store the loss factor as % */
      xplevels[level].mingames = mingames;
      eventlog(eventlog_level_trace, "ladder_createxptable", "inserting level XP info (level: %d, startxp: %d neededxp: %d lossfactor: %d mingames: %d)", level+1, xplevels[level].startxp, xplevels[level].neededxp, xplevels[level].lossfactor, xplevels[level].mingames);
   }
   fclose(fd1);
   
   while(fgets(buffer, 256, fd2)) {
      len = strlen(buffer);
      if (len < 2) continue;
      if (buffer[len - 1] == '\n') buffer[len - 1] = '\0';
      
      /* support comments */
      for(p=buffer; *p && *p != '#'; p++);
      if (*p == '#') *p = '\0';
      
      if (sscanf(buffer, " %d %d %d %d %d %d ", &minlevel, &leveldiff, &higher_xpgained, &higher_xplost, &lower_xpgained, &lower_xplost) != 6)
	continue;
      
      eventlog(eventlog_level_trace, "ladder_createxptable", "parsed 1 minlevel: %d leveldiff : %d", minlevel, leveldiff);
      if (minlevel != 50 && minlevel != 25 && minlevel != 0) {
	 eventlog(eventlog_level_error, "ladder_createxptable", "got invalid minim level : %d", minlevel);
	 continue;
      }
      
      if (leveldiff <0 || leveldiff > W3_XPCALC_MAXLEVELDIFF) {
	 eventlog(eventlog_level_error, "ladder_createxptable", "got invalid level diff : %d", leveldiff);
	 continue;
      }
            
      calctype = minlevel / 25;
      if (calctype>W3_XPCALC_TYPES) calctype=W3_XPCALC_TYPES-1;
      eventlog(eventlog_level_trace, "ladder_createxptable", "parsed 2 minlevel: %d ", calctype);
      xpcalc[calctype].xpchart[leveldiff].higher_winxp = higher_xpgained;
      xpcalc[calctype].xpchart[leveldiff].higher_lossxp = higher_xplost;
      xpcalc[calctype].xpchart[leveldiff].lower_winxp = lower_xpgained;
      xpcalc[calctype].xpchart[leveldiff].lower_lossxp = lower_xplost;
   }
   fclose(fd2);
   
   /* OK, now we need to test couse if the user forgot to put some values
    * lots of profiles could get screwed up
    */
   for(i=0; i<W3_XPCALC_TYPES; i++)
     for(j=0;j<=W3_XPCALC_MAXLEVELDIFF;j++)
       if (xpcalc[i].xpchart[j].higher_winxp == 0 || xpcalc[i].xpchart[j].higher_lossxp == 0 ||
           xpcalc[i].xpchart[j].lower_winxp  == 0 || xpcalc[i].xpchart[j].lower_lossxp  == 0) {
	  eventlog(eventlog_level_error, "ladder_createxptable", "i found 0 for a win/loss XP, please check your config file");
	  ladder_destroyxptable();
	  return -1;
       }
   
   for (i=0; i<W3_XPCALC_MAXLEVEL; i++)
     if ((i > 0 && xplevels[i].neededxp == 0) || xplevels[i].lossfactor == 0
	 || xplevels[i].neededxp > xplevels[i].startxp 
	 || (i > 0 && (xplevels[i].startxp <= xplevels[i-1].startxp || xplevels[i].neededxp < xplevels[i-1].neededxp))) {
	eventlog(eventlog_level_error, "ladder_createxptable", "i found 0 for a level XP, please check your config file (level: %d neededxp: %d lossfactor: %d)", i+1, xplevels[i].neededxp , xplevels[i].lossfactor);
	ladder_destroyxptable();
	return -1;
     }
   
   return 0;
}

extern void ladder_destroyxptable()
{
   int i;
   
   if (xpcalc != NULL) {
      for(i=0; i < W3_XPCALC_TYPES; i++)
	if (xpcalc[i].xpchart != NULL) free(xpcalc[i].xpchart);
      free(xpcalc);
      xpcalc = NULL;
   }
   
   if (xplevels != NULL) free(xplevels);
}

extern int ladder_war3_xpdiff(unsigned int winnerlevel, unsigned int looserlevel, int *winxpdiff, int *loosxpdiff)
{
   int diff, absdiff;

   diff = winnerlevel - looserlevel;
   absdiff = (diff < 0)?(-diff):diff;
   
   if (absdiff > W3_XPCALC_MAXLEVELDIFF) {
      eventlog(eventlog_level_error, "ladder_war3_xpdiff", "got invalid level difference : %d", absdiff);
      return -1;
   }
   
   if (winnerlevel > W3_XPCALC_MAXLEVEL || looserlevel > W3_XPCALC_MAXLEVEL || winnerlevel <1 || looserlevel<1) {
      eventlog(eventlog_level_error, "ladder_war3_xpdiff", "got invalid account levels (win: %d loss: %d)",winnerlevel, looserlevel);
      return -1;
   }
   
   if (winxpdiff == NULL || loosxpdiff == NULL) {
      eventlog(eventlog_level_error, "ladder_war3_xpdiff", "got NULL winxpdiff, loosxpdiff");
      return -1;
   }
   /* we return the xp difference for the winner and the looser
    * we compute that from the xp charts also applying the loss factor for
    * lower level profiles
    * FIXME: ?! loss factor doesnt keep the sum of xp won/lost constant
    * DON'T CARE, cause current win/loss values aren't symetrical any more
    */
   if (diff >= 0) {
      *winxpdiff = xpcalc[0].xpchart[absdiff].higher_winxp;
      *loosxpdiff = - (xpcalc[0].xpchart[absdiff].lower_lossxp * xplevels[looserlevel - 1].lossfactor) / 100;
   } else {
      *winxpdiff = xpcalc[0].xpchart[absdiff].lower_winxp;
      *loosxpdiff = - (xpcalc[0].xpchart[absdiff].higher_lossxp * xplevels[looserlevel - 1].lossfactor) / 100;
   }
   
   return 0;
}

extern int ladder_war3_updatelevel(unsigned int oldlevel, int xp)
{
   int i, mylevel;
   
   if (oldlevel < 1 || oldlevel > W3_XPCALC_MAXLEVEL) {
      eventlog(eventlog_level_error, "ladder_war3_updatelevel", "got invalid level: %d", oldlevel);
      return oldlevel;
   }
   
   if (xp <= 0) return 1;

   mylevel = oldlevel;
   
   for(i=mylevel ; i < W3_XPCALC_MAXLEVEL; i++)
     if (xplevels[i].startxp > xp) { mylevel = i; break;}

   for(i=mylevel - 1; i >0 ; i--)
     if (xplevels[i-1].startxp < xp) { mylevel = i+1; break; }

   return mylevel;
}

extern int ladder_war3_get_min_xp(unsigned int Level)
{
  if (Level < 1 || Level > W3_XPCALC_MAXLEVEL)
  {
	eventlog(eventlog_level_error,__FUNCTION__,"got invalid Level %d",Level);
	return -1;
  }
  return xplevels[Level-1].startxp;
}
