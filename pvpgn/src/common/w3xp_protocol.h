#ifndef __W3XP_PROTOCOL_H__
#define __W3XP_PROTOCOL_H__

#include "common/bnet_protocol.h"

/*
0000  ff 72 3a 00 00 00 00 00 - 36 38 58 49 50 58 33 57   .r:.....68XIPX3W
0010  2d 01 00 00 53 55 6e 65 - 43 08 45 1f 2c 01 00 00   -...SUneC.E.,...
0020  09 04 00 00 09 04 00 00 - 55 53 41 00 55 6e 69 74   ........USA.Unit
0030  65 64 20 53 74 61 74 65 - 73 00                     ed States.

0000:   FF 72 34 00 00 00 00 00   36 38 58 49 50 58 33 57    .r4.....68XIPX3W
0010:   2D 01 00 00 53 55 6E 65   C0 A8 00 03 C4 FF FF FF    -...SUne........
0020:   07 04 00 00 07 04 00 00   44 45 55 00 47 65 72 6D    ........DEU.Germ
0030:   61 6E 79 00                                          any.            

*/
#define CLIENT_W3XP_COUNTRYINFO 0x72ff
typedef struct
{
	t_bnet_header	h;
	bn_int			unknown1; /* 00 00 00 00 */
	bn_int			arch_tag;
	bn_int			client_tag;
	bn_int			versionid;
	bn_int			client_lang; /* 53 55 6e 65 */
	bn_int			unknown2; /* 43 08 45 1f */
	bn_int			bias; /* (gmt-local)/60 (signed math) */
	bn_int			lcid; /* 09 04 00 00 */
	bn_int			langid; /* 09 04 00 00 */
    /* langstr */
    /* countryname */
} t_client_w3xp_countryinfo PACKED_ATTR();
#define CLIENT_W3XP_COUNTRYINFO_UNKNOWN1			0x1F450843
#define CLIENT_W3XP_COUNTRYINFO_LANGID_USENGLISH		0x00000409	
#define CLIENT_W3XP_COUNTRYINFO_COUNTRYNAME_USA			"United States"

/*
0000:   FF 72 E3 00 02 00 00 00   CF 31 65 20 42 F6 00 00    .r.......1e B...
0010:   00 3C 5B A5 63 E8 C0 01   49 58 38 36 76 65 72 34    .<[.c...IX86ver4
0020:   2E 6D 70 71 00 41 3D 34   35 32 35 37 34 32 38 30    .mpq.A=452574280
0030:   20 42 3D 32 36 36 36 34   31 39 38 30 20 43 3D 38     B=266641980 C=8
0040:   31 37 31 32 34 39 38 30   20 34 20 41 3D 41 2D 53    17124980 4 A=A-S
0050:   20 42 3D 42 2B 43 20 43   3D 43 2D 41 20 41 3D 41     B=B+C C=C-A A=A
0060:   5E 42 00 A6 26 38 2C 6B   30 50 16 F0 DF CB 79 BB    ^B..&8,k0P....y.
0070:   A2 AC 81 DA 87 2B 46 6D   56 5B 61 55 12 F1 B1 46    .....+FmV[aU...F
0080:   91 D7 4A 5C E8 30 0F B7   54 85 A4 75 D3 A1 C2 69    ..J\.0..T..u...i
0090:   09 A4 F2 E1 E7 3B 3F 79   F8 71 A2 20 72 13 6C 9E    .....;?y.q. r.l.
00A0:   3A F5 D8 9E 52 C8 30 ED   37 AF 78 6F 54 CC E2 46    :...R.0.7.xoT..F
00B0:   A6 19 7E AE 3F FF BE 45   E8 CC 74 3A 47 0E 99 B9    ..~.?..E..t:G...
00C0:   0E A7 0A F1 DD 0B CA 64   BC 6A 92 C8 EE 36 30 84    .......d.j...60.
00D0:   80 78 4B 2E F0 BC 91 DD   5F CB DF AD 9C E4 C9 FE    .xK....._.......
00E0:   C8 9F 58                                             ..X             
*/

