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
#define CLAN_INTERNAL_ACCESS
#include "common/setup_before.h"
#include <stdio.h>
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
#include "compat/strrchr.h"
#include "compat/strdup.h"
#include "compat/strcasecmp.h"
#include "compat/strncasecmp.h"
#include "compat/pdir.h"
#include <errno.h>
#include "compat/strerror.h"
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif
#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif
#include "common/eventlog.h"
#include "common/packet.h"
#include "common/bnet_protocol.h"
#include "common/w3xp_protocol.h"
#include "common/tag.h"
#include "common/util.h"
#include "common/bnettime.h"
#include "common/eventlog.h"
#include "common/list.h"
#include "common/proginfo.h"
#include "common/bn_type.h"
#include "connection.h"
#include "anongame.h"
#include "prefs.h"
#include "friends.h"
#include "game.h"
#include "clan.h"
#include "account.h"
#include "channel.h"
#include "anongame.h"
#include "storage.h"
#include "common/setup_after.h"

static t_list *clanlist_head = NULL;
int max_clanid = 0;

/* callback function for storage use */

static int _cb_load_clans(void *clan)
{
    if (clanlist_add_clan(clan) < 0)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "failed to add clan to clanlist");
	return -1;
    }
    if (((t_clan *) clan)->clanid > max_clanid)
	max_clanid = ((t_clan *) clan)->clanid;
    return 0;
}

/*
** Packet Management
*/

extern int clan_send_packet_to_online_members(t_clan * clan, t_packet * packet)
{
    t_elem *curr;

    if (!clan)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return -1;
    }

    if (!packet)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL packet");
	return -1;
    }

    LIST_TRAVERSE(clan->members, curr)
    {
	t_clanmember *member;
	char const *clienttag;
	if ((member = elem_get_data(curr)) && (member->memberconn) && (clienttag = conn_get_clienttag(member->memberconn)) && ((strcasecmp(clienttag, CLIENTTAG_WARCRAFT3) == 0) || (strcasecmp(clienttag, CLIENTTAG_WAR3XP) == 0)))
	    conn_push_outqueue(member->memberconn, packet);
    }

    return 0;
}

extern int clan_send_status_window_on_create(t_clan * clan)
{
    t_packet *rpacket = NULL;
    t_elem *curr;

    if (clan == NULL)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return -1;
    }

    if ((rpacket = packet_create(packet_class_bnet)) != NULL)
    {
	char channelname[10];
	if (clan->clanshort)
	    sprintf(channelname, "Clan %c%c%c%c", (clan->clanshort >> 24), (clan->clanshort >> 16) & 0xff, (clan->clanshort >> 8) & 0xff, clan->clanshort & 0xff);
	packet_set_size(rpacket, sizeof(t_server_w3xp_clan_clanack));
	packet_set_type(rpacket, SERVER_W3XP_CLAN_CLANACK);
	bn_byte_set(&rpacket->u.server_w3xp_clan_clanack.unknow1, 0);
	bn_int_set(&rpacket->u.server_w3xp_clan_clanack.clanshort, clan->clanshort);
	LIST_TRAVERSE(clan->members, curr)
	{
	    t_clanmember *member;
	    char const *clienttag;
	    if ((member = elem_get_data(curr)) && member->memberconn && (clienttag = conn_get_clienttag(member->memberconn)) && ((strcasecmp(clienttag, CLIENTTAG_WARCRAFT3) == 0) || (strcasecmp(clienttag, CLIENTTAG_WAR3XP) == 0)))
	    {
		if (conn_get_channel(member->memberconn))
		{
		    conn_update_w3_playerinfo(member->memberconn);
		    channel_set_flags(member->memberconn);
		    if (conn_set_channel(member->memberconn, channelname) < 0)
			conn_set_channel(member->memberconn, CHANNEL_NAME_BANNED);	/* should not fail */
		}
		bn_byte_set(&rpacket->u.server_w3xp_clan_clanack.status, member->status);
		conn_push_outqueue(member->memberconn, rpacket);
	    }
	}
	packet_del_ref(rpacket);
    }
    return 0;
}

