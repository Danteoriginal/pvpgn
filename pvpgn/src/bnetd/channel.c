/*
 * Copyright (C) 1998  Mark Baysinger (mbaysing@ucsd.edu)
 * Copyright (C) 1998,1999,2000  Ross Combs (rocombs@cs.nmsu.edu)
 * Copyright (C) 2000,2001  Marco Ziech (mmz@gmx.net)
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
#define CHANNEL_INTERNAL_ACCESS
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
#include "connection.h"
#include "common/eventlog.h"
#include "common/list.h"
#include "message.h"
#include "account.h"
#include "common/util.h"
#include "prefs.h"
#include "common/token.h"
#include "channel.h"
#include "common/setup_after.h"
#include "irc.h"
#include "common/tag.h"


static t_list * channellist_head=NULL;

static t_channelmember * memberlist_curr=NULL;
static int totalcount=0;


static int channellist_load_permanent(char const * filename);
static t_channel * channellist_find_channel_by_fullname(char const * name);
static char * channel_format_name(char const * sname, char const * country, char const * realmname, unsigned int id);

extern int channel_set_flags(t_connection * c);

extern t_channel * channel_create(char const * fullname, char const * shortname, char const * clienttag, int permflag, int botflag, int operflag, int logflag, char const * country, char const * realmname, int maxmembers, int moderated, int clanflag)
{
    t_channel * channel;
    
    if (!fullname)
    {
        eventlog(eventlog_level_error,"channel_create","got NULL fullname");
	return NULL;
    }
    if (fullname[0]=='\0')
    {
        eventlog(eventlog_level_error,"channel_create","got empty fullname");
	return NULL;
    }
    if (shortname && shortname[0]=='\0')
    {
        eventlog(eventlog_level_error,"channel_create","got empty shortname");
	return NULL;
    }
    if (clienttag && strlen(clienttag)!=4)
    {
	eventlog(eventlog_level_error,"channel_create","client tag has bad length (%u chars)",strlen(clienttag));
	return NULL;
    }
    
    /* non-permanent already checks for this in conn_set_channel */
    if (permflag)
    {
	if ((channel = channellist_find_channel_by_fullname(fullname)))
	{
	    if ((channel_get_clienttag(channel)) && (clienttag) && (strcmp(channel_get_clienttag(channel),clienttag)==0))
	    {
	      eventlog(eventlog_level_error,"channel_create","could not create duplicate permanent channel (fullname \"%s\")",fullname);
	      return NULL;
	    }
	    else if (((channel->flags & channel_flags_allowbots)!=botflag) || 
		     ((channel->flags & channel_flags_allowopers)!=operflag) || 
		     (channel->maxmembers!=maxmembers) || 
		     ((channel->flags & channel_flags_moderated)!=moderated) ||
		     (channel->logname && logflag==0) || (!(channel->logname) && logflag ==1))
	    {
		eventlog(eventlog_level_error,__FUNCTION__,"channel parameters do not match for \"%s\" and \"%s\"",fullname,channel->name);
		return NULL;
	    }
	}
    }
    
    if (!(channel = malloc(sizeof(t_channel))))
    {
        eventlog(eventlog_level_error,"channel_create","could not allocate memory for channel");
        return NULL;
    }
    
    if (permflag)
    {
	channel->flags = channel_flags_public;
	if (clienttag && maxmembers!=-1) /* approximation.. we want things like "Starcraft USA-1" */
	    channel->flags |= channel_flags_system;
    } else
	channel->flags = channel_flags_none;

    if (moderated)
	channel->flags |= channel_flags_moderated;

    if(shortname && (!strcasecmp(shortname, CHANNEL_NAME_KICKED)
       || !strcasecmp(shortname, CHANNEL_NAME_BANNED)))
	channel->flags |= channel_flags_thevoid;
    
    eventlog(eventlog_level_debug,"channel_create","creating new channel \"%s\" shortname=%s%s%s clienttag=%s%s%s country=%s%s%s realm=%s%s%s",fullname,
	     shortname?"\"":"(", /* all this is doing is printing the name in quotes else "none" in parens */
	     shortname?shortname:"none",
	     shortname?"\"":")",
	     clienttag?"\"":"(",
	     clienttag?clienttag:"none",
	     clienttag?"\"":")",
	     country?"\"":"(",
             country?country:"none",
	     country?"\"":")",
	     realmname?"\"":"(",
             realmname?realmname:"none",
             realmname?"\"":")");

    
    if (!(channel->name = strdup(fullname)))
    {
        eventlog(eventlog_level_info,"channel_create","unable to allocate memory for channel->name");
	free(channel);
	return NULL;
    }
    
    if (!shortname)
	channel->shortname = NULL;
    else
	if (!(channel->shortname = strdup(shortname)))
	{
	    eventlog(eventlog_level_info,"channel_create","unable to allocate memory for channel->shortname");
	    free((void *)channel->name); /* avoid warning */
	    free(channel);
	    return NULL;
	}
    
    if (clienttag)
    {
	if (!(channel->clienttag = strdup(clienttag)))
	{
	    eventlog(eventlog_level_error,"channel_create","could not allocate memory for channel->clienttag");
	    if (channel->shortname)
		free((void *)channel->shortname); /* avoid warning */
	    free((void *)channel->name); /* avoid warning */
	    free(channel);
	    return NULL;
	}
    }
    else
	channel->clienttag = NULL;

    if (country)
    {
	if (!(channel->country = strdup(country)))
	{
            eventlog(eventlog_level_info,"channel_create","unable to allocate memory for channel->country");
	    if (channel->clienttag)
	        free((void *)channel->clienttag); /* avoid warning */
	    if (channel->shortname)
	        free((void *)channel->shortname); /* avoid warning */
	    free((void *)channel->name); /* avoid warning */
	    free(channel);
	    return NULL;
    	}
    }
    else
	channel->country = NULL;
	
    if (realmname)
    {
	if (!(channel->realmname = strdup(realmname)))
        {
            eventlog(eventlog_level_info,"channel_create","unable to allocate memory for channel->realmname");
	    if (channel->country)
	    	free((void *)channel->country); /* avoid warning */
	    if (channel->clienttag)
	        free((void *)channel->clienttag); /* avoid warning */
	    if (channel->shortname)
	        free((void *)channel->shortname); /* avoid warning */
	    free((void *)channel->name); /* avoid warning */
	    free(channel);
	    return NULL;
    	}
    }
    else
        channel->realmname=NULL;
	
    if (!(channel->banlist = list_create()))
    {
	eventlog(eventlog_level_error,"channel_create","could not create list");
	if (channel->country)
	    free((void *)channel->country); /* avoid warning */
        if (channel->realmname)
            free((void *)channel->realmname); /*avoid warining */
	if (channel->clienttag)
	    free((void *)channel->clienttag); /* avoid warning */
	if (channel->shortname)
	    free((void *)channel->shortname); /* avoid warning */
	free((void *)channel->name); /* avoid warning */
	free(channel);
	return NULL;
    }
    
    totalcount++;
    if (totalcount==0) /* if we wrap (yeah right), don't use id 0 */
	totalcount = 1;
    channel->id = totalcount;
    channel->maxmembers = maxmembers;
    channel->currmembers = 0;
    channel->memberlist = NULL;
    
    if (permflag) channel->flags |= channel_flags_permanent;
    if (botflag)  channel->flags |= channel_flags_allowbots;
    if (operflag) channel->flags |= channel_flags_allowopers;
    if (clanflag) channel->flags |= channel_flags_clan;
    
    if (logflag)
    {
	time_t      now;
	struct tm * tmnow;
	char        dstr[64];
	char        timetemp[CHANLOG_TIME_MAXLEN];
	
	now = time(NULL);
	
	if (!(tmnow = localtime(&now)))
	    dstr[0] = '\0';
	else
	    sprintf(dstr,"%04d%02d%02d%02d%02d%02d",
		    1900+tmnow->tm_year,
		    tmnow->tm_mon+1,
		    tmnow->tm_mday,
		    tmnow->tm_hour,
		    tmnow->tm_min,
		    tmnow->tm_sec);
	
	if (!(channel->logname = malloc(strlen(prefs_get_chanlogdir())+9+strlen(dstr)+1+6+1))) /* dir + "/chanlog-" + dstr + "-" + id + NUL */
	{
	    eventlog(eventlog_level_error,"channel_create","could not allocate memory for channel->logname");
	    list_destroy(channel->banlist);
	    if (channel->country)
		free((void *)channel->country); /* avoid warning */
            if (channel->realmname)
                free((void *) channel->realmname); /* avoid warning */
	    if (channel->clienttag)
		free((void *)channel->clienttag); /* avoid warning */
	    if (channel->shortname)
		free((void *)channel->shortname); /* avoid warning */
	    free((void *)channel->name); /* avoid warning */
	    free(channel);
	    return NULL;
	}
	sprintf(channel->logname,"%s/chanlog-%s-%06u",prefs_get_chanlogdir(),dstr,channel->id);
	
	if (!(channel->log = fopen(channel->logname,"w")))
	    eventlog(eventlog_level_error,"channel_create","could not open channel log \"%s\" for writing (fopen: %s)",channel->logname,strerror(errno));
	else
	{
	    fprintf(channel->log,"name=\"%s\"\n",channel->name);
	    if (channel->shortname)
		fprintf(channel->log,"shortname=\"%s\"\n",channel->shortname);
	    else
		fprintf(channel->log,"shortname=none\n");
	    fprintf(channel->log,"permanent=\"%s\"\n",(channel->flags & channel_flags_permanent)?"true":"false");
	    fprintf(channel->log,"allowbotse=\"%s\"\n",(channel->flags & channel_flags_allowbots)?"true":"false");
	    fprintf(channel->log,"allowopers=\"%s\"\n",(channel->flags & channel_flags_allowopers)?"true":"false");
	    if (channel->clienttag)
		fprintf(channel->log,"clienttag=\"%s\"\n",channel->clienttag);
	    else
		fprintf(channel->log,"clienttag=none\n");
	    
	    if (tmnow)
		strftime(timetemp,sizeof(timetemp),CHANLOG_TIME_FORMAT,tmnow);
	    else
		strcpy(timetemp,"?");
	    fprintf(channel->log,"created=\"%s\"\n\n",timetemp);
	    fflush(channel->log);
	}
    }
    else
    {
	channel->logname = NULL;
	channel->log = NULL;
    }
    
    if (list_append_data(channellist_head,channel)<0)
    {
        eventlog(eventlog_level_error,"channel_create","could not prepend temp");
	if (channel->log)
	    if (fclose(channel->log)<0)
		eventlog(eventlog_level_error,"channel_create","could not close channel log \"%s\" after writing (fclose: %s)",channel->logname,strerror(errno));
	if (channel->logname)
	    free((void *)channel->logname); /* avoid warning */
	list_destroy(channel->banlist);
	if (channel->country)
	    free((void *)channel->country); /* avoid warning */
        if (channel->realmname)
            free((void *) channel->realmname); /* avoid warning */
	if (channel->clienttag)
	    free((void *)channel->clienttag); /* avoid warning */
	if (channel->shortname)
	    free((void *)channel->shortname); /* avoid warning */
	free((void *)channel->name); /* avoid warning */
	free(channel);
        return NULL;
    }
    
    eventlog(eventlog_level_debug,"channel_create","channel created successfully");
    return channel;
}