#define SERVER_W3XP_AUTHREQ 0x72ff
typedef struct
{
    t_bnet_header h;
    bn_int        unknown1;  /* 02 00 00 00 */
    bn_int        sessionkey;
    bn_int        sessionnum;
    bn_long       timestamp;
    /* versioncheck filename */                                                 
    /* equation */
} t_server_w3xp_authreq PACKED_ATTR();

/*

0000:   FF 04 88 00 4F 5A 08 00   00 07 00 01 11 8B 65 9C    ....OZ........e.
0010:   02 00 00 00 00 00 00 00   1A 00 00 00 0E 00 00 00    ................
0020:   B0 D7 0D 00 00 00 00 00   47 8A 59 8F 80 E1 EA 40    ........G.Y....@
0030:   F1 31 2D 61 B4 32 47 05   EA 0E 8C 69 1A 00 00 00    .1-a.2G....i....
0040:   11 00 00 00 7F 30 00 00   00 00 00 00 6C D0 01 B3    .....0......l...
0050:   32 52 7F 9F 22 D7 57 57   6B E8 4A 28 88 65 51 02    2R..".WWk.J(.eQ.
0060:   77 61 72 33 2E 65 78 65   20 30 33 2F 30 34 2F 30    war3.exe 03/04/0
0070:   33 20 32 33 3A 31 30 3A   32 34 20 31 32 31 31 38    3 23:10:24 12118
0080:   35 39 00 4E 34 6D 65 00                              59.N4me.        

*/

#define CLIENT_W3XP_AUTHREQ	0x04ff
typedef struct
{
    t_bnet_header h;
    bn_int        ticks;
    bn_int        gameversion;
    bn_int        checksum;
    bn_int        cdkey_number; /* count of cdkeys, d2 = 1, lod = 2 */
    bn_int        unknown1; /* 00 00 00 00 */
    /* cdkey info(s) */
    /* executable info */
    /* cdkey owner */
} t_client_w3xp_authreq PACKED_ATTR();

/*
0000:   52 94 AB CB 52 94 AB CB   52 94 AB CB                R...R...R...    

*/

#define SERVER_W3XP_AUTHREPLY	0x04ff
typedef struct
{
    t_bnet_header	h;
    bn_int		message; /* looks like a null terminated string */
} t_server_w3xp_authreply PACKED_ATTR();

/*
 0000  ff 08 08 00 d6 a0 1c cc -                           ........

time 547397     size 8  type: 0x08FF    from server
0000:   FF 08 08 00 F5 28 5D D2                              .....(].
---
time 547397     size 8  type: 0x08FF    to server
0000:   FF 08 08 00 F5 28 5D D2                              .....(].        

*/
#define SERVER_W3XP_ECHOREQ	0x08ff
typedef struct
{
    t_bnet_header	h;
    bn_int		ticks;
} t_server_w3xp_echoreq  PACKED_ATTR();

#define CLIENT_W3XP_ECHOREPLY	0x08ff
typedef struct
{
    t_bnet_header	h;
    bn_int		ticks;
} t_client_w3xp_echoreply  PACKED_ATTR();

/*

0000:   FF 73 24 00 01 00 00 00   00 00 00 00 74 65 72 6D    .s$.........term
0010:   73 6F 66 73 65 72 76 69   63 65 2D 65 6E 55 53 2E    sofservice-enUS.
0020:   74 78 74 00                                          txt.            

0000:   FF 73 20 00 02 00 00 00   00 00 00 00 6E 65 77 61    .s .........newa
0010:   63 63 6F 75 6E 74 2D 65   6E 55 53 2E 74 78 74 00    ccount-enUS.txt.

0000:   FF 73 23 00 03 00 00 00   00 00 00 00 63 68 61 74    .s#.........chat
0010:   68 65 6C 70 2D 77 61 72   33 2D 65 6E 55 53 2E 74    help-war3-enUS.t
0020:   78 74 00                                             xt.             

0000:   FF 73 1E 00 05 00 00 00   00 00 00 00 62 6E 73 65    .s..........bnse
0010:   72 76 65 72 2D 57 41 52   33 2E 69 6E 69 00          rver-WAR3.ini.  

*/