extern int clan_close_status_window_on_disband(t_clan * clan)
{
    t_packet *rpacket = NULL;
    t_elem *curr;

    if (clan == NULL)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return -1;
    }

    if ((rpacket = packet_create(packet_class_bnet)) != NULL)
    {
	packet_set_size(rpacket, sizeof(t_server_w3xp_clan_clanleaveack));
	packet_set_type(rpacket, SERVER_W3XP_CLAN_CLANLEAVEACK);
	bn_byte_set(&rpacket->u.server_w3xp_clan_clanack.unknow1, SERVER_W3XP_CLAN_CLANLEAVEACK_UNKNOWN1);
	LIST_TRAVERSE(clan->members, curr)
	{
	    t_clanmember *member;
	    char const *clienttag;
	    if ((member = elem_get_data(curr)) && member->memberconn && (clienttag = conn_get_clienttag(member->memberconn)) && ((strcasecmp(clienttag, CLIENTTAG_WARCRAFT3) == 0) || (strcasecmp(clienttag, CLIENTTAG_WAR3XP) == 0)))
	    {
		conn_push_outqueue(member->memberconn, rpacket);
		conn_update_w3_playerinfo(member->memberconn);
	    }
	}
	packet_del_ref(rpacket);
    }


    return 0;
}

extern int clan_send_status_window(t_connection * c)
{
    t_packet *rpacket = NULL;
    t_account *acc = conn_get_account(c);
    t_clanmember *member;
    char const *clienttag;

    if (acc == NULL)
	return 0;

    if ((member = account_get_clanmember(acc)) && member->clan && (member->clan->clanshort) && (clienttag = conn_get_clienttag(c)) && ((strcasecmp(clienttag, CLIENTTAG_WARCRAFT3) == 0) || (strcasecmp(clienttag, CLIENTTAG_WAR3XP) == 0)))
    {
	if ((rpacket = packet_create(packet_class_bnet)) != NULL)
	{
	    packet_set_size(rpacket, sizeof(t_server_w3xp_clan_clanack));
	    packet_set_type(rpacket, SERVER_W3XP_CLAN_CLANACK);
	    bn_byte_set(&rpacket->u.server_w3xp_clan_clanack.unknow1, 0);
	    bn_int_set(&rpacket->u.server_w3xp_clan_clanack.clanshort, member->clan->clanshort);
	    bn_byte_set(&rpacket->u.server_w3xp_clan_clanack.status, member->status);
	    conn_push_outqueue(c, rpacket);
	    packet_del_ref(rpacket);
	}
    }
    return 0;
}

extern int clan_close_status_window(t_connection * c)
{
    t_packet *rpacket = NULL;
    char const *clienttag;

    if ((clienttag = conn_get_clienttag(c)) && ((strcasecmp(clienttag, CLIENTTAG_WARCRAFT3) == 0) || (strcasecmp(clienttag, CLIENTTAG_WAR3XP) == 0)) && ((rpacket = packet_create(packet_class_bnet)) != NULL))
    {
	packet_set_size(rpacket, sizeof(t_server_w3xp_clan_clanleaveack));
	packet_set_type(rpacket, SERVER_W3XP_CLAN_CLANLEAVEACK);
	bn_byte_set(&rpacket->u.server_w3xp_clan_clanack.unknow1, SERVER_W3XP_CLAN_CLANLEAVEACK_UNKNOWN1);
	conn_push_outqueue(c, rpacket);
	packet_del_ref(rpacket);
    }
    return 0;
}

extern int clan_send_member_status(t_connection * c, t_packet const *const packet)
{
    t_packet *rpacket = NULL;
    t_elem *curr;
    char const *username;
    t_clanmember *member;
    t_clan *clan;
    t_account *account;
    int count = 0;
    char tmpstr[2];
    const char *append_str = NULL;

    if ((account = conn_get_account(c)) == NULL)
	return -1;

    if ((clan = account_get_clan(account)) == NULL)
	return -1;

    if ((rpacket = packet_create(packet_class_bnet)))
    {
	packet_set_size(rpacket, sizeof(t_server_w3xp_clan_memberreply));
	packet_set_type(rpacket, SERVER_W3XP_CLAN_MEMBERREPLY);
	bn_int_set(&rpacket->u.server_w3xp_clan_memberreply.count, bn_int_get(packet->u.client_w3xp_clan_memberreq.count));
	LIST_TRAVERSE(clan->members, curr)
	{
	    if (!(member = elem_get_data(curr)))
	    {
		eventlog(eventlog_level_error, __FUNCTION__, "got NULL element in list");
		return -1;
	    }
	    username = account_get_name(member->memberacc);
	    packet_append_string(rpacket, username);
	    tmpstr[0] = member->status;
	    append_str = clanmember_get_online_status(member, &tmpstr[1]);
	    packet_append_data(rpacket, tmpstr, 2);
	    if (append_str)
		packet_append_string(rpacket, append_str);
	    else
		packet_append_string(rpacket, "");
	    count++;
	}
	bn_byte_set(&rpacket->u.server_w3xp_clan_memberreply.member_count, count);
	conn_push_outqueue(c, rpacket);
	packet_del_ref(rpacket);
	return 0;
    }

    return -1;
}

