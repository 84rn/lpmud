/*
 *
 * /d/Arkadia/obj/udp.c
 *
 */

#pragma no_clone
#pragma no_shadow
#pragma save_binary

inherit "/sys/global/udp";

/*
  And all the extra udp features
*/

#include <macros.h>
#include <mail.h>
#include <std.h>
#include <time.h>
#include <udp.h>

#define INTERMUD_INFO "/d/Standard/doc/intermud_info"
#define INTERMUD_ERR ("Hmm... You have just experienced a strange bug... " \
          + "You are kindly welcomed to mail us (arkadia@arkadia.rpg.pl) " \
          + "about it.\n")

string *color_codes = ({"BLACK", "BLUE", "CYAN", "GREEN", "MAGENTA", "RED",
                        "WHITE", "YELLOW", "BOLD", "FLASH", "RESET"});
string *denied_muds = ({});

void
create()
{
    ::create();

    setuid();
    seteuid(getuid());
}

string
short()
{
    return "big brother";
}

void 
incoming_udp(string host, string msg)
{
    string name, said_host, t;
    int p;

    /* For relayed packages we must extract the correct info
     */
#ifdef Genesis
    if (sscanf(msg,"RELAY(%s)%s", said_host, t) == 2)
    {
	::incoming_udp(said_host, t);
    }
    else
#endif
	::incoming_udp(host, msg);
}

public string
remove_color_codes(string str)
{
    string *words = explode(str, "%^")
                  + (wildmatch("*%^", str) ? ({""}) : ({}));
    int size = sizeof(words) - 1;
    int index = 0;

    while (++index < size)
        if (member_array(words[index], color_codes) != -1)
        {
            words = (index == 1 ? ({}) : words[..(index - 2)])
                  + ({words[index - 1] + words[index + 1]})
                  + words[(index + 2)..];
            index--;
            size -= 2;
        }

    if (size)
    {
        str = implode(words, "%^");
        log_file("UDP_color", str + "\n");
        return str;
    }

    return words[0];
}

public int
execute_udp_command(string cmd, mapping params)
{
    if (params["GWIZ"] && wildmatch("*%^*", params["GWIZ"]))
        params["GWIZ"] = remove_color_codes(params["GWIZ"]);

    if (params["MSG"] && wildmatch("*%^*", params["MSG"]))
        params["MSG"] = remove_color_codes(params["MSG"]);

    if (params["RWHO"] && wildmatch("*%^*", params["RWHO"]))
        params["RWHO"] = remove_color_codes(params["RWHO"]);

    ::execute_udp_command(cmd, params);
}

#if 0
int
startup(mapping p)
{
    string a,b;

    /* Just log if the name does not contain 'test'
     */
    if (sscanf(" " + p["NAME"]+ " ", "%stest%s", a, b) != 2)
	log_file("UDP_startups", sprintf("%-25s %s (%s:%s:%s)\n", 
					 p["NAME"], ctime(time()),
					 p["HOSTADDRESS"], p["PORT"],
					 p["PORTUDP"]), -1);
    return ::startup(p);
}
#endif 0

int 
gwizmsg(mapping p)
{
    if (member_array(p["NAME"], denied_muds) != -1)
    {
        log_file("UDP_deniedgwiz", ctime(time()) + ": " + p["WIZNAME"] + "@"
               + p["NAME"] + (p["EMOTE"] == "1" ? " " : ": ") + p["GWIZ"]
               + "\n");
	return 0;
    }

    if (wildmatch("*huj*", p["GWIZ"]))
        log_file("UDP_magyar", ctime(time()) + ": " + p["WIZNAME"] + "@"
               + p["NAME"] + (p["EMOTE"] == "1" ? " " : ": ") + p["GWIZ"]
               + "\n");

    switch(p["CHANNEL"] ?: "CREATOR")
    {
        case "CREATOR":
            break;
        case "HUNGARY":
            return 0;
        default:
            log_file("UDP_channel", ctime(time()) + ": <" + p["CHANNEL"]
                   + "> " + p["WIZNAME"] + "@" + p["NAME"]
                   + (p["EMOTE"] == "1" ? " " : ": ") + p["GWIZ"] + "\n");
            return 0;
    }

//    log_file("UDP_gwiz", ctime(time()) + ": " + p["WIZNAME"] + "@" + p["NAME"]
//           + (p["EMOTE"] == "1" ? " " : ": ") + p["GWIZ"] + "\n");

    return ::gwizmsg(p);
}

