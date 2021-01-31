/*
 * /cmd/wiz/apprentice/people.c
 *
 * This is a sub-part of /cmd/wiz/apprentice.c
 *
 * This file contains the commands that give you information about people
 * that are logged on.
 *
 * Commands currently included:
 *
 * - guildtell
 * - mpeople
 * - people
 */

#include "/config/login/login.h"
#include <filepath.h>
#include <files.h>
#include <options.h>

/* All options that don't add something, but are used to select people
 * or sort the people.
 */
#define P_EMPTY         ({ " ", "-", "/", "L", "A", "m", "w", "h", "H", "f" })

#define P_NAME		0
#define P_AGE		1
#define P_BUSY		2
#define P_DOMAIN	3
#define P_IDLE		4
#define P_LOCATION	5
#define P_IPNR		6
#define P_IPNAME	7
#define P_RTITLE	8
#define P_RLEVEL	9
#define P_SNOOP		10
#define P_SPECIES	11
#define P_MET		12
#define P_AVERAGE	13
#define P_DESC		14
#define P_DECL		15	/* Odmiana imienia gracza */
#define P_NUM		16

#define P_LEN_DOMAIN    3
#define P_LEN_SPECIES   3
#define P_LEN_AVERAGE   3
#define P_LEN_RANK      10

#define P_DOMAIN_ROOT	"/d/"

/* **************************************************************************
 * Global variables
 */
private mapping processed_ip_names;

/* **************************************************************************
 * Prototype
 */
nomask int in_guild(object player, string guild);

/* **************************************************************************
 * guildtell - tell all members of a guild something
 */
nomask int
guildtell(string str)
{
    string shadow_name;
    string guild;
    object *players;
    int    index;
    int    size;

    if (!stringp(str) ||
	(sscanf(str, "%s %s", guild, str) != 2))
    {
	notify_fail("Syntax: guildtell <guild> <message>\n");
	return 0;
    }

    shadow_name = SECURITY->query_guild_long_name(guild);
    if (!strlen(shadow_name))
    {
	notify_fail("No guild '" + guild + "' listed.\n");
	return 0;
    }

    players = users();
    index = size = sizeof(players);
    while (--index >= 0)
    {
	if (players[index]->query_guild_name_race() == shadow_name)
	    players[index]->guildtell_msg_race(str);
	else if (players[index]->query_guild_name_occ() == shadow_name)
	    players[index]->guildtell_msg_occ(str);
	else if (players[index]->query_guild_name_lay() == shadow_name)
	    players[index]->guildtell_msg_lay(str);
	else
	    size--;
    }
    
    if (!size)
    {
	notify_fail(sprintf("No players of the guild '%s' present.\n",
	    shadow_name));
	return 0;
    }

#if 0
    players = filter(users(), &in_guild(, shadow_name));
    if (!(size = sizeof(players)))
    {
	notify_fail("No players of the guild '" + shadow_name +
	    "' present.\n");
	return 0;
    }

    index = -1;
    players->guildtell_msg(str);

#endif
/*
    while(++index < size)
    {
	tell_object(players[index], "Without a sound, you hear a message " +
	    "from " + (players[index]->query_met(this_player()) ?
	    this_player()->query_name() :
	    LANG_ADDART(this_player()->query_nonmet_name())) +
	    " to all members of your '" + shadow_name +
	    "'.\nThe voice tells you: " + str + "\n");
    }
*/

    if (this_player()->query_option(OPT_ECHO))
    {
	write("Told all members of '" + shadow_name + "': " + str + "\n");
    }
    else
    {
	write("Ok.\n");
    }
    return 1;
}

/* **************************************************************************
 * people - print information about players
 */

/* **************************************************************************
 * First follow quite some sort-functions. They are all separated since they
 * are called quite a lot. Each check that has to be made in these functions
 * costs time, therefore we sacrifice little memory to keep all these
 * functions to gain a lot in execution time! 
 */