extern int clan_save_motd_chg(t_connection * c, t_packet const *const packet)
{
    t_account *account;
    char const *motd;
    int offset;
    t_clan *clan;

    if ((account = conn_get_account(c)) == NULL)
	return -1;
    if ((clan = account_get_clan(account)) == NULL)
	return -1;
    offset = sizeof(packet->u.client_w3xp_clan_motdchg);
    motd = packet_get_str_const(packet, offset, 25);
    eventlog(eventlog_level_trace, __FUNCTION__, "[%d] got W3XP_CLAN_MOTDCHG packet : %s", conn_get_socket(c), motd);
    if (clan_set_motd(clan, motd) != 0)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "Failed to set clan motd.");
	return -1;
    }
    clan->modified = 1;
    return 0;
}


extern int clan_send_motd_reply(t_connection * c, t_packet const *const packet)
{
    t_packet *rpacket = NULL;
    t_account *account;
    t_clan *clan;

    if ((account = conn_get_account(c)) == NULL)
	return -1;
    if ((clan = account_get_clan(account)) == NULL)
	return -1;
    if (clan->clan_motd == NULL)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "Failed to get clan motd.");
	return -1;
    }
    if ((rpacket = packet_create(packet_class_bnet)))
    {
	packet_set_size(rpacket, sizeof(t_server_w3xp_clan_motdreply));
	packet_set_type(rpacket, SERVER_W3XP_CLAN_MOTDREPLY);
	bn_int_set(&rpacket->u.server_w3xp_clan_motdreply.count, bn_int_get(packet->u.client_w3xp_clan_motdreq.count));
	bn_int_set(&rpacket->u.server_w3xp_clan_motdreply.unknow1, SERVER_W3XP_CLAN_MOTDREPLY_UNKNOW1);
	packet_append_string(rpacket, clan->clan_motd);
	conn_push_outqueue(c, rpacket);
	packet_del_ref(rpacket);
    }

    return 0;
}

/*
** String / Function Management
*/

extern int clan_get_possible_member(t_connection * c, t_packet const *const packet)
{
    t_packet *rpacket = NULL;
    t_channel *channel;
    t_connection *conn;
    char const *username;

    int friend_count = 0;
    int clanshort;
    clanshort = bn_int_get(packet->u.client_w3xp_clan_createreq.clanshort);
    if ((rpacket = packet_create(packet_class_bnet)) == NULL)
    {
	return -1;
    }
    packet_set_size(rpacket, sizeof(t_server_w3xp_clan_createreply));
    packet_set_type(rpacket, SERVER_W3XP_CLAN_CREATEREPLY);
    bn_int_set(&rpacket->u.server_w3xp_clan_createreply.count, bn_int_get(packet->u.client_w3xp_clan_createreq.count));
    if (clanlist_find_clan_by_clanshort(clanshort) != NULL)
    {
	bn_byte_set(&rpacket->u.server_w3xp_clan_createreply.check_clanshort, SERVER_W3XP_CLAN_CREATEREPLY_CHECK_ALLREADY_IN_USE);
	bn_byte_set(&rpacket->u.server_w3xp_clan_createreply.friend_count, 0);
	conn_push_outqueue(c, rpacket);
	packet_del_ref(rpacket);
	return 0;
    }
    bn_byte_set(&rpacket->u.server_w3xp_clan_createreply.check_clanshort, SERVER_W3XP_CLAN_CREATEREPLY_CHECK_OK);
    channel = conn_get_channel(c);
    if (channel_get_permanent(channel))
    {
	/* If not in a private channel, retreive number of mutual friend connected */
	t_list *flist = account_get_friends(conn_get_account(c));
	t_elem const *curr;
	t_friend *fr;

	LIST_TRAVERSE_CONST(flist, curr)
	{
	    if ((fr = elem_get_data(curr)) != NULL)
	    {
		t_account *fr_acc = friend_get_account(fr);
		char const *clienttag;
		if (fr->mutual && ((conn = connlist_find_connection_by_account(fr_acc)) != NULL) && (conn_get_channel(conn) == channel) && (!account_get_clan(fr_acc)) && (!account_get_creating_clan(fr_acc)) && (clienttag = conn_get_clienttag(conn)) && ((strcasecmp(clienttag, CLIENTTAG_WAR3XP) == 0) || (strcasecmp(clienttag, CLIENTTAG_WARCRAFT3) == 0)) && (username = account_get_name(fr_acc)))
		{
		    friend_count++;
		    packet_append_string(rpacket, username);
		}
	    }
	}
    } else
    {
	/* If in a private channel, retreive all users in the channel */
	for (conn = channel_get_first(channel); conn; conn = channel_get_next())
	{
	    if ((conn != c) && (username = conn_get_username(conn)))
	    {
		friend_count++;
		packet_append_string(rpacket, username);
	    }
	}
    }
    bn_byte_set(&rpacket->u.server_w3xp_clan_createreply.friend_count, friend_count);
    conn_push_outqueue(c, rpacket);
    packet_del_ref(rpacket);

    return 0;
}