string *
send_gwizmsg(string wizname, string msg, int emote, string *dont_send = ({}))
{
//    log_file("UDP_gwiz", ctime(time()) + ": " + wizname + "@"
//           + SECURITY->get_mud_name() + (emote ? " " : ": ") + msg + "\n");

    return ::send_gwizmsg(wizname, msg, emote, dont_send + denied_muds);
}

public int
rwho_q(mapping p)
{
    if (stringp(p["NAME"]) && stringp(p["PORTUDP"]))
	log_file("UDP", sprintf("%s: RWHO by %s@%s (%s).\n",
        	ctime(time()), p["ASKWIZ"], p["NAME"], p["HOSTADDRESS"]));
        	
    return ::rwho_q(p);
}

public int
gfinger_q(mapping p)
{
    if (stringp(p["NAME"]) && stringp(p["PORTUDP"]) && stringp(p["ASKWIZ"]) && 
        stringp(p["PLAYER"]))
	log_file("UDP", sprintf("%s: GFINGER for %s by %s@%s (%s).\n",
        	ctime(time()), p["PLAYER"], p["ASKWIZ"], p["NAME"], 
        	p["HOSTADDRESS"]));

    return ::gfinger_q(p);
}

public int
warning(mapping p)
{
    log_file("UDP_warning", ctime(time()) + ": " + val2str(p) + "\n");

    return 1;
}

static string
list_mail_alias(string alias, string single, string multi = single + " team")
{
    string *list;
    int size;

    return IS_MAIL_ALIAS(alias) &&
           (size = sizeof(list = EXPAND_MAIL_ALIAS(alias))) ?
           (size == 1 ? single : multi) + ": "
         + implode(map(list, &capitalize()), ", ") + "\n" : "";
}

static string
convtime(int time)
{
    int *times = TIME2NUM(time);
    string *strings = ({});

    if (times[0])
        strings += times[0] == 1 ? ({"1 day"}) : ({times[0] + " days"});
    if (times[1])
        strings += times[1] == 1 ? ({"1 hour"}) : ({times[1] + " hours"});
    if (times[2])
        strings += times[2] == 1 ? ({"1 minute"}) : ({times[2] + " minutes"});
    if (times[3])
        strings += times[3] == 1 ? ({"1 second"}) : ({times[3] + " seconds"});

    return implode(strings, " ");
}

public int
sort_wizards(string wiz1, string wiz2)
{
    return wiz1 == wiz2 ? 0 :
               (SECURITY->query_wiz_rank(wiz2)
              - SECURITY->query_wiz_rank(wiz1) ?: (wiz1 > wiz2 ? 1 : -1));
}

/*
 *  Maskuje funkcje z /lib/udp/gfinger
 */