extern int channel_destroy(t_channel * channel)
{
    t_elem * ban;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_destroy","got NULL channel");
	return -1;
    }
    
    if (channel->memberlist)
    {
	eventlog(eventlog_level_debug,"channel_destroy","channel is not empty, deferring");
        channel->flags &= ~channel_flags_permanent; /* make it go away when the last person leaves */
	return -1;
    }
    
    if (list_remove_data(channellist_head,channel)<0)
    {
        eventlog(eventlog_level_error,"channel_destroy","could not remove item from list");
        return -1;
    }
    
    eventlog(eventlog_level_info,"channel_destroy","destroying channel \"%s\"",channel->name);
    
    LIST_TRAVERSE(channel->banlist,ban)
    {
	char const * banned;
	
	if (!(banned = elem_get_data(ban)))
	    eventlog(eventlog_level_error,"channel_destroy","found NULL name in banlist");
	else
	    free((void *)banned); /* avoid warning */
	if (list_remove_elem(channel->banlist,ban)<0)
	    eventlog(eventlog_level_error,"channel_destroy","unable to remove item from list");
    }
    list_destroy(channel->banlist);
    
    if (channel->log)
    {
	time_t      now;
	struct tm * tmnow;
	char        timetemp[CHANLOG_TIME_MAXLEN];
	
	now = time(NULL);
	if ((!(tmnow = localtime(&now))))
	    strcpy(timetemp,"?");
	else
	    strftime(timetemp,sizeof(timetemp),CHANLOG_TIME_FORMAT,tmnow);
	fprintf(channel->log,"\ndestroyed=\"%s\"\n",timetemp);
	
	if (fclose(channel->log)<0)
	    eventlog(eventlog_level_error,"channel_destroy","could not close channel log \"%s\" after writing (fclose: %s)",channel->logname,strerror(errno));
    }
    
    if (channel->logname)
	free((void *)channel->logname); /* avoid warning */
    
    if (channel->country)
	free((void *)channel->country); /* avoid warning */
    
    if (channel->realmname)
	free((void *)channel->realmname); /* avoid warning */

    if (channel->clienttag)
	free((void *)channel->clienttag); /* avoid warning */
    
    if (channel->shortname)
	free((void *)channel->shortname); /* avoid warning */

    free((void *)channel->name); /* avoid warning */
    
    free(channel);
    
    return 0;
}