extern int clanmember_on_change_status(t_clanmember * member)
{
    t_packet *rpacket = NULL;
    if (member == NULL)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clanmember");
	return -1;
    }
    if (member->clan == NULL)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return -1;
    }
    if ((rpacket = packet_create(packet_class_bnet)) != NULL)
    {
	char tmpstr[2];
	const char *append_str = NULL;
	packet_set_size(rpacket, sizeof(t_server_w3xp_clan_memberchangeack));
	packet_set_type(rpacket, SERVER_W3XP_CLAN_MEMBERCHANGEACK);
	packet_append_string(rpacket, account_get_name(member->memberacc));
	tmpstr[0] = member->status;
	append_str = clanmember_get_online_status(member, &tmpstr[1]);
	packet_append_data(rpacket, tmpstr, 2);
	if (append_str)
	    packet_append_string(rpacket, append_str);
	else
	    packet_append_string(rpacket, "");
	clan_send_packet_to_online_members(member->clan, rpacket);
	packet_del_ref(rpacket);
    }
    return 0;
}

extern int clanmember_on_change_status_by_connection(t_connection * conn)
{
    t_packet *rpacket = NULL;
    t_account *acc;
    t_clanmember *member;
    if (conn == NULL)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL conn");
	return -1;
    }
    if ((acc = conn_get_account(conn)) == NULL)
	return -1;
    if ((member = account_get_clanmember(acc)) == NULL)
	return -1;
    if (member->clan == NULL)
	return -1;
    if ((rpacket = packet_create(packet_class_bnet)) != NULL)
    {
	char tmpstr[2];
	const char *append_str = NULL;
	packet_set_size(rpacket, sizeof(t_server_w3xp_clan_memberchangeack));
	packet_set_type(rpacket, SERVER_W3XP_CLAN_MEMBERCHANGEACK);
	packet_append_string(rpacket, account_get_name(acc));
	tmpstr[0] = member->status;
	append_str = clanmember_get_online_status_by_connection(conn, &tmpstr[1]);
	packet_append_data(rpacket, tmpstr, 2);
	if (append_str)
	    packet_append_string(rpacket, append_str);
	else
	    packet_append_string(rpacket, "");
	clan_send_packet_to_online_members(member->clan, rpacket);
	packet_del_ref(rpacket);
    }
    return 0;
}

extern int clan_unload_members(t_clan * clan)
{
    t_elem *curr;
    t_clanmember *member;

    if (clan->members)
    {
	LIST_TRAVERSE(clan->members, curr)
	{
	    if (!(member = elem_get_data(curr)))
	    {
		eventlog(eventlog_level_error, __FUNCTION__, "found NULL entry in list");
		continue;
	    }
	    list_remove_elem(clan->members, curr);
	    free((void *) member);
	}

	if (list_destroy(clan->members) < 0)
	    return -1;

	clan->members = NULL;
    }

    return 0;
}

