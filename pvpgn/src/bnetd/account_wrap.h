/*
 * Copyright (C) 1998,1999  Ross Combs (rocombs@cs.nmsu.edu)
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


/*****/
#ifndef JUST_NEED_TYPES
#ifndef INCLUDED_ACCOUNT_WRAP_PROTOS
#define INCLUDED_ACCOUNT_WRAP_PROTOS

#define JUST_NEED_TYPES
#include "account.h"
#include "connection.h"
#include "character.h"
#include "common/bnettime.h"
#include "ladder.h"
#include "game.h"
#undef JUST_NEED_TYPES

/* convenience functions */
#ifdef DEBUG_ACCOUNT
extern unsigned int account_get_numattr_real(t_account * account, char const * key, char const * fn, unsigned int ln);
#define account_get_numattr(A,K) account_get_numattr_real(A,K,__FILE__,__LINE__)
#else
extern unsigned int account_get_numattr(t_account * account, char const * key);
#endif
extern int account_set_numattr(t_account * account, char const * key, unsigned int val);

#ifdef DEBUG_ACCOUNT
extern int account_get_boolattr_real(t_account * account, char const * key, char const * fn, unsigned int ln);
#define account_get_boolattr(A,K) account_get_boolattr_real(A,K,__FILE__,__LINE__)
#else
extern int account_get_boolattr(t_account * account, char const * key);
#endif
extern int account_set_boolattr(t_account * account, char const * key, int val);

#ifdef DEBUG_ACCOUNT
extern int account_unget_name_real(char const * name, char const * fn, unsigned int ln);
# define account_unget_name(N) account_unget_name_real(N,__FILE__,__LINE__)
#else
extern int account_unget_name(char const * name);
#endif
extern char const * account_get_pass(t_account * account);
extern int account_unget_pass(char const * pass);
extern int account_set_pass(t_account * account, char const * passhash1);

/* authorization */
extern int account_get_auth_admin(t_account * account, char const * channelname);
extern int account_set_auth_admin(t_account * account, char const * channelname, int val);
extern int account_get_auth_announce(t_account * account);
extern int account_get_auth_botlogin(t_account * account);
extern int account_get_auth_bnetlogin(t_account * account);
extern int account_get_auth_operator(t_account * account, char const * channelname);
extern int account_set_auth_operator(t_account * account, char const * channelname, int val);
extern int account_get_auth_voice(t_account * account, char const * channelname);
extern int account_set_auth_voice(t_account * account, char const * channelname, int val);
extern int account_get_auth_changepass(t_account * account);
extern int account_get_auth_changeprofile(t_account * account);
extern int account_get_auth_createnormalgame(t_account * account);
extern int account_get_auth_joinnormalgame(t_account * account);
extern int account_get_auth_createladdergame(t_account * account);
extern int account_get_auth_joinladdergame(t_account * account);
extern int account_get_auth_lock(t_account * account);
extern int account_set_auth_lock(t_account * account, int val);

/* profile */
extern char const * account_get_sex(t_account * account); /* the profile attributes are updated directly in bnetd.c */
extern int account_unget_sex(char const * sex); /* the profile attributes are updated directly in bnetd.c */
extern char const * account_get_age(t_account * account);
extern int account_unget_age(char const * age);
extern char const * account_get_loc(t_account * account);
extern int account_unget_loc(char const * loc);
extern char const * account_get_desc(t_account * account);
extern int account_unget_desc(char const * desc);