public string
finger_player(string name)
{
#ifdef Arkadia
    string msg;
    string *keepers;
    object player;
    int real;
    int rank;
    string pronoun;
    string domain;

    switch (name = lower_case(name))
    {
        case "admin":
            if ((msg = list_mail_alias("admin", "Administrator of Arkadia",
                    "Administrators of Arkadia")) == "")
                return INTERMUD_ERR;

            keepers = SECURITY->query_wiz_list(WIZ_KEEPER);

            return msg + "\n" + (sizeof(keepers) ? (sizeof(keepers) == 1 ?
                  "Keeper" : "Keepers") + " of Arkadia: "
                 + implode(map(keepers, &capitalize()), ", ") + "\n" : "")
                 + list_mail_alias("aob", "Arch of Balance")
                 + list_mail_alias("aog", "Arch of Driver")
                 + list_mail_alias("aom", "Arch of Mudlib")
                 + list_mail_alias("aop", "Arch of Players");
        case "info":
            if (file_size(INTERMUD_INFO) > 0)
                return read_file(INTERMUD_INFO);

            return INTERMUD_ERR;
    }

    if (player = find_player(name))
        real = 1;
    else if (!(player = SECURITY->finger_player(name)))
        return "There is no player named '" + capitalize(name)
             + "' on Arkadia.\n";
    else
        real = 0;

    if (!(rank = SECURITY->query_wiz_rank(name)))
        return capitalize(name) + " is a mortal player. "
             + "Should you need any other information about "
             + player->query_objective() + ", contact the "
             + "Arch of Players (see admin@Arkadia).\n";

    msg = capitalize(name) + (real ? " is currently" : " is not")
        + " in the game. Description:\n" + player->long()
        + (pronoun = capitalize(player->query_pronoun()))
        + " holds the rank of " + capitalize(WIZ_RANK_NAME(rank))
        + " (level " + SECURITY->query_wiz_level(name) + ").\n";

    if (strlen(domain = SECURITY->query_wiz_dom(name)))
    {
        string lord;

        msg += pronoun + " is "
             + (rank == WIZ_LORD ?
                (player->query_gender() == G_FEMALE ? "the lady" : "the lord")
              + " of domain " + domain :
                (rank == WIZ_STEWARD ? "the steward of domain " :
                 "a member of the domain ") + domain + " with "
              + (strlen(lord = SECURITY->query_domain_lord(domain)) ?
                 capitalize(lord) + " as a liege" : "no lord")) + ".\n";
    }

    if (real)
    {
        msg += pronoun + " is logged on for "
             + convtime(time() - player->query_login_time()) + " from "
             + player->query_login_from() + ".\n";

        if (query_ip_number(player))
        {
            if (query_idle(player))
                msg += "Idle time: " + convtime(query_idle(player)) + "\n";
        }
        else
            msg += pronoun + " is linkdead for "
                 + convtime(time() - player->query_linkdead()) + ".\n";
    }
    else
    {
        int chtime = SECURITY->query_player_file_time(name)
                   - player->query_login_time();

        msg += "Last login " + convtime(time() - player->query_login_time())
             + " ago from " + player->query_login_from() + ".\n";

        if (chtime < 86400)	/* 24 hours, guard against patched files */
            msg += "Duration of stay was " + convtime(chtime) + ".\n";
    }

    msg += "Age: " + convtime(player->query_age() * 2) + "\n"
         + "Email: " + player->query_mailaddr() + "\n";

    if (!real)
        player->remove_object();

    return msg;
#else
    return ::finger_player(name);
#endif
}

/*
 *  Maskuje funkcje z /lib/udp/rwho.
 */
public string
rwho_message()
{
#ifdef Arkadia
    object *players = users();
    object *wizards = filter(players, &->query_wiz_level());
    int w = sizeof(wizards);
    int m = sizeof(players) - w;

    return "Arkadia LPMud - driver: " + SECURITY->do_debug("version")
         + ", mudlib: " + MUDLIB_VERSION + "\n"
         + "Additional information: gfinger info@Arkadia, admin@Arkadia\n\n"
         + "Uptime: " + convtime(time() - SECURITY->query_start_time()) + "\n"
         + "Local time: " + efun::ctime(time()) + "\n"
         + capitalize(LANG_WNUM(w)) + (w == 1 ? " wizard" : " wizards")
         + (w || m ? " and " + LANG_WNUM(m) : " nor") + " mortal"
         + (m == 1 ? " player" : " players") + " logged on.\n"
         + (w ? (w == 1 ? "Wizard: " : "Wizards: ")
             + implode(map(sort_array(wizards->query_real_name(PL_MIA),
                   "sort_wizards"), capitalize), ", ") + "\n" : "");
#else
    return ::rwho_message();
#endif
}