extern int clan_remove_all_members(t_clan * clan)
{
    t_elem *curr;
    t_clanmember *member;

    if (clan->members)
    {
	LIST_TRAVERSE(clan->members, curr)
	{
	    if (!(member = elem_get_data(curr)))
	    {
		eventlog(eventlog_level_error, __FUNCTION__, "found NULL entry in list");
		continue;
	    }
	    if (member->memberacc != NULL)
		account_set_clanmember(member->memberacc, NULL);
	    list_remove_elem(clan->members, curr);
	    free((void *) member);
	}

	if (list_destroy(clan->members) < 0)
	    return -1;

	clan->members = NULL;
    }

    return 0;
}

extern int clanlist_remove_clan(t_clan * clan)
{
    if (clan == NULL)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "get NULL clan");
	return -1;
    }
    if (list_remove_data(clanlist_head, clan) < 0)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "could not delete clan entry");
	return -1;
    }
    return 0;
}

extern int clan_remove(int clanshort)
{
    return storage->remove_clan(clanshort);
}

extern int clan_save(t_clan * clan)
{
    if (clan->created <= 0)
    {
	if (time(NULL) - clan->creation_time > 120)
	{
	    clanlist_remove_clan(clan);
	    clan_destroy(clan);
	}
	return 0;
    }

    storage->write_clan(clan);

    clan->modified = 0;

    return 0;
}

extern t_list *clanlist()
{
    return clanlist_head;
}

extern int clanlist_add_clan(t_clan * clan)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return -1;
    }

    if (!(clan->clanid))
	clan->clanid = ++max_clanid;

    if (list_append_data(clanlist_head, clan) < 0)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "could not append item");
	clan_remove_all_members(clan);
	if (clan->clan_motd)
	    free((void *) clan->clan_motd);
	if (clan->clanname)
	    free((void *) clan->clanname);
	free((void *) clan);
	return -1;
    }

    return clan->clanid;
}

int clanlist_load()
{
    // make sure to unload previous clanlist before loading again
    if (clanlist_head)
	clanlist_unload();

    if (!(clanlist_head = list_create()))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "could not create clanlist");
	return -1;
    }

    storage->load_clans(_cb_load_clans);

    return 0;
}

extern int clanlist_save()
{
    t_elem *curr;
    t_clan *clan;

    if (clanlist_head)
    {
	LIST_TRAVERSE(clanlist_head, curr)
	{
	    if (!(clan = elem_get_data(curr)))
	    {
		eventlog(eventlog_level_error, __FUNCTION__, "found NULL entry in list");
		continue;
	    }
	    if (clan->modified)
		clan_save(clan);
	}

    }

    return 0;
}

extern int clanlist_unload()
{
    t_elem *curr;
    t_clan *clan;

    if (clanlist_head)
    {
	LIST_TRAVERSE(clanlist_head, curr)
	{
	    if (!(clan = elem_get_data(curr)))
	    {
		eventlog(eventlog_level_error, __FUNCTION__, "found NULL entry in list");
		continue;
	    }
	    if (clan->clanname)
		free((void *) clan->clanname);
	    if (clan->clan_motd)
		free((void *) clan->clan_motd);
	    clan_unload_members(clan);
	    free((void *) clan);
	    list_remove_elem(clanlist_head, curr);
	}

	if (list_destroy(clanlist_head) < 0)
	    return -1;

	clanlist_head = NULL;
    }

    return 0;
}

extern t_clan *clanlist_find_clan_by_clanid(int cid)
{
    t_elem *curr;
    t_clan *clan;

    if (clanlist_head)
    {
	LIST_TRAVERSE(clanlist_head, curr)
	{
	    if (!(clan = elem_get_data(curr)))
	    {
		eventlog(eventlog_level_error, __FUNCTION__, "found NULL entry in list");
		continue;
	    }
	    eventlog(eventlog_level_error, __FUNCTION__, "trace %d", clan->clanid);
	    if (clan->created && (clan->clanid == cid))
		return clan;
	}

    }

    return NULL;
}

extern t_clan *clanlist_find_clan_by_clanshort(int clanshort)
{
    t_elem *curr;
    t_clan *clan;

    if (clanshort == 0)
	return NULL;
    if (clanlist_head)
    {
	LIST_TRAVERSE(clanlist_head, curr)
	{
	    if (!(clan = elem_get_data(curr)))
	    {
		eventlog(eventlog_level_error, __FUNCTION__, "found NULL entry in list");
		continue;
	    }
	    if (clan->created && (clan->clanshort == clanshort))
		return clan;
	}

    }

    return NULL;
}