#define CLIENT_W3XP_FILEINFOREQ 0x73ff
typedef struct
{
    t_bnet_header h;
    bn_int        type;     /* type of file (TOS,icons,etc.) */
    bn_int        unknown1; /* 00 00 00 00 */ /* always zero? */
    /* filename */          /* default/suggested filename? */
} t_client_w3xp_fileinforeq PACKED_ATTR();
#define CLIENT_W3XP_FILEINFOREQ_TYPE_TOS	0x00000001 /* Terms of Service enUS */
#define CLIENT_W3XP_FILEINFOREQ_TYPE_NWACCT	0x00000002 /* New Account File */
#define CLIENT_W3XP_FILEINFOREQ_TYPE_CHTHLP	0x00000003 /* Chat Help */
#define CLIENT_W3XP_FILEINFOREQ_TYPE_SRVLST	0x00000005 /* Server List */
#define CLIENT_W3XP_FILEINFOREQ_UNKNOWN1	0x00000000
#define CLIENT_W3XP_FILEINFOREQ__FILE_TOSUSA	"termsofservice-enUS.txt"
#define CLIENT_W3XP_FILEINFOREQ__FILE_NWACCT	"newaccount-enUS.txt"
#define CLIENT_W3XP_FILEINFOREQ__FILE_CHTHLP	"chathelp-war3-enUS.txt"
#define CLIENT_W3XP_FILEINFOREQ__FILE_SRVLST	"bnserver-WAR3.ini"

/*

0000:   FF 73 2C 00 01 00 00 00   00 00 00 00 00 5F 0E D6    .s,.........._..
0010:   7C DF C2 01 74 65 72 6D   73 6F 66 73 65 72 76 69    |...termsofservi
0020:   63 65 2D 65 6E 55 53 2E   74 78 74 00                ce-enUS.txt.    

0000:   FF 73 28 00 02 00 00 00   00 00 00 00 00 72 F4 D2    .s(..........r..
0010:   54 21 C2 01 6E 65 77 61   63 63 6F 75 6E 74 2D 65    T!..newaccount-e
0020:   6E 55 53 2E 74 78 74 00                              nUS.txt.        

0000:   FF 73 2B 00 03 00 00 00   00 00 00 00 00 D9 C1 51    .s+............Q
0010:   88 DF C2 01 63 68 61 74   68 65 6C 70 2D 77 61 72    ....chathelp-war
0020:   33 2D 65 6E 55 53 2E 74   78 74 00                   3-enUS.txt.     

0000:   FF 73 26 00 05 00 00 00   00 00 00 00 00 B4 0E F1    .s&.............
0010:   07 0D C2 01 62 6E 73 65   72 76 65 72 2D 57 41 52    ....bnserver-WAR
0020:   33 2E 69 6E 69 00                                    3.ini.          

*/

#define SERVER_W3XP_FILEINFOREPLY	0x73ff
typedef struct
{
    t_bnet_header	h;
    bn_int		type;		/* type of file (TOS,icons,etc.) */
    bn_int		unknown2;	/* 00 00 00 00 */ /* same as in TOSREQ */
    bn_long		timestamp;	/* file modification time */
    /* filename */
} t_server_w3xp_fileinforeply PACKED_ATTR();

/*
0000:   FF 45 04 00                                          .E..            
*/
#define CLIENT_W3XP_ICONREQ	0x45ff
typedef struct {
    t_bnet_header	h;
} t_client_w3xp_iconreq PACKED_ATTR();

/*
0000:   FF 45 1B 00 00 7F D7 3C   69 B7 C2 01 69 63 6F 6E    .E.....<i...icon
0010:   73 2D 57 41 52 33 2E 62   6E 69 00                   s-WAR3.bni.     
*/
#define SERVER_W3XP_ICONREPLY	0x45ff
typedef struct {
    t_bnet_header	h;
    bn_long		timestamp; /* file modification time? */ 
    /* filename */
} t_server_war3xp_iconreply PACKED_ATTR();