/* first login */
extern unsigned int account_get_fl_time(t_account * account);
extern int account_set_fl_time(t_account * account, unsigned int t);
extern unsigned int account_get_fl_connection(t_account * account);
extern int account_set_fl_connection(t_account * account, unsigned int connection);
extern char const * account_get_fl_host(t_account * account);
extern int account_unget_fl_host(char const * clientexe);
extern int account_set_fl_host(t_account * account, char const * host);
extern char const * account_get_fl_user(t_account * account);
extern int account_unget_fl_user(char const * clientexe);
extern int account_set_fl_user(t_account * account, char const * user);
extern char const * account_get_fl_clientexe(t_account * account);
extern int account_unget_fl_clientexe(char const * clientexe);
extern int account_set_fl_clientexe(t_account * account, char const * exefile);
extern char const * account_get_fl_clienttag(t_account * account);
extern int account_unget_fl_clienttag(char const * clienttag);
extern int account_set_fl_clienttag(t_account * account, char const * clienttag);
extern char const * account_get_fl_clientver(t_account * account);
extern int account_unget_fl_clientver(char const * clientver);
extern int account_set_fl_clientver(t_account * account, char const * version);
extern char const * account_get_fl_owner(t_account * account);
extern int account_unget_fl_owner(char const * owner);
extern int account_set_fl_owner(t_account * account, char const * owner);
extern char const * account_get_fl_cdkey(t_account * account);
extern int account_unget_fl_cdkey(char const * cdkey);
extern int account_set_fl_cdkey(t_account * account, char const * cdkey);

/* last login */
extern unsigned int account_get_ll_time(t_account * account);
extern int account_set_ll_time(t_account * account, unsigned int t);
extern unsigned int account_get_ll_connection(t_account * account);
extern int account_set_ll_connection(t_account * account, unsigned int connection);
extern char const * account_get_ll_host(t_account * account);
extern int account_unget_ll_host(char const * clientexe);
extern int account_set_ll_host(t_account * account, char const * host);
extern char const * account_get_ll_user(t_account * account);
extern int account_unget_ll_user(char const * clientexe);
extern int account_set_ll_user(t_account * account, char const * user);
extern char const * account_get_ll_clientexe(t_account * account);
extern int account_unget_ll_clientexe(char const * clientexe);
extern int account_set_ll_clientexe(t_account * account, char const * exefile);
extern char const * account_get_ll_clienttag(t_account * account);
extern int account_unget_ll_clienttag(char const * clienttag);
extern int account_set_ll_clienttag(t_account * account, char const * clienttag);
extern char const * account_get_ll_clientver(t_account * account);
extern int account_unget_ll_clientver(char const * clientver);
extern int account_set_ll_clientver(t_account * account, char const * version);
extern char const * account_get_ll_owner(t_account * account);
extern int account_unget_ll_owner(char const * owner);
extern int account_set_ll_owner(t_account * account, char const * owner);
extern char const * account_get_ll_cdkey(t_account * account);
extern int account_unget_ll_cdkey(char const * cdkey);
extern int account_set_ll_cdkey(t_account * account, char const * cdkey);

extern int account_remove_verbose_accounting(t_account * account);

/* normal stats */
extern unsigned int account_get_normal_wins(t_account * account, char const * clienttag);
extern int account_inc_normal_wins(t_account * account, char const * clienttag);
extern unsigned int account_get_normal_losses(t_account * account, char const * clienttag);
extern int account_inc_normal_losses(t_account * account, char const * clienttag);
extern unsigned int account_get_normal_draws(t_account * account, char const * clienttag);
extern int account_inc_normal_draws(t_account * account, char const * clienttag);
extern unsigned int account_get_normal_disconnects(t_account * account, char const * clienttag);
extern int account_inc_normal_disconnects(t_account * account, char const * clienttag);
extern int account_set_normal_last_time(t_account * account, char const * clienttag, t_bnettime t);
extern int account_set_normal_last_result(t_account * account, char const * clienttag, char const * result);