extern t_clanmember *clan_find_member(t_clan * clan, t_account * memberacc)
{
    t_clanmember *member;
    t_elem *curr;
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return NULL;
    }
    if (!(clan->members))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "found NULL clan->members");
	return NULL;
    }
    LIST_TRAVERSE(clan->members, curr)
    {
	if (!(member = elem_get_data(curr)))
	{
	    eventlog(eventlog_level_error, __FUNCTION__, "got NULL element in list");
	    return NULL;
	}
	if (member->memberacc == memberacc)
	    return member;
    }

    return NULL;
}

extern t_clanmember *clan_find_member_by_name(t_clan * clan, char const *membername)
{
    t_clanmember *member;
    t_elem *curr;
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return NULL;
    }
    if (!(clan->members))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "found NULL clan->members");
	return NULL;
    }
    LIST_TRAVERSE(clan->members, curr)
    {
	if (!(member = elem_get_data(curr)))
	{
	    eventlog(eventlog_level_error, __FUNCTION__, "got NULL element in list");
	    return NULL;
	}
	if (strcmp(account_get_name(member->memberacc), membername) == 0)
	    return member;
    }

    return NULL;
}

extern t_clanmember *clan_find_member_by_uid(t_clan * clan, unsigned int memberuid)
{
    t_clanmember *member;
    t_elem *curr;
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return NULL;
    }
    if (!(clan->members))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "found NULL clan->members");
	return NULL;
    }
    LIST_TRAVERSE(clan->members, curr)
    {
	if (!(member = elem_get_data(curr)))
	{
	    eventlog(eventlog_level_error, __FUNCTION__, "got NULL element in list");
	    return NULL;
	}
	if (account_get_uid(member->memberacc) == memberuid)
	    return member;
    }

    return NULL;
}

extern t_account *clanmember_get_account(t_clanmember * member)
{
    if (!(member))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clanmember");
	return NULL;
    }

    return (t_account *) member->memberacc;
}

extern int clanmember_set_account(t_clanmember * member, t_account * memberacc)
{
    if (!(member))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clanmember");
	return -1;
    }

    member->memberacc = memberacc;
    return 0;
}

extern t_connection *clanmember_get_connection(t_clanmember * member)
{
    if (!(member))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clanmember");
	return NULL;
    }

    return (t_connection *) member->memberconn;
}

extern int clanmember_set_connection(t_clanmember * member, t_connection * memberconn)
{
    if (!(member))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clanmember");
	return -1;
    }

    member->memberconn = memberconn;

    return 0;
}

extern char clanmember_get_status(t_clanmember * member)
{
    if (!(member))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clanmember");
	return 0;
    }

    if ((member->status == CLAN_NEW) && (time(NULL) - member->join_time > prefs_get_clan_newer_time() * 3600))
    {
	member->status = CLAN_PEON;
	member->clan->modified = 1;
#ifdef WITH_SQL
	member->modified = 1;
#endif
    }

    return member->status;
}

extern int clanmember_set_status(t_clanmember * member, char status)
{
    if (!(member))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clanmember");
	return -1;
    }

    if (member->status != status)
    {
	member->status = status;
	member->clan->modified = 1;
#ifdef WITH_SQL
	member->modified = 1;
#endif
    }
    return 0;
}

extern time_t clanmember_get_join_time(t_clanmember * member)
{
    if (!(member))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clanmember");
	return 0;
    }

    return member->join_time;
}

extern t_clan *clanmember_get_clan(t_clanmember * member)
{
    if (!(member))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clanmember");
	return 0;
    }

    return member->clan;
}

extern const char *clanmember_get_online_status(t_clanmember * member, char *status)
{
    if (member->memberconn)
    {
	t_game *game;
	t_channel *channel;
	if ((game = conn_get_game(member->memberconn)) != NULL)
	{
	    if (game_get_flag_private(game))
		(*status) = SERVER_W3XP_CLAN_MEMBER_PRIVATE_GAME;
	    else
		(*status) = SERVER_W3XP_CLAN_MEMBER_GAME;
	    return game_get_name(game);
	}
	if ((channel = conn_get_channel(member->memberconn)) != NULL)
	{
	    (*status) = SERVER_W3XP_CLAN_MEMBER_CHANNEL;
	    return channel_get_name(channel);
	}

	(*status) = SERVER_W3XP_CLAN_MEMBER_ONLINE;
    }
    (*status) = SERVER_W3XP_CLAN_MEMBER_OFFLINE;
    return NULL;
}