/*
0000:   FF 43 2B 00 98 FF 51 49   A4 47 27 70 D5 23 A2 ED    .C+...QI.G'p.#..
0010:   21 FC 46 A1 39 52 48 1D   F4 AF 90 CA 2F C5 F2 6B    !.F.9RH...../..k
0020:   D1 CC 92 C4 66 6F 6F 62   61 72 00                   ....user.     
*/
#define CLIENT_W3XP_LOGINREQ 0x43ff
typedef struct
{
    t_bnet_header h;
    bn_byte        unknown[32];
    /* player name */
} t_client_w3xp_loginreq PACKED_ATTR();

/*
0000:   FF 43 48 00 00 00 00 00   46 A2 67 31 D2 54 A9 1F    .CH.....F.g1.T..
0010:   91 F0 F6 AB F1 64 43 5A   38 C8 83 1C 99 AC E9 A4    .....dCZ8.......
0020:   8B 5F F3 89 2C 21 97 F3   21 AC 38 AB F0 20 26 83    ._..,!..!.8.. &.
0030:   20 3A 41 CF 02 58 8D B3   CC 37 86 65 88 57 69 EC     :A..X...7.e.Wi.
0040:   29 91 68 37 DA 14 B6 07                              ).h7....        
*/
#define SERVER_W3XP_LOGINREPLY	0x43ff
typedef struct
{
    t_bnet_header	h;
    bn_int		message; /* 0 */
    bn_int		unknown[16];
} t_server_w3xp_loginreply PACKED_ATTR();

/*
0000:   FF 12 18 00 2A 4D CA 1C   A3 3E A8 78 53 0A 0F 31    ....*M...>.xS..1
0010:   C0 A7 67 6E D2 34 91 8E                              ..gn.4..        
*/
#define CLIENT_W3XP_LOGONPROOFREQ 0x12ff
typedef struct
{
    t_bnet_header h;
    bn_int        password_hash1[5];
} t_client_w3xp_logonproofreq PACKED_ATTR();

/*
0000:   FF 12 1C 00 00 00 00 00   81 FE FC 0F B6 1B 1F 39    ...............9
0010:   78 A5 7F D8 97 BE 11 26   CE A9 07 57                x......&...W    
*/
#define SERVER_W3XP_LOGONPROOFREPLY	0x12ff
typedef struct
{
    t_bnet_header	h;
    bn_int		response;
    bn_int		unknown1;
    bn_short		port0;
    bn_int		unknown2;
    bn_short		port1;
    bn_int		unknown3;
    bn_int		unknown4;
} t_server_w3xp_logonproofreply PACKED_ATTR();

/*
0000:   FF 5A 06 00 E0 17                                    .Z....          
*/
#define CLIENT_W3XP_CHANGEGAMEPORT	0x5aff
typedef struct
{
    t_bnet_header	h;
    bn_short		port;
} t_client_w3xp_changegameport PACKED_ATTR();

/*
0000:   FF 7F 06 00 00 00                                    ......          
*/
#define CLIENT_W3XP_PLAYERINFOREQ	0x7fff
typedef struct
{
    t_bnet_header	h;
    /* player name */
    /* player info */
} t_client_w3xp_playerinforeq PACKED_ATTR();

/*
0000:   FF 7F 19 00 66 6F 6F 62   61 72 00 50 58 33 57 20    ....foobar.PX3W 
0010:   30 00 66 6F 6F 62 61 72   00                         0.foobar.       
*/
#define SERVER_W3XP_PLAYERINFOREPLY	0x7fff
typedef struct
{
    t_bnet_header	h;
    /* player name */
    /* status */
} t_server_w3xp_playerinforeply PACKED_ATTR();

/*
0000:   FF 36 08 00 80 A6 5E 3E                              .6....^>        
*/
#define CLIENT_W3XP_MOTD		0x36ff
typedef struct
{
    t_bnet_header	h;
    bn_int		last_news_time; // date of the last news item the client has 
} t_client_w3xp_motd PACKED_ATTR();