extern char const * channel_get_name(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_warn,"channel_get_name","got NULL channel");
	return "";
    }
    
    return channel->name;
}


extern char const * channel_get_clienttag(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_error,"channel_get_clienttag","got NULL channel");
	return "";
    }
    
    return channel->clienttag;
}


extern t_channel_flags channel_get_flags(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_error,"channel_get_flags","got NULL channel");
	return channel_flags_none;
    }
    
    return channel->flags;
}


extern int channel_get_permanent(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_error,"channel_get_permanent","got NULL channel");
	return 0;
    }
    
    return (channel->flags & channel_flags_permanent);
}


extern unsigned int channel_get_channelid(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_error,"channel_get_channelid","got NULL channel");
	return 0;
    }
    return channel->id;
}


extern int channel_set_channelid(t_channel * channel, unsigned int channelid)
{
    if (!channel)
    {
        eventlog(eventlog_level_error,"channel_set_channelid","got NULL channel");
	return -1;
    }
    channel->id = channelid;
    return 0;
}

extern int channel_rejoin(t_connection * conn)
{
  t_channel const * channel;
  char const * temp;
  char const * chname;

  if (!(channel = conn_get_channel(conn)))
    return -1;

  if (!(temp = channel_get_name(channel)))
    return -1;

  if ((chname=strdup(temp)))
  {
    conn_set_channel(conn, NULL);
    if (conn_set_channel(conn,chname)<0)
      conn_set_channel(conn,CHANNEL_NAME_BANNED);
    free((void *)chname);
  }
  return 0;  
}


extern int channel_add_connection(t_channel * channel, t_connection * connection)
{
    t_channelmember * member;
    t_connection *    user;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_add_connection","got NULL channel");
	return -1;
    }
    if (!connection)
    {
	eventlog(eventlog_level_error,"channel_add_connection","got NULL connection");
	return -1;
    }
    
    if (channel_check_banning(channel,connection))
    {
	channel_message_log(channel,connection,0,"JOIN FAILED (banned)");
	return -1;
    }
    
    if (!(member = malloc(sizeof(t_channelmember))))
    {
	eventlog(eventlog_level_error,"channel_add_connection","could not allocate memory for channelmember");
	return -1;
    }
    member->connection = connection;
    member->next = channel->memberlist;
    channel->memberlist = member;
    channel->currmembers++;

    if ((!(channel->flags & channel_flags_permanent)) 
        && (!(channel->flags & channel_flags_thevoid)) 
        && (!(channel->flags & channel_flags_clan)) 
	&& (channel->currmembers==1) 
	&& (account_is_operator_or_admin(conn_get_account(connection),channel_get_name(channel))==0))
    {
	message_send_text(connection,message_type_info,connection,"you are now tempOP for this channel");
	account_set_tmpOP_channel(conn_get_account(connection),(char *)channel_get_name(channel));
    }
    
    channel_message_log(channel,connection,0,"JOINED");
    
    message_send_text(connection,message_type_channel,connection,channel_get_name(channel));

    if(!(channel_get_flags(channel) & channel_flags_thevoid))
        for (user=channel_get_first(channel); user; user=channel_get_next())
        {
	    message_send_text(connection,message_type_adduser,user,NULL);
    	    if (user!=connection)
    		message_send_text(user,message_type_join,connection,NULL);
        }
    
    /* please don't remove this notice */
    if (channel->log)
	message_send_text(connection,message_type_info,connection,prefs_get_log_notice());
    
    return 0;
}