nomask int
sort_name(object a, object b)
{
    string sa = a->query_real_name();
    string sb = b->query_real_name();

    if (sa < sb)
	return -1;
    if (sa == sb)
	return 0;
    return 1;
}

nomask int
sort_name_and_level(object a, object b)
{
    int ia = SECURITY->query_wiz_rank(a->query_real_name());
    int ib = SECURITY->query_wiz_rank(b->query_real_name());

    /* most likely check first to save cpu-time. */
    if (ia == ib)
	return sort_name(a, b);
    if (ia < ib)
	return -1;
    return 1;
}

nomask int
sort_level(object a, object b)
{
    int ia = SECURITY->query_wiz_rank(a->query_real_name());
    int ib = SECURITY->query_wiz_rank(b->query_real_name());

    /* most likely check first to save cpu-time. */
    if (ia == ib)
        return 0;
    if (ia < ib)
	return -1;
    return 1;
}

nomask string
process_ip_name(object player)
{
    string ip = query_ip_name(player);
    string *arr = explode(ip, ".");
    int n = sizeof(arr);
    string str = "";

    switch (ip[-1..])
    {
        case "0".."9":		/* Brak DNSa - ip_name == ip_number. */
            return ip;
    }

    while(--n)
        str += arr[n] + " ";

    return str + arr[0];
}

nomask void
process_ip_names(object *players)
{
    int n = sizeof(players);

    processed_ip_names = ([]);
    while(n--)
        processed_ip_names[players[n]] = process_ip_name(players[n]);
}

nomask int
sort_name_and_ip_name(object a, object b)
{
    string sa = processed_ip_names[a];
    string sb = processed_ip_names[b];

    if (sa == sb)
	return sort_name(a, b);
    if (sa < sb)
	return -1;
    return 1;
}

nomask int
sort_ip_name(object a, object b)
{
    string sa = processed_ip_names[a];
    string sb = processed_ip_names[b];

    if (sa == sb)
	return 0;
    if (sa < sb)
	return -1;
    return 1;
}

nomask int
sort_name_and_ip_number(object a, object b)
{
    string sa = query_ip_number(a);
    string sb = query_ip_number(b);

    if (sa == sb)
	return sort_name(a, b);
    if (sa < sb)
	return -1;
    return 1;
}

nomask int
sort_ip_number(object a, object b)
{
    string sa = query_ip_number(a);
    string sb = query_ip_number(b);

    if (sa == sb)
	return 0;
    if (sa < sb)
	return -1;
    return 1;
}

nomask int
in_domain(object player, string domain)
{
    if (objectp(environment(player)))
    {
	return (explode(file_name(environment(player)), domain)[0] ==
	    P_DOMAIN_ROOT);
    }

    return 0;
}

nomask int
in_guild(object player, string guild)
{
    return ((player->query_guild_name_occ() == guild) ||
	(player->query_guild_name_lay() == guild) ||
	(player->query_guild_name_race() == guild));
}

nomask int
is_player(object obj)
{
    return ((function_exists("create_container", obj) == LIVING_OBJECT) ||
	(function_exists("create_object", obj) == POSSESSION_OBJECT));
}