extern const char *clanmember_get_online_status_by_connection(t_connection * conn, char *status)
{
    if (conn)
    {
	t_game *game;
	t_channel *channel;
	if ((game = conn_get_game(conn)) != NULL)
	{
	    if (game_get_flag_private(game))
		(*status) = SERVER_W3XP_CLAN_MEMBER_PRIVATE_GAME;
	    else
		(*status) = SERVER_W3XP_CLAN_MEMBER_GAME;
	    return game_get_name(game);
	}
	if ((channel = conn_get_channel(conn)) != NULL)
	{
	    (*status) = SERVER_W3XP_CLAN_MEMBER_CHANNEL;
	    return channel_get_name(channel);
	}

	(*status) = SERVER_W3XP_CLAN_MEMBER_ONLINE;
    }
    (*status) = SERVER_W3XP_CLAN_MEMBER_OFFLINE;
    return NULL;
}

extern int clanmember_set_online(t_connection * c)
{
    t_clanmember *member;
    t_account *acc;

    if (!c)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL connection");
	return -1;
    }

    if ((acc = conn_get_account(c)) != NULL && ((member = account_get_clanmember(acc)) != NULL))
    {
	clanmember_set_connection(member, c);
	clanmember_on_change_status(member);
    }

    return 0;
}

extern int clanmember_set_offline(t_connection * c)
{
    t_clanmember *member;
    t_account *acc;

    if (!c)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL connection");
	return -1;
    }

    if ((acc = conn_get_account(c)) != NULL && ((member = account_get_clanmember(acc)) != NULL))
    {
	clanmember_set_connection(member, NULL);
	clanmember_on_change_status(member);
    }

    return 0;
}

extern int clan_get_created(t_clan * clan)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return -1;
    }

    return clan->created;
}

extern int clan_set_created(t_clan * clan, int created)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return -1;
    }

    clan->created = created;

    return 0;
}

extern char clan_get_modified(t_clan * clan)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return -1;
    }

    return clan->modified;
}

extern int clan_set_modified(t_clan * clan, char modified)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return -1;
    }

    clan->modified = modified;

    return 0;
}

extern char clan_get_channel_type(t_clan * clan)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return -1;
    }

    return clan->channel_type;
}

extern int clan_set_channel_type(t_clan * clan, char channel_type)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return -1;
    }

    clan->channel_type = channel_type;

    return 0;
}

extern t_list *clan_get_members(t_clan * clan)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return NULL;
    }

    return clan->members;
}

extern char const *clan_get_name(t_clan * clan)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return NULL;
    }

    return clan->clanname;
}

extern int clan_get_clanshort(t_clan * clan)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return 0;
    }

    return clan->clanshort;
}

extern char const *clan_get_motd(t_clan * clan)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return NULL;
    }

    return clan->clan_motd;
}

extern int clan_set_motd(t_clan * clan, const char *motd)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return -1;
    }
    if (!(motd))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL motd");
	return -1;
    } else
    {
	if (clan->clan_motd)
	    free((void *) clan->clan_motd);
	clan->clan_motd = strdup(motd);
    }
    return 0;
}

extern unsigned int clan_get_clanid(t_clan * clan)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return -1;
    }

    return clan->clanid;
}

extern time_t clan_get_creation_time(t_clan * clan)
{
    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return 0;
    }

    return clan->creation_time;
}

extern t_clanmember *clan_add_member(t_clan * clan, t_account * memberacc, t_connection * memberconn, char status)
{
    t_clanmember *member;

    if (!(clan))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clan");
	return NULL;
    }

    if (!(clan->members))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "found NULL clan->members");
	return NULL;
    }

    if (!(member = malloc(sizeof(t_clanmember))))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "could not allocate memory for clanmember");
	return NULL;
    }

    member->memberacc = memberacc;
    member->memberconn = memberconn;
    member->status = status;
    member->join_time = time(0);
    member->clan = clan;
#ifdef WITH_SQL
    member->modified = 1;
#endif

    if (list_append_data(clan->members, member) < 0)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "could not append item");
	free((void *) member);
	return NULL;
    }

    account_set_clanmember(memberacc, member);

    clan->modified = 1;

    return member;
}