extern int channel_del_connection(t_channel * channel, t_connection * connection)
{
    t_channelmember * curr;
    t_channelmember * temp;
    t_account 	    * acc;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_del_connection","got NULL channel");
        return -1;
    }
    if (!connection)
    {
	eventlog(eventlog_level_error,"channel_del_connection","got NULL connection");
        return -1;
    }
    
    channel_message_log(channel,connection,0,"PARTED");
    
    channel_message_send(channel,message_type_part,connection,NULL);
    
    curr = channel->memberlist;
    if (curr->connection==connection)
    {
        channel->memberlist = channel->memberlist->next;
        free(curr);
    }
    else
    {
        while (curr->next && curr->next->connection!=connection)
            curr = curr->next;
        
        if (curr->next)
        {
            temp = curr->next;
            curr->next = curr->next->next;
            free(temp);
        }
	else
	{
	    eventlog(eventlog_level_error,"channel_del_connection","[%d] connection not in channel member list",conn_get_socket(connection));
	    return -1;
	}
    }
    channel->currmembers--;

    acc = conn_get_account(connection);

    if (account_get_tmpOP_channel(acc) && 
	strcmp(account_get_tmpOP_channel(acc),channel_get_name(channel))==0)
    {
	account_set_tmpOP_channel(acc,NULL);
    }
    
    if (!channel->memberlist && !(channel->flags & channel_flags_permanent)) /* if channel is empty, delete it unless it's a permanent channel */
    {
	channel_destroy(channel);
	list_purge(channellist_head);
    }
    
    return 0;
}


extern void channel_update_latency(t_connection * me)
{
    t_channel *    channel;
    t_message *    message;
    t_connection * c;
    
    if (!me)
    {
	eventlog(eventlog_level_error,"channel_update_latency","got NULL connection");
        return;
    }
    if (!(channel = conn_get_channel(me)))
    {
	eventlog(eventlog_level_error,"channel_update_latency","connection has no channel");
        return;
    }
    
    if (!(message = message_create(message_type_userflags,me,NULL,NULL))) /* handles NULL text */
	return;

    for (c=channel_get_first(channel); c; c=channel_get_next())
        if (conn_get_class(c)==conn_class_bnet)
            message_send(message,c);
    message_destroy(message);
}


extern void channel_update_flags(t_connection * me)
{
    t_channel *    channel;
    t_message *    message;
    t_connection * c;
    
    if (!me)
    {
	eventlog(eventlog_level_error,"channel_update_flags","got NULL connection");
        return;
    }
    if (!(channel = conn_get_channel(me)))
    {
	eventlog(eventlog_level_error,"channel_update_flags","connection has no channel");
        return;
    }
    
    if (!(message = message_create(message_type_userflags,me,NULL,NULL))) /* handles NULL text */
	return;
    
    for (c=channel_get_first(channel); c; c=channel_get_next())
	message_send(message,c);
    
    message_destroy(message);
}


extern void channel_message_log(t_channel const * channel, t_connection * me, int fromuser, char const * text)
{
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_message_log","got NULL channel");
        return;
    }
    if (!me)
    {
	eventlog(eventlog_level_error,"channel_message_log","got NULL connection");
        return;
    }
    
    if (channel->log)
    {
	time_t       now;
	struct tm *  tmnow;
	char         timetemp[CHANLOG_TIME_MAXLEN];
	char const * tname;
	
	now = time(NULL);
	if ((!(tmnow = localtime(&now))))
	    strcpy(timetemp,"?");
	else
	    strftime(timetemp,sizeof(timetemp),CHANLOGLINE_TIME_FORMAT,tmnow);
	
	if (fromuser)
	    fprintf(channel->log,"%s: \"%s\" \"%s\"\n",timetemp,(tname = conn_get_username(me)),text);
	else
	    fprintf(channel->log,"%s: \"%s\" %s\n",timetemp,(tname = conn_get_username(me)),text);
	conn_unget_username(me,tname);
	fflush(channel->log);
    }
}


extern void channel_message_send(t_channel const * channel, t_message_type type, t_connection * me, char const * text)
{
    t_connection * c;
    unsigned int   heard;
    t_message *    message;
    char const *   tname;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_message_send","got NULL channel");
        return;
    }
    if (!me)
    {
	eventlog(eventlog_level_error,"channel_message_send","got NULL connection");
        return;
    }

    if(channel_get_flags(channel) & channel_flags_thevoid) // no talking in the void
	return;

    if(channel_get_flags(channel) & channel_flags_moderated) // moderated channel - only admins,OPs and voices may talk
    {
	if (type==message_type_talk || type==message_type_emote)
	{
	    if (!((account_is_operator_or_admin(conn_get_account(me),channel_get_name(channel))) ||
		 (channel_account_has_tmpVOICE(channel,conn_get_account(me)))))
	    {
		message_send_text(me,message_type_error,me,"This channel is moderated");
	        return;
	    }
	}
    }
    
    if (!(message = message_create(type,me,NULL,text)))
    {
	eventlog(eventlog_level_error,"channel_message_send","could not create message");
	return;
    }
    
    heard = 0;
    tname = conn_get_chatname(me);
    for (c=channel_get_first(channel); c; c=channel_get_next())
    {
	if (c==me && (type==message_type_talk || type==message_type_join || type==message_type_part))
	    continue; /* ignore ourself */
	if ((type==message_type_talk || type==message_type_whisper || type==message_type_emote || type==message_type_broadcast) &&
	    conn_check_ignoring(c,tname)==1)
	    continue; /* ignore squelched players */
	
	if (message_send(message,c)==0 && c!=me)
	    heard = 1;
    }
    conn_unget_chatname(me,tname);
    message_destroy(message);
    
    if (!heard && (type==message_type_talk || type==message_type_emote))
	message_send_text(me,message_type_info,me,"No one hears you.");
}