nomask int
people(string opts)
{
    object *list, *list2, tmpob;
    mixed   foo;
    mixed  *contents = allocate(P_NUM);
    int     i, j, size, len, a, filt;
    int    *spcs = ({ });
    string  tmp_name, tmp_str, item, *tmp_arr, *flags;
    string  to_write = "";
    mapping snoop_map = ([ ]);

    CHECK_SO_WIZ;

    i = -1;
    while(++i < P_NUM)
	contents[i] = ({ });

    /* Default options if no command line argument was given. */
    if (!stringp(opts))
    {
	opts = "-ALirRdal";
    }

    /* Filter out the stuff that isn't really interesting, like login
     * objects and players in the creation process.
     */
    list = filter(users() - ({ 0 }), is_player);

    /* Start by collecting some general information, like the number
     * of players present and active and the number of people trying to
     * log in.
     */
    to_write += ("--- " + SECURITY->get_mud_name() + ": " + sizeof(list) +
	(sizeof(list) == 1 ? " player" : " players"));

    i = -1;
    a = 0;
    size = sizeof(list);
    while(++i < size)
    {
	if (query_idle(list[i]) >= 300) /* 5 minutes */
	{
	    a++;
	}
    }

    if (a)
    {
	to_write += (" (" + (sizeof(list) - a) + " active)");
    }
    to_write += ".\n";

    if (j = sizeof(users()) - sizeof(list))
    {
	to_write += ("There " + ((j == 1) ? "is one player" :
	    ("are " + j + " players")) +
	    " trying to log in right now.\n");
    }

    /* Test whether the last flag is a domain name, in which case, only list
     * the players in that domain are listed.
     */
    tmp_arr = explode(opts, " ");
    j = sizeof(tmp_arr);
    while(--j >= 0)
    {
	/* All normal argument flags must be preceded by a dash. We are
	 * kind and allow a slash, too. Single lettered arguments are also
	 * ignored.
	 */
	if ((strlen(tmp_arr[j]) <= 1) ||
	    (tmp_arr[j][0] == '-') ||
	    (tmp_arr[j][0] == '/'))
	{
	    continue;
	}

	item = lower_case(tmp_arr[j]);
	tmp_arr = exclude_array(tmp_arr, j, j);

	/* See whether it is a domain name. */
	if (SECURITY->query_domain_number(item) > -1)
	{
	    list = filter(list, &in_domain(, capitalize(item)));
	    continue;
	}

	opts = SECURITY->query_guild_long_name(item);
	/* See whether it is a guild name. */
	if (strlen(opts))
	{
	    list = filter(list, &in_guild(, opts));
	    continue;
	}

	/* Not recognised: Disallow the rest to execute to prevent spam. */
	notify_fail("The argument " + item + " is not recognised as " +
	    "domain or guild name. To prevent spam, people will not " +
	    "further execute. All arguments must be preceded by a dash, " +
	    "except for domain names and guild names.\n");
	return 0;
    }

    flags = explode(implode(tmp_arr, " "), "");

    /* If the people flag for snooped 's' is added, only list the people
     * that are actually snooped and those that are snooping them. That
     * is: only those with the correct access will be able to see the
     * snoopers too.
     */
    filt = (member_array("f", flags) >= 0 ? 1 : 0);
    if (member_array("s", flags) >= 0)
    {
	if (filt)
	    to_write += "Filtering those who are snooping or being snooped.\n";

	i = -1;
	size = sizeof(list);
	while(++i < size)
	{
	    if (tmpob = SECURITY->query_snoop(list[i]))
	    {
		if (objectp(tmpob))
		{
		    snoop_map[tmpob] = list[i];
		}
	    }
	    else if (filt)
	    {
		list[i] = 0;
	    }
	}

	if (filt)
	{
	    list -= ({ 0 });

	    if (m_sizeof(snoop_map))
	    {
		list2 = ({ }) + m_indices(snoop_map);
		list += (list2 - list);
	    }
	}
    }
    /* Wizard wants to list only the wizards present in the game. */
    if (member_array("w", flags) >= 0)
    {
	list = filter(list, &->query_wiz_level());

	/* These options have no sense for wizards. */
	flags -= ({ "M" });
    }
    /* Wizard wants to list only the mortals present in the game. This is
     * an else-statement to prevent wizards from doing people -mw DUH
     */
    else if (member_array("m", flags) >= 0)
    {
	list = filter(list, &not() @ &->query_wiz_level());

	/* save a LOT of cpu-time if no mortals are logged on :-) */
	if (!sizeof(list))
	{
	    to_write += ("\nCurrently no mortal players logged on.\n");
            /* No need to distinguish between people and mpeople here. */
            write(to_write);
	    return 1;
	}

	/* These options have no sense for mortals. */
	flags -= ({ "b", "d", "L", "r", "R" });
    }

    /* Sort the players by their level. */
    if (member_array("L", flags) >= 0)
    {
	/* This allows you to specify both "L" and "A" */
        if (member_array("A", flags) == -1)
	{
	    list = sort_array(list, "sort_level", this_object());
	}
	else
	{
	    list = sort_array(list, "sort_name_and_level", this_object());
	}
    }
    /* Sort the players by their ip-number. */
    else if (member_array("h", flags) >= 0)
    {
	/* This allows you to specify both "h" and "A" */
	if (member_array("A", flags) == -1)
	{
	    list = sort_array(list, "sort_ip_number", this_object());
	}
	else
	{
	    list = sort_array(list, "sort_name_and_ip_number", this_object());
	}
    }
    /* Sort the players by their ip-name. */
    else if (member_array("H", flags) >= 0)
    {
	/* This allows you to specify both "H" and "A" */
	if (member_array("A", flags) == -1)
	{
	    process_ip_names(list);
	    list = sort_array(list, "sort_ip_name", this_object());
	}
	else
	{
	    process_ip_names(list);
	    list = sort_array(list, "sort_name_and_ip_name", this_object());
	}
    }
    /* Sort the player by their name. */
    else if (member_array("A", flags) >= 0)
    {
	list = sort_array(list, "sort_name", this_object());
    }

    flags -= P_EMPTY;
    i = -1;
    len = sizeof(flags);
    spcs = allocate(len + 1);
    size = sizeof(list);
    while(++i < size)
    {
        /* should never return 'unknown' any more. */
	tmp_name = list[i]->query_possessed();
	if (!strlen(tmp_name))
	{
	    tmp_name = list[i]->query_real_name();
	}
	contents[P_NAME] += ({ capitalize(tmp_name) });
	spcs[0] = ((spcs[0] > strlen(contents[P_NAME][i])) ? spcs[0] :
	    strlen(contents[P_NAME][i]));

	j = -1;
	while (++j < len)
	{
	    switch(flags[j])
	    {
	    case "a": /* age */
		a = list[i]->query_age();

		if (a > 43200)
		{
		    contents[P_AGE] += ({ (a / 43200)  + " d" });
		}
		else if (a > 1800)
		{
		    contents[P_AGE] += ({ (a / 1800) + " h" });
		}
		else
		{
		    contents[P_AGE] += ({ (a / 30) + " m" });
		}

		spcs[j + 1] = ((spcs[j + 1] > strlen(contents[P_AGE][i])) ?
		    spcs[j + 1] : strlen(contents[P_AGE][i]));
		break;

	    case "b": /* busy */
		contents[P_BUSY] +=
		    ({ busy_string(list[i]->query_prop(WIZARD_I_BUSY_LEVEL))
		       });
		spcs[j + 1] = ((spcs[j + 1] > strlen(contents[P_BUSY][i])) ?
		    spcs[j + 1] : strlen(contents[P_BUSY][i]));
		break;

	    case "d": /*domain */
		contents[P_DOMAIN] += ({ SECURITY->query_wiz_dom(tmp_name) });
		spcs[j + 1] = ((spcs[j + 1] > strlen(contents[P_DOMAIN][i])) ?
		    spcs[j + 1] : strlen(contents[P_DOMAIN][i]));
		break;

	    case "i": /* idle */
		if (query_idle(list[i]) >= 300)
		{
		    contents[P_IDLE] += ({ "I" });
		}
		else
		{
		    contents[P_IDLE] += ({ "" });
		}
		spcs[j + 1] = ((spcs[j + 1] > strlen(contents[P_IDLE][i])) ?
		    spcs[j + 1] : strlen(contents[P_IDLE][i]));
		break;

	    case "l": /* location */
		if (strlen(list[i]->query_possessed()))
		{
		    if (function_exists("possess", list[i]) ==
			POSSESSION_OBJECT)
		    {
			contents[P_LOCATION] += ({ "Possessed by " +
		            capitalize(list[i]->query_demon()) + "." });
		    }
		    else
		    {
			contents[P_LOCATION] += ({ capitalize(list[i]->
			    query_real_name()) + "@" + (environment(list[i]) ?
			    RPATH(file_name(environment(list[i]))) :
			    "In the big black void.") });
		    }
		}
		else
		{
		    contents[P_LOCATION] += ({ environment(list[i]) ?
		        RPATH(file_name(environment(list[i]))) :
		        "In the big black void." });
		}
		spcs[j + 1] = ((spcs[j + 1] >
				strlen(contents[P_LOCATION][i])) ?
			       spcs[j + 1] : strlen(contents[P_LOCATION][i]));
		break;

	    case "M": /* met */
		contents[P_MET] += ({ ((list[i]->
		    query_met(this_interactive())) ? "M" : "") });
		spcs[j + 1] = ((spcs[j + 1] > strlen(contents[P_MET][i])) ?
		    spcs[j + 1] : strlen(contents[P_MET][i]));
		break;

	    case "n": /* ip-number */
		contents[P_IPNR] += ({ query_ip_number(list[i]) });
		spcs[j + 1] = ((spcs[j + 1] > strlen(contents[P_IPNR][i])) ?
		    spcs[j + 1] : strlen(contents[P_IPNR][i]));
		break;

	    case "N": /* ip-name */
		if (WIZ_CHECK >= WIZ_ARCH &&
		    strlen(tmp_str = query_ip_ident(list[i])))
		{
		    tmp_str += "@";
		}
		else
		{
		    tmp_str = "";
		}
		contents[P_IPNAME] += ({ tmp_str + query_ip_name(list[i]) });
		spcs[j + 1] = ((spcs[j + 1] > strlen(contents[P_IPNAME][i])) ?
		    spcs[j + 1] : strlen(contents[P_IPNAME][i]));
		break;

	    case "r": /* title rank */
		contents[P_RTITLE] +=
		    ({ capitalize(WIZ_RANK_NAME(SECURITY->query_wiz_rank(tmp_name))) });
		spcs[j + 1] = ((spcs[j + 1] > strlen(contents[P_RTITLE][i])) ?
		    spcs[j + 1] : strlen(contents[P_RTITLE][i]));
		break;

	    case "R": /* level rank */
		contents[P_RLEVEL] += ({ "" +
		    SECURITY->query_wiz_level(tmp_name) });
		spcs[j + 1] = ((spcs[j + 1] > strlen(contents[P_RLEVEL][i])) ?
		    spcs[j + 1] : strlen(contents[P_RLEVEL][i]));
		break;

	    case "s": /* snoop */
		tmp_str = (objectp(snoop_map[list[i]]) ?  (">" +
		    capitalize(snoop_map[list[i]]->query_real_name())) : "");

		if ((foo = SECURITY->query_snoop(list[i])))
		{
		    if (objectp(foo) && strlen(foo->query_real_name()))
		    {
			contents[P_SNOOP] += ({ tmp_str + "<" +
			    capitalize(foo->query_real_name()) });
		    }
		    else
		    {
			contents[P_SNOOP] += ({ tmp_str + "<S" });
		    }
		}
		else
		{
		    contents[P_SNOOP] += ({ tmp_str + "" });
		}
		spcs[j + 1] = ((spcs[j + 1] > strlen(contents[P_SNOOP][i])) ?
		    spcs[j + 1] : strlen(contents[P_SNOOP][i]));
		break;

	    case "S": /* Species */
		if (tmp_str = capitalize(list[i]->query_race()))
		{
		    if (stringp(RACES_SHORT[tmp_str]))
		    {
			contents[P_SPECIES] += ({ RACES_SHORT[tmp_str] });
		    }
		    else
		    {
			contents[P_SPECIES] += ({ extract(tmp_str, 0, 2) });
		    }
		}
		else
		{
		    contents[P_SPECIES] += ({ "---" });
		}
		break;

	    case "V": /* aVerage stat */
		contents[P_AVERAGE] +=
		    ({ list[i]->query_average_stat() + "" });
		break;

	    case "D": /* nonmet description */
/*
		if (sizeof(tmp_arr = list[i]->query_adj(1)) > 0)
		{
		    contents[P_DESC] += ({ implode(tmp_arr, " ") });
		}
		else
		{
		    contents[P_DESC] += ({ "---" });
		}
*/
                contents[P_DESC] += ({list[i]->query_nonmet_name()});
                spcs[j + 1] = ((spcs[j + 1] > strlen(contents[P_DESC][i])) ?
                    spcs[j + 1] : strlen(contents[P_DESC][i]));
		break;

	    case "O": /* odmiana */
		if (sizeof(tmp_arr = list[i]->query_imiona()) >= PL_MIE)
		{
		    int tmp = PL_MIA;

		    while (++tmp <= PL_MIE)
		        tmp_arr[tmp] = capitalize(tmp_arr[tmp]);

		    contents[P_DECL] += ({ implode(tmp_arr[PL_DOP..], " ") });
		}
		else
		{
		    contents[P_DECL] += ({ " ---" });
		}
		spcs[j + 1] = ((spcs[j + 1] > strlen(contents[P_DECL][i])) ?
		    spcs[j + 1] : strlen(contents[P_DECL][i]));
		break;

	    default: /* Strange... */
		break;
	    }
	}
    }

    i = -1;
    while (++i < size)
    {
	item = sprintf("%-*s ", spcs[0], contents[P_NAME][i]);
	j = -1;
	while(++j < len)
	{
	    switch(flags[j])
	    {
	    case "a": /* age */
		item += sprintf("%*s ", spcs[j + 1], contents[P_AGE][i]);
		break;

	    case "b": /* busy */
		item += sprintf("%-*s ", spcs[j + 1], contents[P_BUSY][i]);
		break;

	    case "d": /*domain */
		item += sprintf("%-*s ", spcs[j + 1], contents[P_DOMAIN][i]);
		break;

	    case "i": /* idle */
		item += sprintf("%*s ", spcs[j + 1], contents[P_IDLE][i]);
		break;

	    case "l": /* location */
		item += sprintf("%s ", contents[P_LOCATION][i]);
		break;

	    case "M": /* met */
		item += sprintf("%-*s ", spcs[j + 1], contents[P_MET][i]);
		break;

	    case "n": /* ip-number */
		item += sprintf("%-*s ", spcs[j + 1], contents[P_IPNR][i]);
		break;

	    case "N": /* ip-name */
		item += sprintf("%-*s ", spcs[j + 1], contents[P_IPNAME][i]);
		break;

	    case "r": /* title rank */
		item += sprintf("%-*s ", spcs[j + 1], contents[P_RTITLE][i]);
		break;

	    case "R": /* level rank */
		item += sprintf("%*s ", spcs[j + 1], contents[P_RLEVEL][i]);
		break;

	    case "s": /* snoop */
		item += sprintf("%-*s ", spcs[j + 1], contents[P_SNOOP][i]);
		break;

	    case "S": /* Species */
		item += sprintf("%-*s ", P_LEN_SPECIES,
				contents[P_SPECIES][i]);
		break;

	    case "V": /* aVerage stat */
		item += sprintf("%*s ", P_LEN_AVERAGE, contents[P_AVERAGE][i]);
		break;

	    case "D": /* description */
		item += sprintf("%-*s ", spcs[j + 1], contents[P_DESC][i]);
		break;

	    case "O": /* odmiana */
		item += sprintf("%-*s ", spcs[j + 1], contents[P_DECL][i]);
		break;

	    default: /* Strange... */
		break;
	    }
	}
	to_write += (item + "\n");
    }

    if (query_verb() == "mpeople")
    {
        setuid();
        seteuid(getuid());

        this_player()->more(to_write);
    }
    else
    {
        write(to_write);
    }

    return 1;
}