extern int clan_remove_member(t_clan * clan, t_clanmember * member)
{
    if (!member)
	return -1;
    if (list_remove_data(clan->members, member) < 0)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "could not remove member");
	return -1;
    }
    if (member->memberacc != NULL)
    {
	account_set_clanmember(member->memberacc, NULL);
	storage->remove_clanmember(account_get_uid(member->memberacc));
    }
    free((void *) member);
    list_purge(clan->members);
    clan->modified = 1;
    return 0;
}

extern t_clan *clan_create(t_account * chieftain_acc, t_connection * chieftain_conn, int clanshort, const char *clanname, const char *motd)
{
    t_clan *clan;
    t_clanmember *member;

    if (!(clan = malloc(sizeof(t_clan))))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "could not allocate memory for new clan");
	return NULL;
    }

    if (!(member = malloc(sizeof(t_clanmember))))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "could not allocate memory for clanmember");
	free((void *) clan);
	return NULL;
    }

    if (!(clanname))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "got NULL clanname");
	free((void *) clan);
	free((void *) member);
	return NULL;
    }

    if (!(clan->clanname = strdup(clanname)))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "could not allocate memory for clanname");
	free((void *) clan);
	free((void *) member);
	return NULL;
    }

    if (!(motd))
    {
	clan->clan_motd = strdup("This is a newly created clan");
    } else
    {
	clan->clan_motd = strdup(motd);
    }
    if (!(clan->clan_motd))
    {
	eventlog(eventlog_level_error, __FUNCTION__, "could not allocate memory for clan_motd");
	if (clan->clanname)
	    free((void *) clan->clanname);
	free((void *) member);
	free((void *) clan);
	return NULL;
    }

    clan->creation_time = time(0);
    clan->clanshort = clanshort;
    clan->clanid = ++max_clanid;
    clan->created = 0;
    clan->modified = 1;
    clan->channel_type = prefs_get_clan_channel_default_private();

    if ((clan->members = list_create()) < 0)
    {
	eventlog(eventlog_level_error, __FUNCTION__, "could not create memberlist");
	if (clan->clanname)
	    free((void *) clan->clanname);
	if (clan->clan_motd)
	    free((void *) clan->clan_motd);
	free((void *) member);
	free((void *) clan);
	return NULL;
    }

    member->memberacc = chieftain_acc;
    member->memberconn = chieftain_conn;
    member->status = CLAN_CHIEFTAIN;
    member->join_time = clan->creation_time;
    member->clan = clan;
#ifdef WITH_SQL
    member->modified = 1;
#endif

    if (list_append_data(clan->members, member) < 0)
    {
	t_elem *curr;
	eventlog(eventlog_level_error, __FUNCTION__, "could not append item");
	if (clan->clanname)
	    free((void *) clan->clanname);
	if (clan->clan_motd)
	    free((void *) clan->clan_motd);
	free((void *) member);
	LIST_TRAVERSE(clan->members, curr)
	{
	    t_clanmember *member;
	    if ((member = elem_get_data(curr)) != NULL)
		free(member);
	    list_remove_elem(clan->members, curr);
	}
	list_destroy(clan->members);
	free((void *) clan);
	return NULL;
    }
    account_set_clanmember(chieftain_acc, member);

    return clan;
}

extern int clan_destroy(t_clan * clan)
{
    if (!clan)
	return 0;
    if (clan->clanname)
	free((void *) clan->clanname);
    if (clan->clan_motd)
	free((void *) clan->clan_motd);
    clan_remove_all_members(clan);
    free((void *) clan);
    return 0;
}

extern int clan_get_member_count(t_clan * clan)
{
    t_elem *curr;
    int count = 0;
    LIST_TRAVERSE(clan->members, curr)
    {
	t_clanmember *member;
	if ((member = elem_get_data(curr)) != NULL)
	    count++;
    }
    return count;
}

extern int str_to_clanshort(const char *str)
{
    int clanshort = 0;

    if (!str)
	return 0;

    if (str[0])
    {
	clanshort = str[0] << 24;
	if (str[1])
	{
	    clanshort += str[1] << 16;
	    if (str[2])
	    {
		clanshort += str[2] << 8;
		if (str[3])
		    clanshort += str[3];
	    }
	}
    }
    return clanshort;

}