extern int channel_ban_user(t_channel * channel, char const * user)
{
    t_elem const * curr;
    char *         temp;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_ban_user","got NULL channel");
	return -1;
    }
    if (!user)
    {
	eventlog(eventlog_level_error,"channel_ban_user","got NULL user");
	return -1;
    }
    if (!channel->name)
    {
	eventlog(eventlog_level_error,"channel_ban_user","got channel with NULL name");
	return -1;
    }
    
    if (strcasecmp(channel->name,CHANNEL_NAME_BANNED)==0 ||
	strcasecmp(channel->name,CHANNEL_NAME_KICKED)==0)
        return -1;
    
    LIST_TRAVERSE_CONST(channel->banlist,curr)
        if (strcasecmp(elem_get_data(curr),user)==0)
            return 0;
    
    if (!(temp = strdup(user)))
    {
        eventlog(eventlog_level_error,"channel_ban_user","could not allocate memory for temp");
        return -1;
    }
    if (list_append_data(channel->banlist,temp)<0)
    {
	free(temp);
        eventlog(eventlog_level_error,"channel_ban_user","unable to append to list");
        return -1;
    }
    return 0;
}


extern int channel_unban_user(t_channel * channel, char const * user)
{
    t_elem * curr;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_unban_user","got NULL channel");
	return -1;
    }
    if (!user)
    {
	eventlog(eventlog_level_error,"channel_unban_user","got NULL user");
	return -1;
    }
    
    LIST_TRAVERSE(channel->banlist,curr)
    {
	char const * banned;
	
	if (!(banned = elem_get_data(curr)))
	{
            eventlog(eventlog_level_error,"channel_unban_user","found NULL name in banlist");
	    continue;
	}
        if (strcasecmp(banned,user)==0)
        {
            if (list_remove_elem(channel->banlist,curr)<0)
            {
                eventlog(eventlog_level_error,"channel_unban_user","unable to remove item from list");
                return -1;
            }
            free((void *)banned); /* avoid warning */
            return 0;
        }
    }
    
    return -1;
}


extern int channel_check_banning(t_channel const * channel, t_connection const * user)
{
    t_elem const * curr;
    
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_check_banning","got NULL channel");
	return -1;
    }
    if (!user)
    {
	eventlog(eventlog_level_error,"channel_check_banning","got NULL user");
	return -1;
    }
    
    if (!(channel->flags & channel_flags_allowbots) && conn_get_class(user)==conn_class_bot)
	return 1;
    
    LIST_TRAVERSE_CONST(channel->banlist,curr)
        if (conn_match(user,elem_get_data(curr))==1)
            return 1;
    
    return 0;
}


extern int channel_get_length(t_channel const * channel)
{
    t_channelmember const * curr;
    int                     count;
    
    for (curr=channel->memberlist,count=0; curr; curr=curr->next,count++);
    
    return count;
}


extern t_connection * channel_get_first(t_channel const * channel)
{
    if (!channel)
    {
	eventlog(eventlog_level_error,"channel_get_first","got NULL channel");
        return NULL;
    }
    
    memberlist_curr = channel->memberlist;
    
    return channel_get_next();
}

extern t_connection * channel_get_next(void)
{

    t_channelmember * member;
    
    if (memberlist_curr)
    {
        member = memberlist_curr;
        memberlist_curr = memberlist_curr->next;
        
        return member->connection;
    }
    return NULL;
}


extern t_list * channel_get_banlist(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_warn,"channel_get_banlist","got NULL channel");
	return NULL;
    }
    
    return channel->banlist;
}


extern char const * channel_get_shortname(t_channel const * channel)
{
    if (!channel)
    {
        eventlog(eventlog_level_warn,"channel_get_shortname","got NULL channel");
	return NULL;
    }
    
    return channel->shortname;
}