/*
0000:   FF 36 DB 00 01 48 BF 69   3E 80 A6 5E 3E 80 A6 5E    .6...H.i>..^>..^
0010:   3E 00 00 00 00 57 65 6C   63 6F 6D 65 20 74 6F 20    >....Welcome to 
0020:   42 61 74 74 6C 65 2E 6E   65 74 21 0A 54 68 69 73    Battle.net!.This
0030:   20 73 65 72 76 65 72 20   69 73 20 68 6F 73 74 65     server is hoste
0040:   64 20 62 79 20 41 54 26   54 2E 0A 54 68 65 72 65    d by AT&T..There
0050:   20 61 72 65 20 63 75 72   72 65 6E 74 6C 79 20 34     are currently 4
0060:   38 36 20 75 73 65 72 73   20 69 6E 20 57 61 72 63    86 users in Warc
0070:   72 61 66 74 20 49 49 49   20 54 68 65 20 46 72 6F    raft III The Fro
0080:   7A 65 6E 20 54 68 72 6F   6E 65 2C 20 61 6E 64 20    zen Throne, and 
0090:   34 38 36 20 75 73 65 72   73 20 70 6C 61 79 69 6E    486 users playin
00A0:   67 20 38 34 20 67 61 6D   65 73 20 6F 6E 20 42 61    g 84 games on Ba
00B0:   74 74 6C 65 2E 6E 65 74   2E 0A 4C 61 73 74 20 6C    ttle.net..Last l
00C0:   6F 67 6F 6E 3A 20 53 61   74 20 4D 61 72 20 38 20    ogon: Sat Mar 8 
00D0:   20 31 31 3A 30 30 20 41   4D 0A 00                    11:00 AM..     
*/
#define SERVER_W3XP_MOTD	0x36ff
typedef struct
{
    t_bnet_header	h;
    bn_byte		msgtype;
    bn_int		currenttime;
    bn_int		first_news_time;
    bn_int		timestamp;
    bn_int		timestamp2; /* equal with timestamp except last packet */
    /* text */
} t_server_w3xp_motd PACKED_ATTR();

/*
0000:   FF 71 0B 00 01 00 00 00   57 33 00                   .q......W3.     
*/
#define CLIENT_W3XP_JOINCHANNEL	0x71ff
typedef struct
{
    t_bnet_header h;
    bn_int        channelflag;
} t_client_w3xp_joinchannel PACKED_ATTR();

/*
0000:   FF 59 39 00 07 00 00 00   21 00 00 00 BC 00 00 00    .Y9.....!.......
0010:   00 00 00 00 0D F0 AD BA   0D F0 AD BA 77 23 66 6F    ............w#fo
0020:   6F 62 61 72 00 46 72 6F   7A 65 6E 20 54 68 72 6F    obar.Frozen Thro
0030:   6E 65 20 44 45 55 2D 31   00                         ne DEU-1.       
*/
#define SERVER_W3XP_MESSAGE	0x59ff
typedef struct
{
    t_bnet_header h;
    bn_int        type;
    bn_int        flags;     /* player flags (or channel flags for MT_CHANNEL) */
    bn_int        latency;
    bn_int        unknown1;  /* always zero? */
    bn_int        player_ip; /* player's IP (big endian), no longer used, always 0D F0 AD BA */
    bn_int        unknown3;  /* server ip and/or reg auth? CD key and/or account number? */
  /* player name */
  /* text */
} t_server_w3xp_message PACKED_ATTR();