/* ladder stats (active) */
extern unsigned int account_get_ladder_active_wins(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_set_ladder_active_wins(t_account * account, char const * clienttag, t_ladder_id id, unsigned int wins);
extern unsigned int account_get_ladder_active_losses(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_set_ladder_active_losses(t_account * account, char const * clienttag, t_ladder_id id, unsigned int losses);
extern unsigned int account_get_ladder_active_draws(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_set_ladder_active_draws(t_account * account, char const * clienttag, t_ladder_id id, unsigned int draws);
extern unsigned int account_get_ladder_active_disconnects(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_set_ladder_active_disconnects(t_account * account, char const * clienttag, t_ladder_id id, unsigned int disconnects);
extern unsigned int account_get_ladder_active_rating(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_set_ladder_active_rating(t_account * account, char const * clienttag, t_ladder_id id, unsigned int rating);
extern unsigned int account_get_ladder_active_rank(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_set_ladder_active_rank(t_account * account, char const * clienttag, t_ladder_id id, unsigned int rank);
extern char const * account_get_ladder_active_last_time(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_set_ladder_active_last_time(t_account * account, char const * clienttag, t_ladder_id id, t_bnettime t);

/* ladder stats (current) */
extern unsigned int account_get_ladder_wins(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_inc_ladder_wins(t_account * account, char const * clienttag, t_ladder_id id);
extern unsigned int account_get_ladder_losses(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_inc_ladder_draws(t_account * account, char const * clienttag, t_ladder_id id);
extern unsigned int account_get_ladder_draws(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_inc_ladder_losses(t_account * account, char const * clienttag, t_ladder_id id);
extern unsigned int account_get_ladder_disconnects(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_inc_ladder_disconnects(t_account * account, char const * clienttag, t_ladder_id id);
extern unsigned int account_get_ladder_rating(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_adjust_ladder_rating(t_account * account, char const * clienttag, t_ladder_id id, int delta);
extern unsigned int account_get_ladder_rank(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_set_ladder_rank(t_account * account, char const * clienttag, t_ladder_id id, unsigned int rank);
extern unsigned int account_get_ladder_high_rating(t_account * account, char const * clienttag, t_ladder_id id);
extern unsigned int account_get_ladder_high_rank(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_set_ladder_last_time(t_account * account, char const * clienttag, t_ladder_id id, t_bnettime t);
extern char const * account_get_ladder_last_time(t_account * account, char const * clienttag, t_ladder_id id);
extern int account_set_ladder_last_result(t_account * account, char const * clienttag, t_ladder_id id, char const * result);

/* Diablo normal stats */
extern unsigned int account_get_normal_level(t_account * account, char const * clienttag);
extern int account_set_normal_level(t_account * account, char const * clienttag, unsigned int level);
extern unsigned int account_get_normal_class(t_account * account, char const * clienttag);
extern int account_set_normal_class(t_account * account, char const * clienttag, unsigned int class);
extern unsigned int account_get_normal_diablo_kills(t_account * account, char const * clienttag);
extern int account_set_normal_diablo_kills(t_account * account, char const * clienttag, unsigned int diablo_kills);
extern unsigned int account_get_normal_strength(t_account * account, char const * clienttag);
extern int account_set_normal_strength(t_account * account, char const * clienttag, unsigned int strength);
extern unsigned int account_get_normal_magic(t_account * account, char const * clienttag);
extern int account_set_normal_magic(t_account * account, char const * clienttag, unsigned int magic);
extern unsigned int account_get_normal_dexterity(t_account * account, char const * clienttag);
extern int account_set_normal_dexterity(t_account * account, char const * clienttag, unsigned int dexterity);
extern unsigned int account_get_normal_vitality(t_account * account, char const * clienttag);
extern int account_set_normal_vitality(t_account * account, char const * clienttag, unsigned int vitality);
extern unsigned int account_get_normal_gold(t_account * account, char const * clienttag);
extern int account_set_normal_gold(t_account * account, char const * clienttag, unsigned int gold);

/* Diablo II closed characters */
extern char const * account_get_closed_characterlist(t_account * account, char const * clienttag, char const * realmname);
extern int account_unget_closed_characterlist(t_account * account, char const * charlist);
extern int account_set_closed_characterlist(t_account * account, char const * clienttag, char const * charlist);
extern int account_add_closed_character(t_account * account, char const * clienttag, t_character * ch);
extern int account_check_closed_character(t_account * account, char const * clienttag, char const * realmname, char const * charname);

/* UNDYING CLAN NAME ADDITION */
extern char const * account_get_w3_clanname( t_account * account);
extern int account_set_w3_clanname( t_account * account, char const * acctsetclanname );

// UNDYING FRIENDS LIST SAVE/GET 5/15/02
// [zap-zero] 20020516

extern int account_set_friend( t_account * account, int friendnum, unsigned int frienduid );
extern unsigned int account_get_friend( t_account * account, int friendnum);
extern int account_set_friendcount( t_account * account, int count);
extern int account_get_friendcount( t_account * account );
extern int account_add_friend( t_account * my_acc, t_account * facc );
extern int account_remove_friend( t_account * account, int friendnum );
extern int account_remove_friend2( t_account * account, const char * friend );

extern char const * race_get_str(unsigned int race);
extern int account_set_admin( t_account * account );
extern int account_set_demoteadmin( t_account * account );

extern unsigned int account_get_command_groups(t_account * account);
extern int account_set_command_groups(t_account * account, unsigned int groups);

extern int account_set_w3pgrace( t_account * account, char const * clienttag, unsigned int race);
extern int account_get_w3pgrace( t_account * account, char const * clienttag );

extern void account_get_raceicon(t_account * account, char * raceicon, unsigned int * raceiconnumber, unsigned int * wins,char const * clienttag);
//Used to call the save stats for all opponents
extern int account_set_saveladderstats(t_account *a, unsigned int gametype, t_game_result result, unsigned int opponlevel,char const * clienttag);

extern int account_set_racewin( t_account * account, unsigned int intrace, char const * clienttag);
extern int account_get_racewin( t_account * account, unsigned int intrace, char const * clienttag);
extern int account_set_raceloss( t_account * account, unsigned int intrace, char const * clienttag);
extern int account_get_raceloss( t_account * account, unsigned int intrace, char const * clienttag);
// 1v1 Solo Play Game funcs
extern int account_set_solowin( t_account * account, char const * clienttag);
extern int account_get_solowin( t_account * account, char const * clienttag);
extern int account_set_sololoss( t_account * account, char const * clienttag );
extern int account_get_sololoss( t_account * account, char const * clienttag );
extern int account_set_soloxp(t_account * account, char const * clienttag, t_game_result gameresult, unsigned int opponlevel,int * xp_diff);
extern int account_get_soloxp(t_account * account, char const * clienttag);
extern int account_set_sololevel(t_account * account, char const * clienttag);
extern int account_get_sololevel(t_account * account, char const * clienttag);
extern int account_set_solorank(t_account * account, char const * clienttag);
extern int account_get_solorank(t_account * account, char const * clienttag);
// aaron - 2 new funcs for optimized access used in ladder
extern int account_set_solorank_ladder(t_account * account, char const * clienttag, int rank);
extern int account_get_solorank_ladder(t_account * account, char const * clienttag);
// Any Team Game great then 1v1 .. so thats (2v2,3v3,4v4)
extern int account_set_teamwin(t_account * account, char const * clienttag);
extern int account_get_teamwin(t_account * account, char const * clienttag);
extern int account_set_teamloss(t_account * account, char const * clienttag);
extern int account_get_teamloss(t_account * account, char const * clienttag);
extern int account_set_teamxp(t_account * account, char const * clienttag, t_game_result gameresult, unsigned int opponlevel,int * xp_diff);
extern int account_get_teamxp(t_account * account, char const * clienttag);
extern int account_set_teamlevel(t_account * account, char const * clienttag);
extern int account_get_teamlevel(t_account * account, char const * clienttag);
extern int account_set_teamrank(t_account * account, char const * clienttag);
extern int account_get_teamrank(t_account * account, char const * clienttag);
// aaron - another 2 funcs for ladder optimized access
extern int account_set_teamrank_ladder(t_account * account, char const * clienttag, int rank);
extern int account_get_teamrank_ladder(t_account * account, char const * clienttag);
// Funcs for small and big FFA
extern int account_set_ffawin(t_account * account, char const * clienttag);
extern int account_get_ffawin(t_account * account, char const * clienttag);
extern int account_set_ffaloss(t_account * account, char const * clienttag);
extern int account_get_ffaloss(t_account * account, char const * clienttag);
extern int account_set_ffaxp(t_account * account, char const * clienttag, t_game_result gameresult, unsigned int opponlevel, int * xp_diff);
extern int account_get_ffaxp(t_account * account, char const * clienttag);
extern int account_set_ffalevel(t_account * account, char const * clienttag);
extern int account_get_ffalevel(t_account * account, char const * clienttag);
extern int account_set_ffalevel(t_account * account, char const * clienttag);
extern int account_get_ffarank(t_account * account, char const * clienttag);
// aaron - those 2 functions again for this ladder
extern int account_set_ffarank_ladder(t_account * account, char const * clienttag, int rank);
extern int account_get_ffarank_ladder(t_account * account, char const * clienttag);
// Funcs for Arranged Teams - THEUNDYING
extern int account_set_atteamcount(t_account * account, char const * clienttag, unsigned int teamcount);
extern int account_get_atteamcount(t_account * account, char const * clienttag);
extern int account_set_atteamsize(t_account * account, unsigned int teamcount, char const * clienttag, unsigned int teamsize);
extern int account_get_atteamsize(t_account * account, unsigned int teamcount, char const * clienttag);
extern int account_set_atteamwin(t_account * account, unsigned int teamcount, char const * clienttag, int wins);
extern int account_atteamwin(t_account * account, unsigned int teamcount, char const * clienttag);
extern int account_get_atteamwin(t_account * account, unsigned int teamcount, char const * clienttag);
extern int account_set_atteamloss(t_account * account, unsigned int teamcount, char const * clienttag, int loss);
extern int account_atteamloss(t_account * account, unsigned int teamcount, char const * clienttag);
extern int account_get_atteamloss(t_account * account, unsigned int teamcount, char const * clienttag);
extern int account_set_atteamxp(t_account * account, unsigned int teamcount, char const * clienttag, int xp);
extern int account_update_atteamxp(t_account * account, t_game_result gameresult, unsigned int opponlevel, unsigned int teamcount, char const * clienttag, int * xp_diff);
extern int account_get_atteamxp(t_account * account, unsigned int teamcount, char const * clienttag);
extern int account_set_atteamlevel(t_account * account, unsigned int teamcount, char const * clienttag, int teamlevel);
extern int account_update_atteamlevel(t_account * account, unsigned int teamcount, char const * clienttag);
extern int account_get_atteamlevel(t_account * account, unsigned int teamcount, char const * clienttag);
//aaron: --->
extern int account_set_atteamrank(t_account * account, unsigned int teamcount, char const * clienttag, int teamrank);
extern int account_update_atteamrank(t_account * account, unsigned int teamcount, char const * clienttag);
extern int account_get_atteamrank(t_account * account, unsigned int teamcount, char const * clienttag);
extern int account_set_atteamrank_ladder(t_account * account, int rank, unsigned int teamcount, char const * clienttag);
extern int account_get_atteamrank_ladder(t_account * account, unsigned int teamcount, char const * clienttag);
// <---
extern int account_set_atteammembers(t_account * account, unsigned int teamcount, char const * clienttag, char const *members);
extern char const * account_get_atteammembers(t_account * account, unsigned int teamcount, char const * clienttag);
extern int account_fix_at(t_account * account, const char * ctag);

extern int account_set_saveATladderstats(t_account * account, unsigned int gametype, t_game_result result, unsigned int opponlevel, unsigned int current_teamnum, char const * clienttag);

extern int account_check_team(t_account * account, const char * members_usernames, char const * clienttag);
extern int account_create_newteam(t_account * account, const char * members_usernames, unsigned int teamsize, char const * clienttag);

extern int account_set_currentatteam(t_account * account, unsigned int teamcount);
extern int account_get_currentatteam(t_account * account);
extern int account_set_new_at_team(t_account * account, unsigned int value);
extern int account_get_new_at_team(t_account * account);

extern int account_get_highestladderlevel(t_account * account,char const * clienttag); //func will compare levels for AT, Solo/Team Ladder and out the higest level of the 3

// Determines the length of XP bar in profiles screen
extern int account_get_profile_calcs(t_account * account, int xp, unsigned int level);
extern unsigned int account_get_icon_profile(t_account * account,char const * clienttag);

extern int account_set_user_icon( t_account * account, char const * clienttag,char const * usericon);
extern char const * account_get_user_icon( t_account * account, char const * clienttag );
extern unsigned int account_icon_to_profile_icon(char const * icon,t_account * account, char const * ctag);
extern char const * account_icon_default(t_account * account, char const * clienttag);

extern int account_is_operator_or_admin(t_account * account, char const * channel);

extern int account_set_email(t_account * account, char const * email);
extern char const * account_get_email(t_account * account);

#endif
#endif