static int channellist_load_permanent(char const * filename)
{
    FILE *       fp;
    unsigned int line;
    unsigned int pos;
    int          botflag;
    int          operflag;
    int          logflag;
    unsigned int modflag;
    char *       buff;
    char *       name;
    char *       sname;
    char *       tag;
    char *       bot;
    char *       oper;
    char *       log;
    char *       country;
    char *       max;
    char *       moderated;
    char *       newname;
    char *       realmname;
    
    if (!filename)
    {
	eventlog(eventlog_level_error,"channellist_load_permanent","got NULL filename");
	return -1;
    }
    
    if (!(fp = fopen(filename,"r")))
    {
	eventlog(eventlog_level_error,"channellist_load_permanent","could not open channel file \"%s\" for reading (fopen: %s)",filename,strerror(errno));
	return -1;
    }
    
    for (line=1; (buff = file_get_line(fp)); line++)
    {
	if (buff[0]=='#' || buff[0]=='\0')
	{
	    free(buff);
	    continue;
	}
        pos = 0;
	if (!(name = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"missing name in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(sname = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"missing sname in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(tag = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"missing tag in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(bot = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"missing bot in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(oper = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"missing oper in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(log = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"missing log in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(country = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"missing country in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
        if (!(realmname = next_token(buff,&pos)))
        {
           eventlog(eventlog_level_error,__FUNCTION__,"missing realmname in line %u in file \"%s\"",line,filename);
           free(buff);
           continue;
        }
	if (!(max = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"missing max in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	if (!(moderated = next_token(buff,&pos)))
	{
	    eventlog(eventlog_level_error,__FUNCTION__,"missing mod in line %u in file \"%s\"",line,filename);
	    free(buff);
	    continue;
	}
	
	switch (str_get_bool(bot))
	{
	case 1:
	    botflag = 1;
	    break;
	case 0:
	    botflag = 0;
	    break;
	default:
	    eventlog(eventlog_level_error,"channellist_load_permanent","invalid boolean value \"%s\" for field 4 on line %u in file \"%s\"",bot,line,filename);
	    free(buff);
	    continue;
        }
	
	switch (str_get_bool(oper))
	{
	case 1:
	    operflag = 1;
	    break;
	case 0:
	    operflag = 0;
	    break;
	default:
	    eventlog(eventlog_level_error,"channellist_load_permanent","invalid boolean value \"%s\" for field 5 on line %u in file \"%s\"",oper,line,filename);
	    free(buff);
	    continue;
        }
	
	switch (str_get_bool(log))
	{
	case 1:
	    logflag = 1;
	    break;
	case 0:
	    logflag = 0;
	    break;
	default:
	    eventlog(eventlog_level_error,"channellist_load_permanent","invalid boolean value \"%s\" for field 5 on line %u in file \"%s\"",log,line,filename);
	    free(buff);
	    continue;
        }

	switch (str_get_bool(moderated))
	{
	    case 1:
		modflag = 1;
		break;
	    case 0:
		modflag = 0;
		break;
	    default:
		eventlog(eventlog_level_error,__FUNCTION__,"invalid boolean value \"%s\" for field 10 on line %u in file \"%s\"",moderated,line,filename);
		free(buff);
		continue;
	}
	
	if (strcmp(sname,"NULL") == 0)
	    sname = NULL;
	if (strcmp(tag,"NULL") == 0)
	    tag = NULL;
        if (strcmp(name,"NONE") == 0)
	    name = NULL;
        if (strcmp(country, "NULL") == 0)
            country = NULL;
        if (strcmp(realmname,"NULL") == 0)
            realmname = NULL;
	
	if (name)
	    {
            channel_create(name,sname,tag,1,botflag,operflag,logflag,country,realmname,atoi(max),modflag,0);
	    }
	else
	    {
            newname = channel_format_name(sname,country,realmname,1);
            if (newname)
		{
                   channel_create(newname,sname,tag,1,botflag,operflag,logflag,country,realmname,atoi(max),modflag,0);
                   free(newname);
	    }
            else
	    {
                   eventlog(eventlog_level_error,"channellist_load_permanent","cannot format channel name");
		}
            }
	
	/* FIXME: call channel_delete() on current perm channels and do a
	   channellist_find_channel() and set the long name, perm flag, etc,
	   otherwise call channel_create(). This will make HUPing the server
           handle re-reading this file correctly. */
	free(buff);
    }
    
    if (fclose(fp)<0)
	eventlog(eventlog_level_error,"channellist_load_permanent","could not close channel file \"%s\" after reading (fclose: %s)",filename,strerror(errno));
    return 0;
}

static char * channel_format_name(char const * sname, char const * country, char const * realmname, unsigned int id)
{
    char * fullname;
    unsigned int len;

    if (!sname)
    {
        eventlog(eventlog_level_error,"channel_format_name","got NULL sname");
        return NULL;
    }
    len = strlen(sname)+1; /* FIXME: check lengths and format */
    if (country) 
    	len = len + strlen(country) + 1;
    if (realmname)
    	len = len + strlen(realmname) + 1;
    len = len + 32 + 1;

    if (!(fullname=malloc(len)))
    {
        eventlog(eventlog_level_error,"channel_format_name","could not allocate memory for fullname");
        return NULL;
    }
    sprintf(fullname,"%s%s%s%s%s-%d",
            realmname?realmname:"",
            realmname?" ":"",
            sname,
            country?" ":"",
            country?country:"",
            id);
    return fullname;
}

extern int channellist_reload(void)
{
  t_elem * curr;
  t_channel * channel, * old_channel;
  t_channelmember * memberlist, * member, * old_member;
  t_list * channellist_old;

  if (channellist_head)
    {

      if (!(channellist_old = list_create()))
        return -1;

      /* First pass - get members */
      LIST_TRAVERSE(channellist_head,curr)
      {
	if (!(channel = elem_get_data(curr)))
	{
	  eventlog(eventlog_level_error,"channellist_reload","channel list contains NULL item");
	  continue;
	}
	/* Trick to avoid automatic channel destruction */
	channel->flags |= channel_flags_permanent;
	if (channel->memberlist)
	{
	  /* we need only channel name and memberlist */

	  old_channel = (t_channel *) malloc(sizeof(t_channel));
	  old_channel->shortname = strdup(channel->shortname);
	  old_channel->memberlist = NULL;
	  member = channel->memberlist;

	  /* First pass */
	  while (member)
	  {
	    if (!(old_member = malloc(sizeof(t_channelmember))))
	    {
	      /* FIX-ME: need free */
	      eventlog(eventlog_level_error,"channellist_reload","could not allocate memory for channelmember");
	      goto old_channel_destroy;
	    }

	    old_member->connection = member->connection;
	    
	    if (old_channel->memberlist)
	      old_member->next = old_channel->memberlist;
	    else
	      old_member->next = NULL;

	    old_channel->memberlist = old_member;
	    member = member->next;
	  }

	  /* Second pass - remove connections from channel */
	  member = old_channel->memberlist;
	  while (member)
	  {
	    channel_del_connection(channel,member->connection);
	    conn_set_channel_var(member->connection,NULL);
	    member = member->next;
	  }
	  
	  if (list_prepend_data(channellist_old,old_channel)<0)
	  {
	    eventlog(eventlog_level_error,"channellist_reload","error reloading channel list, destroying channellist_old");
	    memberlist = old_channel->memberlist;
	    while (memberlist)
	    {
	      member = memberlist;
	      memberlist = memberlist->next;
	      free((void*)member);
	    }
	    goto old_channel_destroy;
	  }

	}
	
	/* Channel is empty - Destroying it */
	channel->flags &= ~channel_flags_permanent;
	if (channel_destroy(channel)<0)
	  eventlog(eventlog_level_error,"channellist_reload","could not destroy channel");
	
      }

      /* Cleanup and reload */
      
      if (list_destroy(channellist_head)<0)
	return -1;

      channellist_head = NULL;
      channellist_create();
      
      /* Now put all users on their previous channel */
      
      LIST_TRAVERSE(channellist_old,curr)
      {
	if (!(channel = elem_get_data(curr)))
	{
	  eventlog(eventlog_level_error,"channellist_reload","old channel list contains NULL item");
	  continue;
	}

	memberlist = channel->memberlist;
	while (memberlist)
	{
	  member = memberlist;
	  memberlist = memberlist->next;
	  conn_set_channel(member->connection, channel->shortname);
	}
      }


      /* Ross don't blame me for this but this way the code is cleaner */
 
   old_channel_destroy:
      LIST_TRAVERSE(channellist_old,curr)
      {
	if (!(channel = elem_get_data(curr)))
	{
	  eventlog(eventlog_level_error,"channellist_reload","old channel list contains NULL item");
	  continue;
	}
	
	memberlist = channel->memberlist;
	while (memberlist)
	{
	  member = memberlist;
	  memberlist = memberlist->next;
	  free((void*)member);
	}

	if (channel->shortname)
	  free((void*)channel->shortname);

	if (list_remove_data(channellist_old,channel)<0)
	  eventlog(eventlog_level_error,"channellist_reload","could not remove item from list");
	free((void*)channel);

      }

      if (list_destroy(channellist_old)<0)
	return -1;
    }
  return 0;

}     

extern int channellist_create(void)
{
    if (!(channellist_head = list_create()))
	return -1;
    
    return channellist_load_permanent(prefs_get_channelfile());
}


extern int channellist_destroy(void)
{
    t_channel *    channel;
    t_elem const * curr;
    
    if (channellist_head)
    {
	LIST_TRAVERSE(channellist_head,curr)
	{
	    if (!(channel = elem_get_data(curr))) /* should not happen */
	    {
		eventlog(eventlog_level_error,"channellist_destroy","channel list contains NULL item");
		continue;
	    }
	    
	    channel_destroy(channel);
	}
	
	if (list_destroy(channellist_head)<0)
	    return -1;
	channellist_head = NULL;
    }
    
    return 0;
}


extern t_list * channellist(void)
{
    return channellist_head;
}


extern int channellist_get_length(void)
{
    return list_get_length(channellist_head);
}

extern int channel_get_max(t_channel const * channel)
{
  if (!channel)
  {
    eventlog(eventlog_level_error,__FUNCTION__,"got NULL channel");
    return 0;
  }

  return channel->maxmembers;
}

extern int channel_get_curr(t_channel const * channel)
{
  if (!channel)
  {
    eventlog(eventlog_level_error,__FUNCTION__,"got NULL channel");
    return 0;
  }
  
  return channel->currmembers;

}

extern int channel_account_is_tmpOP(t_channel const * channel, t_account * account)
{
	if (!channel)
	{
	  eventlog(eventlog_level_error,__FUNCTION__,"got NULL channel");
	  return 0;
	}

	if (!account)
	{
	  eventlog(eventlog_level_error,__FUNCTION__,"got NULL account");
	  return 0;
	}

	if (!account_get_tmpOP_channel(account)) return 0;
	
	if (strcmp(account_get_tmpOP_channel(account),channel_get_name(channel))==0) return 1;

	return 0;
}

extern int channel_account_has_tmpVOICE(t_channel const * channel, t_account * account)
{
	if (!channel)
	{
	  eventlog(eventlog_level_error,__FUNCTION__,"got NULL channel");
	  return 0;
	}

	if (!account)
	{
	  eventlog(eventlog_level_error,__FUNCTION__,"got NULL account");
	  return 0;
	}

	if (!account_get_tmpVOICE_channel(account)) return 0;

	if (strcmp(account_get_tmpVOICE_channel(account),channel_get_name(channel))==0) return 1;

	return 0;
}

static t_channel * channellist_find_channel_by_fullname(char const * name)
{
    t_channel *    channel;
    t_elem const * curr;
    
    if (channellist_head)
    {
	LIST_TRAVERSE(channellist_head,curr)
	{
	    channel = elem_get_data(curr);
	    if (!channel->name)
	    {
		eventlog(eventlog_level_error,"channellist_find_channel_by_fullname","found channel with NULL name");
		continue;
	    }

	    if (strcasecmp(channel->name,name)==0)
		return channel;
	}
    }
    
    return NULL;
}


/* Find a channel based on the name. 
 * Create a new channel if it is a permanent-type channel and all others
 * are full.
 */
extern t_channel * channellist_find_channel_by_name(char const * name, char const * country, char const * realmname)
{
    t_channel *    channel;
    t_elem const * curr;
    int            foundperm;
    int            maxchannel; /* the number of "rollover" channels that exist */
    char const *   saveshortname;
    char const *   savetag;
    int            savebotflag;
    int            saveoperflag;
    int            savelogflag;
    unsigned int   savemoderated;
    char const *   savecountry;
    char const *   saverealmname;
    int            savemaxmembers;

    // try to make gcc happy and initialize all variables
    saveshortname = savetag = savecountry = saverealmname = NULL;
    savebotflag = saveoperflag = savelogflag = savemaxmembers = savemoderated = 0;
    
    maxchannel = 0;
    foundperm = 0;
    if (channellist_head)
    {
	LIST_TRAVERSE(channellist_head,curr)
	{
	    channel = elem_get_data(curr);
	    if (!channel->name)
	    {
		eventlog(eventlog_level_error,"channellist_find_channel_by_name","found channel with NULL name");
		continue;
	    }

	    if (strcasecmp(channel->name,name)==0)
	    {
		eventlog(eventlog_level_debug,"channellist_find_channel_by_name","found exact match for \"%s\"",name);
		return channel;
	    }
	    
            if (channel->shortname && strcasecmp(channel->shortname,name)==0)
	    {
		/* FIXME: what should we do if the client doesn't have a country?  For now, just take the first
		 * channel that would otherwise match. */
                if ( (!channel->country || !country || 
		      (channel->country && country && (strcmp(channel->country, country)==0))) &&
	             ((!channel->realmname && !realmname) || 
		      (channel->realmname && realmname && (strcmp(channel->realmname, realmname)==0))) )

		{
		    if (channel->maxmembers==-1 || channel->currmembers<channel->maxmembers) 
		    {
			eventlog(eventlog_level_debug,"channellist_find_channel_by_name","found permanent channel \"%s\" for \"%s\"",channel->name,name);
			return channel;
		    }
		    maxchannel++;
		}
		else
		    eventlog(eventlog_level_debug,"channellist_find_channel_by_name","countries didn't match");
		
		foundperm = 1;
		
		/* save off some info in case we need to create a new copy */
		saveshortname = channel->shortname;
		savetag = channel->clienttag;
		savebotflag = channel->flags & channel_flags_allowbots;
		saveoperflag = channel->flags & channel_flags_allowopers;
		if (channel->logname)
		    savelogflag = 1;
                else
                    savelogflag = 0;
                if (country)
		    savecountry = country;
		else
		    savecountry = channel->country;
                if (realmname)
                    saverealmname = realmname;
                else
                    saverealmname = channel->realmname;
		savemaxmembers = channel->maxmembers;
		savemoderated = channel->flags & channel_flags_moderated;
	    } 
	}
    }
    
    /* we've gone thru the whole list and either there was no match or the
     * channels are all full.
     */

    if (foundperm) /* All the channels were full, create a new one */
    {
	char * channelname;
	
        if (!(channelname=channel_format_name(saveshortname,savecountry,saverealmname,maxchannel+1)))
                return NULL;

        channel = channel_create(channelname,saveshortname,savetag,1,savebotflag,saveoperflag,savelogflag,savecountry,saverealmname,savemaxmembers,savemoderated,0);
        free(channelname);
	
	eventlog(eventlog_level_debug,"channellist_find_channel_by_name","created copy \"%s\" of channel \"%s\"",(channel)?(channel->name):("<failed>"),name);
        return channel;
    }
    
    /* no match */
    eventlog(eventlog_level_debug,"channellist_find_channel_by_name","could not find channel \"%s\"",name);
    return NULL;
}


extern t_channel * channellist_find_channel_bychannelid(unsigned int channelid)
{
    t_channel *    channel;
    t_elem const * curr;
    
    if (channellist_head)
    {
	LIST_TRAVERSE(channellist_head,curr)
	{
	    channel = elem_get_data(curr);
	    if (!channel->name)
	    {
		eventlog(eventlog_level_error,"channellist_find_channel_bychannelid","found channel with NULL name");
		continue;
	    }
	    if (channel->id==channelid)
		return channel;
	}
    }
    
    return NULL;
}

extern int channel_set_flags(t_connection * c)
{
  unsigned int	newflags;
  char const *	channel;
  t_account  *	acc;
  
  if (!c) return -1; // user not connected, no need to update his flags
  
  acc = conn_get_account(c);
  
  /* well... unfortunatly channel_get_name never returns NULL but "" instead 
     so we first have to check if user is in a channel at all */
  if ((!conn_get_channel(c)) || (!(channel = channel_get_name(conn_get_channel(c)))))
    return -1;
  
  if (account_get_auth_admin(acc,channel) == 1 || account_get_auth_admin(acc,NULL) == 1)
    newflags = MF_BLIZZARD;
  else if (account_get_auth_operator(acc,channel) == 1 || 
	   account_get_auth_operator(acc,NULL) == 1)
    newflags = MF_BNET;
  else if (channel_account_is_tmpOP(conn_get_channel(c),acc))
    newflags = MF_GAVEL;
  else if ((account_get_auth_voice(acc,channel) == 1) ||
	   (channel_account_has_tmpVOICE(conn_get_channel(c),acc)))
    newflags = MF_VOICE;
  else
    if (strcmp(conn_get_clienttag(c), CLIENTTAG_WARCRAFT3) == 0 || 
	strcmp(conn_get_clienttag(c), CLIENTTAG_WAR3XP) == 0)
      newflags = W3_ICON_SET;
    else 
      newflags = 0;
  
  if (conn_get_flags(c) != newflags) {
    conn_set_flags(c, newflags);
    channel_update_flags(c);
  }
  
  return 0;
}