/*
0000:   FF 16 47 00 01 00 00 00   03 00 00 00 01 00 00 00    ..G.............
0010:   42 69 5A 61 52 52 65 00   70 72 6F 66 69 6C 65 5C    BiZaRRe.profile\
0020:   6C 6F 63 61 74 69 6F 6E   00 70 72 6F 66 69 6C 65    location.profile
0030:   5C 64 65 73 63 72 69 70   74 69 6F 6E 00 63 6C 61    \description.cla
0040:   6E 5C 6E 61 6D 65 00                                 n\name.         
*/
#define CLIENT_W3XP_STATSREQ	0x16ff
typedef struct
{
    t_bnet_header h;
    bn_int        name_count;
    bn_int        key_count;
    bn_int        unknown1; /* 78 52 82 02 */
    /* player name */
    /* field key ... */
} t_client_w3xp_statsreq PACKED_ATTR();

/*
0000:   FF 16 34 00 01 00 00 00   03 00 00 00 01 00 00 00    ..4.............
0010:   75 64 20 3D 20 49 4D 42   41 21 21 21 21 31 32 00    ud = IMBA!!!!12.
0020:   31 20 65 72 72 6F 72 20   69 6E 20 73 6F 6C 6F 20    1 error in solo 
0030:   3D 5B 00 00                                          =[..            
*/
#define SERVER_W3XP_STATSREPLY	0x16ff
typedef struct
{
    t_bnet_header h;
    bn_int        name_count;
    bn_int        key_count;
    bn_int        unknown1; /* 78 52 82 02 */ /* EE E4 84 03 */ /* same as request */
    /* field values ... */
} t_server_w3xp_statsreply PACKED_ATTR();

/*
0000:   FF 34 15 00 04 01 00 00   00 42 69 5A 61 52 52 65    .4.......BiZaRRe
0010:   00 50 58 33 57                                       .PX3W           
*/
#define CLIENT_W3XP_PROFILEREQ	0x34ff
typedef struct   
{   
    t_bnet_header h;   
    bn_byte     option;    
    bn_int          count;
    // USERNAME TO LOOKUP //
    // CLIENT TAG //
} t_client_wa3xp_profilereq PACKED_ATTR();


/*
*/
typedef struct
{
    t_bnet_header h; //header
    bn_byte option; // in this case it will be 0x04 (for profile)
    bn_int count; // count that goes up each time user clicks on someones profile
    bn_int unknown1; /* added in 1.03 seems to differ 0x63726165 */
    bn_byte rescount;
// REST OF PROFILE STATS - THIS WILL BE SET IN HANDLE_BNET.C after
// SERVER LOOKS UP THE USER ACCOUNT
} t_server_w3xp_profilereply PACKED_ATTR();

#define CLIENT_W3XP_CREATEACCOUNT	0x07ff
#define SERVER_W3XP_CREATEACCOUNT	0x07ff

/*
0000:   FF 68 17 00 00 00 00 00   00 00 00 00 00 00 00 00    .h..............
0010:   14 00 00 00 00 00 00                                 .......         
*/
#define CLIENT_W3XP_GAMELISTREQ		0x68FF

/*
0000:   FF 68 0C 00 00 00 00 00   01 00 00 00                .h..........    

0000:   FF 68 30 02 04 00 00 00   01 00 00 00 09 04 00 00    .h0.............
0010:   02 00 17 E0 3F CC 76 DD   00 00 00 00 00 00 00 00    ....?.v.........
0020:   10 00 00 00 2D 00 00 00   68 65 72 6F 20 61 72 65    ....-...hero are
0030:   6E 61 00 00 62 31 30 30   30 30 30 30 30 01 03 49    na..b10000000..I
0040:   07 01 01 75 01 F1 75 01   79 A1 23 AB 4D 8B 61 71    ...u..u.y.#.M.aq
0050:   73 5D 47 73 6F 85 7B 65   6F 55 69 73 6F A5 6F 65    s]Gso.{eoUiso.oe
0060:   5D 43 65 75 61 A9 5D 49   65 73 6F 21 41 95 73 65    ]Ceua.]Ieso!A.se
0070:   6F 61 21 77 31 E1 2F 31   33 2F 77 33 6D 4D 01 53    oa!w1./13/w3mM.S
0080:   75 73 67 65 73 09 73 6F   37 39 01 01 00 01 00 00    usges.so79......
0090:   00 09 04 00 00 02 00 17   E0 42 45 46 9D 00 00 00    .........BEF....
00A0:   00 00 00 00 00 10 00 00   00 3E 00 00 00 55 6C 74    .........>...Ult
00B0:   69 6D 61 74 65 20 48 65   72 6F 20 41 72 65 6E 61    imate Hero Arena
00C0:   21 00 00 62 34 30 30 30   30 30 30 30 01 03 49 07    !..b40000000..I.
00D0:   01 01 B5 01 91 95 01 5B   A3 39 85 4D 8B 61 71 73    .......[.9.M.aqs
00E0:   5D 47 73 6F 85 7B 65 6F   55 69 73 6F A5 6F 65 5D    ]Gso.{eoUiso.oe]
00F0:   43 65 75 61 35 5D 41 6F   67 65 6D 21 AB 41 73 65    Ceua5]Aogem!.Ase
0100:   6F 61 21 55 B9 4D 55 49   4D 41 55 45 89 21 77 31    oa!U.MUIMAUE.!w1
0110:   2F 31 2F 77 57 33 6D 01   41 73 63 69 CF 69 4D 6F    /1/wW3m.Asci.iMo
0120:   6F 65 5D 65 03 5B 01 01   00 01 00 00 00 04 08 00    oe]e.[..........
0130:   00 02 00 17 E0 DA 3A F0   23 00 00 00 00 00 00 00    ......:.#.......
0140:   00 10 00 00 00 2A 00 00   00 48 45 52 4F 20 50 4B    .....*...HERO PK
0150:   00 00 62 62 30 30 30 30   30 30 30 01 03 49 07 01    ..bb0000000..I..
0160:   01 75 01 F1 75 01 79 A1   23 AB 4D 8B 61 71 73 5D    .u..u.y.#.M.aqs]
0170:   47 73 6F 85 7B 65 6F 55   69 73 6F A5 6F 65 5D 43    Gso.{eoUiso.oe]C
0180:   65 75 61 A9 5D 49 65 73   6F 21 41 95 73 65 6F 61    eua.]Ieso!A.seoa
0190:   21 77 31 61 2F 31 33 2F   77 33 79 89 01 63 65 6F    !w1a/13/w3y..ceo
01A0:   2F 71 6F 03 77 01 01 00   09 00 00 00 09 04 00 00    /qo.w...........
01B0:   02 00 17 E0 44 07 84 40   00 00 00 00 00 00 00 00    ....D..@........
01C0:   10 00 00 00 09 00 00 00   4A 75 73 74 20 50 6C 61    ........Just Pla
01D0:   79 2E 2E 2E 2E 2E 2E 00   00 33 34 30 30 30 30 30    y........3400000
01E0:   30 30 01 03 49 07 01 01   75 01 E9 75 01 59 A1 EB    00..I...u..u.Y..
01F0:   FF 4D 8B 61 71 73 5D 47   73 6F 85 7B 65 6F 55 69    .M.aqs]Gso.{eoUi
0200:   73 6F A5 6F 65 5D 43 65   75 61 D1 5D 29 35 29 49    so.oe]Ceua.])5)I
0210:   61 69 55 6D 53 75 6F 6F   65 2F C7 77 33 79 01 43    aiUmSuooe/.w3y.C
0220:   69 67 EB 5F 43 61 65 5F   57 6F 01 6D 67 01 01 00    ig._Cae_Wo.mg...
*/
#define SERVER_W3XP_GAMELISTREPLY	0x68FF

#define CLIENT_W3XP_STARTGAME		0x79ff

/*
0000:   FF 4A 1D 00 00 00 00 00   2D 01 00 00 4A 75 73 74    .J......-...Just
0010:   20 50 6C 61 79 2E 2E 2E   2E 2E 2E 00 00              Play........   
*/
#define CLIENT_W3XP_JOIN_GAME		0x4AFF

/*
*/
// #define CLIENT_W3XP_GAMERESULT		0x37ff

//#define CLIENT_W3XP_CLOSEGAME		0x67FF

#define CLIENT_W3XP_MESSAGE		0x0aff

#endif
