/*
 * /cmd/wiz/apprentice.c
 *
 * This object holds the apprentice wizards commands. The soul has been split
 * over a few modules. Hence, the list of commands below only reflects the
 * commands actually coded in this module:
 * 
 * - allcmd
 * - altitle
 * - applications
 * - apply
 * - bit
 * - domainsanction (short: dsanction)
 * - finger
 * - gd
 * - gfinger
 * - goto
 * - graph
 * - gtell
 * - guild
 * - gwiz
 * - gwize
 * - home
 * - last
 * - localcmd
 * - metflag
 * - mudlist
 * - newhooks
 * - notify
 * - odmimie
 * - ranking
 * - regret
 * - review
 * - rsupport
 * - rwho
 * - sanction
 * - second
 * - setmin
 * - setmmin
 * - setmmout
 * - setmout
 * - start
 * - title
 * - unmetflag
 * - wizopt
 * - whereis
 * - whichsoul
 */

#pragma no_clone
#pragma no_inherit
#pragma strict_types
#pragma save_binary

inherit "/cmd/std/tracer_tool_base.c";

#include <cmdparse.h>
#include <composite.h>
#include <filter_funs.h>
#include <formulas.h>
#include <language.h>
#include <mail.h>
#include <macros.h>
#include <ss_types.h>
#include <std.h>
#include <stdproperties.h>
#include <time.h>
#include <options.h>
#include <pl.h>
#include <living_desc.h>

#define CHECK_SO_WIZ 	if (WIZ_CHECK < WIZ_APPRENTICE) return 0; \
			if (this_interactive() != this_player()) return 0
#define ALLCMD_LIST_SOULS(name, souls, screen) \
            (name + break_string(implode((souls)->get_soul_id(), ", "), \
                                 screen, 10)[10..] + "\n")
#define WIZARD_S_GOTO_SET "_wizard_s_goto_set"

#include "/cmd/wiz/apprentice/communication.c"
#include "/cmd/wiz/apprentice/files.c"
#include "/cmd/wiz/apprentice/manual.c"
#include "/cmd/wiz/apprentice/people.c"

/* **************************************************************************
 * At creation, the save-file of the soul is restored.
 */
nomask void
create()
{
    setuid();
    seteuid(getuid());

    restore_object(WIZ_CMD_APPRENTICE);

    init_line();
}

/* **************************************************************************
 * Return a list of which souls are to be loaded.
 * The souls are listed in order of command search.
 */
nomask string *
get_soul_list()
{
    return ({ WIZ_CMD_APPRENTICE,
	      MBS_SOUL });
}

/* **************************************************************************
 * Return a proper name of the soul in order to get a nice printout.
 */
nomask string
get_soul_id()
{
    return WIZNAME_APPRENTICE;
}

/* **************************************************************************
 * The list of verbs and functions. Please add new in alfabetical order.
 */
nomask mapping
query_cmdlist()
{
    return ([
	     "allcmd":"allcmd",
#ifndef NO_ALIGN_TITLE
	     "altitle":"altitle",
#endif NO_ALIGN_TITLE
	     "applications":"applications",
	     "apply":"apply",
	     "audience":"audience",

	     "bit":"bit",
	     "busy":"busy",

	     "cat":"cat_file",
	     "cd":"cd",

	     "dirs":"dirs",
	     "domainsanction":"domainsanction",
	     "dsanction":"domainsanction",

	     "emote":"emote",
	     ":":"emote",

	     "finger":"finger",

	     "gd":"gd_info",
	     "gfinger":"gfinger",
	     "graph":"graph",
	     "goto":"goto",
	     "gtell":"gtell",
	     "guild":"guild",
	     "guildtell":"guildtell",
	     "gwiz":"gwiz",
	     "gwize":"gwiz",

	     "home":"home",

	     "last":"last",
	     "line":"line",
	     "linee":"line",
	     "lineconfig":"lineconfig",
	     "lman":"lman",
	     "localcmd":"localcmd",
	     "ls":"list_files",

	     "man":"man",
	     "metflag":"metflag",
	     "more":"more_file",
             "mpeople":"people",
	     "mudlist":"mudlist",

	     "newhooks":"newhooks",
	     "next":"next",
	     "notify":"notify",
	     
	     "odmimie":"odm_imie",

	     "people":"people",
	     "people_guild":"people_guild",
	     "popd":"popd",
	     "pushd":"pushd",
	     "pwd":"pwd",

	     "ranking":"ranking",
	     "regret":"regret",
	     "review":"review",
	     "rsupport":"rsupport",
	     "rwho":"rwho",

             "sanction":"sanction",
	     "second":"second",
	     "setmin":"set_m_in",
	     "setmmin":"set_mm_in",
	     "setmmout":"set_mm_out",
	     "setmout":"set_m_out",
	     "sman":"sman",
	     "start":"start",

	     "tail":"tail_file",
	     "tell":"tell",
	     "title":"title",
	     "tree":"tree",

	     "unmetflag":"unmetflag",

	     "wizopt":"wizopt",
	     "whereis":"whereis",
             "whichsoul":"whichsoul",
	     "wiz":"wiz",
	     "wize":"wiz",
             "wsay":"wsay",
	     ]);
}

/* **************************************************************************
 * Here follows the actual functions. Please add new functions in the
 * same order as in the function name list.
 * **************************************************************************/

/* **************************************************************************
 * allcmd - List all commands available to a player
 */

/*
 * Function name: print_soul_list
 * Description  : This function actually prints the souls in wizard and
 *                commands linked to them.
 * Arguments    : string *soul_list - the souls in this section.
 *                string  soul      - then soul the wizard wants to see,
 *                                    0 if (s)he wants to see all.
 * Returns      : Number of matching souls found.
 */
static int
print_soul_list(string *soul_list, string soul)
{
    int index = -1;
    int size = sizeof(soul_list);
    int counter = 0;
    string soul_id;

    while(++index < size)
    {
        soul_id = soul_list[index]->get_soul_id();

        if (!soul || soul == soul_id)
        {
            write("----- " + capitalize(soul_id) + ":\n");
            write(implode(sort_array(m_indices(
                soul_list[index]->query_cmdlist())), ", ") + "\n");

            counter++;
        }
    }

    return counter;
}

public int
allcmd(string soul)
{
    object ob;
    int counter;

    CHECK_SO_WIZ;

    if (soul)
    {
        string *list = explode(soul, " ");

        if (list[0] == "me")
        {
            ob = this_player();
            soul = sizeof(list) == 1 ? 0 : implode(list[1..], " ");
        }
        else if (ob = find_living(list[0]))
            soul = sizeof(list) == 1 ? 0 : implode(list[1..], " ");
        else
            ob = this_player();
    }
    else
        ob = this_player();

    if (soul == "list")
    {
        int screen_width = this_player()->query_option(OPT_SCREEN_WIDTH);

        write("Localcmd: base\n");
        if (sizeof(ob->query_wizsoul_list()))
            write(ALLCMD_LIST_SOULS("Wizsouls: ", ob->query_wizsoul_list(),
                                    screen_width));
        if (sizeof(ob->query_tool_list()))
            write(ALLCMD_LIST_SOULS("Tools:    ", ob->query_tool_list(),
                                    screen_width));
        if (sizeof(ob->query_cmdsoul_list()))
            write(ALLCMD_LIST_SOULS("Cmdsouls: ", ob->query_cmdsoul_list(),
                                    screen_width));

        return 1;
    }

    if (!soul || soul == "base")
    {
        /* We do not sort the local commands because their order is linked
         * to the order of their execution. The first command listed is
         * tested. This order also reflects the inventory of the player.
         * Objects first in the inventory are evaluated before objects later
         * in the inventory. */

        write("----- Base:\n");
        write(implode(ob->local_cmd(), ", ") + "\n");

        counter = 1;
    }
    else
    {
        counter = 0;
        notify_fail("No soul named \"" + soul + "\" found.\n");
    }

    return counter + print_soul_list(ob->query_wizsoul_list()
                                   + ob->query_tool_list()
                                   + ob->query_cmdsoul_list(), soul);
}

/* **************************************************************************
 * altitle - display/change the alignment title
 */
#ifndef NO_ALIGN_TITLE
nomask int
altitle(string t)
{
    CHECK_SO_WIZ;

    if (!stringp(t))
    {
	write("Your alignment title is '" +
	      this_interactive()->query_al_title() + "'.\n");
	return 1;
    }
    this_interactive()->set_al_title(t);
    write("Ok.\n");
    return 1;
}
#endif

/* **************************************************************************
 * applications - display applications to a domain
 */
nomask int
applications(string domain)
{
    CHECK_SO_WIZ;

    return SECURITY->list_applications(domain);
}

/* **************************************************************************
 * apply - apply to a domain
 */
nomask int
apply(string domain)
{
    CHECK_SO_WIZ;

    return SECURITY->apply_to_domain(domain);
}

/* **************************************************************************
 * bit - affect bits
 */
nomask int
bit(string args)
{
    int    argc;
    int    group;
    int    bit;
    int    index;
    int    index2;
    string *argv;
    string result;
    object player;

    CHECK_SO_WIZ;

    if (!stringp(args))
    {
	notify_fail("No argument to 'bit'. Check with the help text.\n");
	return 0;
    }
    argv = explode(args, " ");
    argc = sizeof(argv);

    if (argc == 1)
    {
	notify_fail("Not enough arguments to 'bit'. "
	          + "Check with the help text.\n");
	return 0;
    }

    if (!objectp(player = find_player(argv[1])))
    {
    	notify_fail("Player " + capitalize(argv[1]) + " is not logged in.\n");
    	return 0;
    }

    if (argc == 5)
    {
    	if (SECURITY->query_wiz_rank(this_player()->query_real_name()) <
    	    WIZ_ARCH)
    	{
    	    notify_fail("You are not allowed to give a domain name as " +
    	    	"argument. In order to change a bit administered by your " +
    	    	"own domain, you can omit the domain argument.\n");
    	    return 0;
    	}

	argv[2] = capitalize(argv[2]);
    	if (SECURITY->query_domain_number(argv[2]) == -1)
    	{
    	    notify_fail("There is no domain named '" + argv[2] + "'.\n");
    	    return 0;
    	}

	if (!seteuid(argv[2]))
	{
	    write("Failed to set euid to " + argv[2] + ".\n");
	    return 1;
	}

	write("Euid changed to " + argv[2] + " for bit operation!\n");
    	argv = exclude_array(argv, 2, 2);
    	argc--;
    }

    switch(argv[0])
    {
    case "set":
	if (argc != 4)
	{
	    write("Syntax error. Check with the help text.\n");
	    return 1;
	}
	if ((sscanf(argv[2], "%d", group) != 1) ||
	    (sscanf(argv[3], "%d", bit) != 1))
	{
	    write("Syntax error. Check with the help text.\n");
	    return 1;
	}
	if (!(player->set_bit(group, bit)))
	{
	    write("You were not allowed to set bit " + group + ":" + bit +
		" in " + capitalize(argv[1]) + ".\n");
	    return 1;
	}
	write("Set bit " + group + ":" + bit + " in " + capitalize(argv[1]) +
	    ".\n");
	return 1;

    case "clear":
	if (argc != 4)
	{
	    write("Syntax error. Check with the help text.\n");
	    return 1;
	}
	if ((sscanf(argv[2], "%d", group) != 1) ||
	    (sscanf(argv[3], "%d", bit) != 1))
	{
	    write("Syntax error. Check with the help text.\n");
	    return 1;
	}
	if (!(player->clear_bit(group, bit)))
	{
	    write("You were not allowed to clear bit " + group + ":" + bit +
		" in " + capitalize(argv[1]) + ".\n");
	    return 1;
	}
	write("Cleared bit " + group + ":" + bit + " in " +
	    capitalize(argv[1]) + ".\n");
	return 1;

    case "list":
	if (argc != 3)
	{
	    write("Syntax: bit list <player> <domain>\n");
	    return 1;
	}

	argv[2] = capitalize(argv[2]);
	result = "";
	index = -1;
	while(++index < 5)
	{
	    index2 = -1;
	    while(++index2 < 20)
	    {
		if (player->test_bit(argv[2], index, index2))
		{
		    result += " " + index + ":" + index2;
		}
	    }
	}
	if (!strlen(result))
	{
	    write(argv[2] + " has no bits set in " +
		capitalize(argv[1]) + ".\n");
	    return 1;
	}

	write(argv[2] + " has set bits:" + result + "\n");
	return 1;

    default:
    	notify_fail("No subcommand '" + argv[0] + "' for the command bit.\n");
	return 0;
    }

    notify_fail("Should never happen: switch() error in then end of bit()\n");
    return 0;
}

/* **************************************************************************
 * domainsanction - manage the domain sanction list
 */
nomask int
domainsanction(string str)
{
    CHECK_SO_WIZ;

    return SECURITY->domainsanction(str);
}

/* **************************************************************************
 * finger - display information about player/domain/wiztype
 */

/*
 * Function name: domainfinger
 * Description  : This function is called to display finger information about
 *                a domain.
 * Arguments    : string domain - the domain name.
 */
nomask static void
domainfinger(string domain)
{
    string *names;
    string name;
    string path;
    int    size;

    names = (string *)SECURITY->query_domain_members(domain);
    name = SECURITY->query_domain_lord(domain);
    if (strlen(name))
    {
	write("The Liege of " + domain + " is " + capitalize(name));
	names = names - ({ name });
    }
    else
    {
	write("The domain " + domain + " has no Liege");
    }

    name = SECURITY->query_domain_steward(domain);
    if (strlen(name))
    {
	write(", " + capitalize(name) + " is the steward.\n");
	names -= ({ name });
    }
    else
    {
	write(".\n");
    }

    if (!sizeof(names))
    {
	write("The domain has no members.\n");
    }
    else
    {
	names = sort_array(map(names, capitalize));
	write("The domain has " + LANG_WNUM(sizeof(names)) + " member" +
	      ((sizeof(names) == 1) ? "" : "s") + ": " +
	      COMPOSITE_WORDS(names) + ".\n");
    }

    name = SECURITY->query_domain_madwand(domain);
    if (strlen(name))
    {
	write("Madwand of the domain is " + capitalize(name) + ".\n");
    }

    size = SECURITY->query_domain_max(domain) -
	sizeof(SECURITY->query_domain_members(domain));
    if (!size)
    {
	write("The are no vacancies.\n");
    }
    else
    {
	write("There " + ((size == 1) ? "is " : "are ") + LANG_WNUM(size) +
	      ((size == 1) ? " vacancy" : " vacancies") + ".\n");
    }

    path = SECURITY->query_wiz_path(domain) + "/open/finger_info";
    if (file_size(path) > 0)
    {
	write("--------- Special domain supplied info:\n" +
	      read_file(path, 1, 10) + "\n");
    }
}

/*
 * Function name: playerfinger
 * Description  : This function is called to display finger information about
 *                a player.
 * Arguments    : string name - the player name.
 *                int l - true if the long version is desired
 */
nomask static void
playerfinger(string name, int l)
{
    int    real;
    object player;
    object env;
    string pronoun;
    string domain;
    string str;
    string *names;
    int    chtime;

    /* Display the location of the player in the game, get a 'finger_player'
     * if the player is not in the game.
     */
    if (objectp(player = find_player(name)))
    {
        write(capitalize(name) + " jest obecnie w grze. Polozenie: "
            + ((env = environment(player)) ? RPATH(file_name(env)) : "VOID")
            + "\n");
        real = 1;
    }
    else
    {
        player = SECURITY->finger_player(name);
        write(player->query_name(PL_DOP) + " nie ma obecnie w grze.\n");
        real = 0;
    }

    /* Display the long description of the player. */
    if (l || !real)
        write(player->long());
    else
        write("Jest " + player->query_nonmet_name(PL_NAR) + ", znan"
            + player->koncowka("ym", "a") + " jako:\n"
            + player->query_presentation() + ".\n");

    pronoun = ((this_player()->query_real_name() == name) ? "You are " :
	       capitalize(player->query_pronoun()) + " is ");

    /* Display the rank/level of the player. */
    if (SECURITY->query_wiz_rank(name) >= WIZ_APPRENTICE)
	write(pronoun +
	      LANG_ADDART(WIZ_RANK_NAME(SECURITY->query_wiz_rank(name))) +
	      " (level " + SECURITY->query_wiz_level(name) +
	      "), set by " +
	      (strlen(str = SECURITY->query_wiz_chl(name)) ?
	       capitalize(str) : "root") +
#ifdef FOB_KEEP_CHANGE_TIME
	      ((chtime = SECURITY->query_wiz_chl_time(name)) ?
	       (" on " + ctime(chtime)) : "") +
#endif FOB_KEEP_CHANGE_TIME
	      ".\n");
    else
	write(pronoun + "a mortal player.\n");

    /* Display the domain the player is in. */
    if (strlen(domain = SECURITY->query_wiz_dom(name)))
	write(pronoun +
	      ((SECURITY->query_wiz_rank(name) == WIZ_LORD) ? "Liege" :
	       "a member") + " of the domain " + domain + ", added by " +
	      (strlen(str = SECURITY->query_wiz_chd(name)) ?
	       capitalize(str) : "root") +
#ifdef FOB_KEEP_CHANGE_TIME
	      ((chtime = SECURITY->query_wiz_chd_time(name)) ?
	       (" on " + ctime(chtime)) : "") +
#endif FOB_KEEP_CHANGE_TIME
	      ".\n");

    /* Display login information. */
    if (real)
    {
        write(pronoun + "logged on for " +
            CONVTIME(time() - player->query_login_time()) + " from " +
            player->query_login_from() + ".\n");
	if (query_ip_number(player))
	{
	    if (query_idle(player) > 0)
		write("Idle time: " + CONVTIME(query_idle(player)) + "\n");
	}
	else
	{
            write(pronoun + "linkdead for " +
                CONVTIME(time() - player->query_linkdead()) + ".\n");
        }
    }
    else
    {
	write("Last login " + CONVTIME(time() - player->query_login_time()) +
	    " ago from " + player->query_login_from() + ".\n");
        chtime = file_time(PLAYER_FILE(name) + ".o") -
            player->query_login_time();
        if (chtime < 86400) /* 24 hours, guard against patched files */
        {
            write("Duration of stay was " + CONVTIME(chtime) + ".\n");
        }
    }

    /* Display the age and email address of the player. */
    write("Age: " + CONVTIME(player->query_age() * 2) + "\n");
    write("Email: " + player->query_mailaddr() + "\n");

    /* Ewentualne pinfo o graczu. */
    if (WIZ_CMD_HELPER->pinfo_info(name))
        write("There is a pinfo record about " + player->query_objective()
            + ".\n");

    /* Wizowe finger_info. */
    player->finger_info();

    /* Clean up after ourselves if the player is not logged in. */
    if (!real)
	player->remove_object();
}

nomask int
finger(string str)
{
    int    index;
    int    size;
    string *names, str2;
    mapping gread;
    object *logins;
    mixed *banished;

    CHECK_SO_WIZ;

    /* This function never uses notify_fail() and return 0 since I do not
     * want it to fall back to the 'mortal' finger command. This is not
     * because of the emote as such, but merely to not mess up the fail
     * message which might contain useful information.
     */
    if (!stringp(str))
    {
	write("Syntax: finger <something>\n");
	return 1;
    }

#if 0
    /* Wizard wants to use the 'mortal' finger command. */
    if (wildmatch("-m *", str))
    {
	return SOUL_CMD->finger(extract(str, 3));
    }
#endif

    /* Wizard wants to finger a player. */
    if (wildmatch("-l *", str) &&
        SECURITY->exist_player(str2 = lower_case(extract(str, 3))))
    {
        playerfinger(str2, 1);
        return 1;
    }

    if (SECURITY->exist_player(str2 = lower_case(str)))
    {
        playerfinger(str2, 0);
        return 1;
    }

    /* Wizard wants to list all domains. */
    if (str == "domains")
    {
        write(sprintf("The domains of this mud are:\n%-60#s\n",
		      implode(sort_array(SECURITY->query_domain_list()),
			      "\n")));
	return 1;
    }

    /* Wizard wants to list a particular domain. */
    if (SECURITY->query_domain_number(str) > -1)
    {
	domainfinger(capitalize(str));
	return 1;
    }

    /* Wizard wants to list the people in the queue. */
    if ((str == "queue") ||
	(str == "q"))
    {
	names = QUEUE->queue_list(1);
	if (!(size = sizeof(names)))
	{
	    write("There are no players in the queue right now.\n");
	    return 1;
	}

	index = -1;
	while(++index < size)
	{
	    names[index] = sprintf("%2d: %s", (index + 1), names[index]);
	}
	write("The following people are in the queue:\n" +
	      sprintf("%-70#s\n", implode(names, "\n")));
	return 1;
    }

    /* Wizard wants to list all trainees. */
    if ((str == "trainee") ||
	(str == "trainees"))
    {
	names = SECURITY->query_trainees();
	
	if (!(size = sizeof(names)))
	{
	    write("No trainees are listed.\n");
	    return 1;
	}

	names = sort_array(names);
	write("Trainee     Domain\n----------- ------\n");
	index = -1;
	while(++index < size)
	{
	    write(sprintf("%-11s", capitalize(names[index])) + " " +
		  SECURITY->query_wiz_dom(names[index]) + "\n");
	}
	return 1;
    }

    /* Wizard wants to list those with global read. */
    if ((str == "global read") ||
	(str == "global") ||
	(str == "globals"))
    {
	gread = SECURITY->query_global_read();
	if (!m_sizeof(gread))
	{
	    write("There are no wizards with global read rights.\n");
	    return 1;
	}
	
	write("Wizard      Added by    Reason\n");
	write("----------- ----------- ------\n");
	names = sort_array(m_indices(gread));
	index = -1;
	size = sizeof(names);
	while(++index < size)
	{
	    write(sprintf("%-11s %-11s %s\n",
			  capitalize(names[index]),
			  capitalize(gread[names[index]][0]),
			  gread[names[index]][1]));
	}
	return 1;
    }
    
    if (str == "login")
    {
        logins = object_clones(find_object("/secure/login"));
        if (!(size = sizeof(logins)))
            write("W chwili obecnej nikt sie nie loguje.\n");
            
        index = -1;
        while(++index < size)
            write(logins[index]->query_name() + " - " + 
                query_ip_number(logins[index]) + ", " + 
                query_ip_name(logins[index]) + ".\n");
        return 1;
    }

    /* Wizard wants to see a particular class of wizards. */
    if ((index = member_array(LANG_SWORD(str), WIZ_N)) > -1)
    {
	names = SECURITY->query_wiz_list(WIZ_R[index]);
	if (!sizeof(names))
	{
	    write("No wizards of that type registered.\n");
	    return 1;
	}

	write(sprintf("The following " + LANG_PWORD(WIZ_N[index]) +
		      " are registered:\n%-60#s\n",
		      implode(sort_array(map(names, capitalize)), "\n")));
	return 1;
    }

    banished = SECURITY->banish(str, 0);
    if (sizeof(banished) == 2)
    {
        write("The name " + capitalize(str) + " was banished by " +
            capitalize(banished[0]) + " on " + ctime(banished[1], 1) + ".\n");
        return 1;
    }

    /* Ewentualne pinfo o graczu. */
    if (WIZ_CMD_HELPER->pinfo_info(str))
    {
        write("There is no such player. There is, however, a pinfo record "
            + "about it.\n");
        return 1;
    }

    write("There is no such player, domain, category, etcetera.\n");
    return 1;
}

/* **************************************************************************
 * gd_info - Get some relevant Game driver information
 */
nomask int
gd_info(string icmd)
{
    string inf, *p;
    object ob;
    int f;

    if (!stringp(icmd))
    {
	notify_fail("No argument to 'gd'.\n");
	return 0;
    }

    inf = SECURITY->do_debug(icmd);
    if (!inf)
    {
	if (sizeof(p = explode(icmd, " ")) > 1)
	{
	    if (sizeof(p) > 2)
	    {
		f = 0;
		if (sscanf(p[1],"%d", f) != 1)
		    f = 0;
		ob = parse_list(p[2]);
		inf = SECURITY->do_debug(p[0], f, ob);
	    }
	    else
	    {
		ob = parse_list(p[1]);
		inf = SECURITY->do_debug(p[0], ob);
	    }
	}
    }

    if (stringp(inf))
	write(icmd + ":\n" + inf + "\n");
    else
	dump_array(inf);
    write("\n");
    return 1;
}

/* **************************************************************************
 * gfinger - finger showone at another mud
 */
nomask int
gfinger(string str)
{
    CHECK_SO_WIZ;
#ifdef UDP_MANAGER
    return UDP_MANAGER->cmd_gfinger(str);
#else
    notify_fail("No udp manager active.\n");
    return 0;
#endif
}


/* **************************************************************************
 * graph - display user graphs
 */
nomask int
graph(string str)
{
    return SECURITY->graph(str);
}

/* **************************************************************************
 * goto - teleport somewhere
 */
nomask int
goto(string dest)
{
    object loc;

    CHECK_SO_WIZ;

    if (!stringp(dest))
    {
	if (objectp(loc = this_interactive()->query_prop(LIVE_O_LAST_ROOM)))
	{
	    this_interactive()->move_living("X", loc);
	    return 1;
	}

	notify_fail("Goto where? You have no last room.\n");
	return 0;
    }

    if (!objectp(loc = find_player(dest)))
    {
	loc = find_living(dest);
    }

    if (objectp(loc) &&
	objectp(environment(loc)))
    {
	this_interactive()->move_living("X", environment(loc));
	return 1;
    }

    if (sscanf(dest, "set %s", dest) == 1)
    {
	if (!objectp(loc = parse_list(dest)))
	{
	    notify_fail("Room '" + dest + "' not found.\n");
	    return 0;
	}

	dest = file_name(loc);
	this_interactive()->add_prop(WIZARD_S_GOTO_SET, dest);
	write("Goto set to '" + dest + "'.\n");

	return 1;
    }

    if (dest == "set")
    {
	if (!stringp(dest = this_interactive()->query_prop(WIZARD_S_GOTO_SET)))
	{
	    notify_fail("You have no previous location set.\n");
	    return 0;
	}
    }
    else
    {
	dest = FTPATH(this_interactive()->query_path(), dest);
    }

    if (!objectp(loc = find_object(dest)))
    {
	if (LOAD_ERR(dest))
	{
	    notify_fail("Destination '" + dest + "' cannot be loaded.\n");
	    return 0;
	}

	if (!objectp(loc = find_object(dest)))
	{
	    notify_fail("Destination '" + dest + "' not found.\n");
	    return 0;
	}
    }

    this_interactive()->move_living("X", loc);
    return 1;
}

/* **************************************************************************
 * gtell - tell showone at another mud
 */
nomask int
gtell(string str)
{
    CHECK_SO_WIZ;
#ifdef UDP_MANAGER
    return UDP_MANAGER->cmd_gtell(str);
#else
    notify_fail("No udp manager active.\n");
    return 0;
#endif
}

/* **************************************************************************
 * guild - manage information about guilds
 */
nomask int
guild(string str)
{
    CHECK_SO_WIZ;

    return SECURITY->guild_command(str);
}

/* **************************************************************************
 * gwiz - intermud wizline
 */
nomask int
gwiz(string str)
{
    CHECK_SO_WIZ;
#ifdef UDP_MANAGER
    return UDP_MANAGER->cmd_gwiz(str, query_verb() == "gwize");
#else
    notify_fail("No udp manager active.\n");
    return 0;
#endif
}

/* **************************************************************************
 * home - go home, to a domain workroom, or to an other wizards home
 */
nomask int
home(string str)
{
    CHECK_SO_WIZ;

    if (str == "admin")
    {
	str = ADMIN_HOME;
    }
    else
    {
	if (!stringp(str))
	{
	    str = this_interactive()->query_real_name();
	}

	str = SECURITY->wiz_home(str);
    }

    if (this_interactive()->move_living("X", str))
    {
	write("There is no such workroom.\n");
    }
    return 1;
}

/* **************************************************************************
 * last - give information on the players login-time
 */

/*
 * Function name: last_check
 * Description  : This function is used by the last-command. It prints the
 *                individual lines about all players the wizard selected
 *                to gather information about.
 * Arguments    : string who   - the name of the wizard to check.
 *                int    login - true for login-info rather than logout-info.
 * Returns      : string - the line to print.
 */
static nomask string
last_check(string who, int login)
{
    string result;
    object pl;
    int    tmp;
    int    t_in;
    int    t_out;

    if (objectp(pl = find_player(who)))
    {
	if (interactive(pl))
	{
	    tmp = time() - pl->query_login_time();
	    result = "Logged on    " + TIME2STR(tmp, 2);

	    /* Only list idle time if more than one minute. */
	    if ((tmp = query_idle(pl)) >= 60)
	    {
		result += "   " + TIME2STR(tmp, 2);
	    }
	}
	else
	{
	    tmp = time() - pl->query_login_time();
	    result = "Linkdead     " + TIME2STR(tmp, 2);
            tmp = time() - pl->query_linkdead();
            result += "   " + TIME2STR(tmp, 2);
	}
    }
    else
    {
	/* Get the time the file was last modified. If the file_time is
	 * false, it means that the name is not valid for a player.
	 */
	t_out = SECURITY->query_player_file_time(who);
	if (!t_out)
	{
	    return sprintf("%-12s No such player", capitalize(who));
	}

	/* Get a finger-player to get the login time, then clean out the
	 * finger-player again. We do not want to waste the memory.
	 */
	pl = SECURITY->finger_player(who);
	t_in = pl->query_login_time();
	pl->remove_object();

	/* This test checks whether the alleged duration of the last
	 * visit of the wizard does not exceed two days. If the wizard
	 * was reported to have been logged in for more than two days,
	 * this probably means that the playerfile was adjusted after
	 * the player last logged out. Ergo, the duration time would
	 * range from the moment the wizard logged in until the moment
	 * his/her playerfile was changed and that is naturally not
	 * something we want.
	 */
	if ((t_out - t_in) < 172800)
	{
	    tmp = time() - (login ? t_in : t_out);
	    result = " " + TIME2STR(tmp, 2);

	    tmp = t_out - t_in;
	    result += "  " + TIME2STR(tmp, 2);
	}
	else
	{
	    /* If the player's file has been changed, we use the login
	     * time since the logout time (ie the file time) is not
	     * correct.
	     */
	    tmp = time() - t_in;
	    result = " " + TIME2STR(tmp , 2) + "         -";
	}
    }

    return sprintf("%-12s ", capitalize(who)) + result;
}

nomask int
last(string str)
{
    int     login = 0;
    int     index;
    string *plist = ({ });

    CHECK_SO_WIZ;

    if (stringp(str))
    {
        str = lower_case(str);
	if (login = wildmatch("-i*", str))
	{
	    str = extract(str, 3);
	}
    }

    /* Get the wizards currently logged in. We have to test for strlen
     * again because the wizard may have given only '-i' as argument.
     */
    if (!strlen(str))
    {
    	plist = filter(users(), &->query_wiz_level())->query_real_name();
    }
    /* The name may be a class of wizards. */
    else if ((index = member_array(str, WIZ_N)) >= 0)
    {
    	plist = SECURITY->query_wiz_list(WIZ_R[index]);

	/* Ask for arches and get the keepers too. */
    	if (WIZ_R[index] == WIZ_ARCH)
    	{
    	    plist += SECURITY->query_wiz_list(WIZ_KEEPER);
    	}
    }
    /* The name may be the name of a domain. */
    else if (SECURITY->query_domain_number(str) > -1)
    {
    	plist = SECURITY->query_domain_members(str);
    }
    /* The name may be a mail alias. */
    else if (IS_MAIL_ALIAS(str))
    {
    	plist = EXPAND_MAIL_ALIAS(str);
    }
    /* The list of one or more players. */
    else
    {
    	plist = (explode(str, " ") - ({ "" }) );
    }

    if (login)
    {
	write("Who          Last login     Duration    Idle time\n");
	write("---          -----------    --------    ---------\n");
    }
    else
    {
	write("Who          Last logout    Duration    Idle time\n");
	write("---          -----------    --------    ---------\n");
    }

    write(implode(map(sort_array(plist), &last_check(, login)), "\n") + "\n");
    return 1;
}

/* **************************************************************************
 * localcmd - list my available commands
 */
nomask int
localcmd(string arg)
{
    return allcmd((stringp(arg) ? arg : "me") + " base");
}

/* **************************************************************************
 * metflag - Set the metflag, a wizard can decide if met or unmet with everyone
 */
nomask int
metflag(string str)
{
    CHECK_SO_WIZ;

    if (stringp(str))
    {
        notify_fail("Metflag what?\n");
        return 0;
    }

    this_interactive()->set_wiz_unmet(0);
    write("Ok.\n");
    return 1;
}

/* **************************************************************************
 * mudlist - List the muds known to us
 */
nomask int
mudlist(string arg)
{
    CHECK_SO_WIZ;
#ifdef UDP_MANAGER
    return UDP_MANAGER->cmd_mudlist(arg);
#else
    notify_fail("No udp manager active.\n");
    return 0;
#endif
}

/* **************************************************************************
 * newhooks - get new command hooks
 */
nomask int
newhooks(string str)
{
    object pl;

    CHECK_SO_WIZ;

    if ((WIZ_CHECK < WIZ_ARCH) ||
    	(!stringp(str)) ||
	(find_player(str) == this_interactive()))
    {
	write("Will update your command hooks.\n");
	this_interactive()->update_hooks();
	return 1;
    }

    pl = find_player(str);
    if (!objectp(pl))
    {
	pl = present(str, environment(this_interactive()));
    }
    if (!objectp(pl) ||
    	!living(pl))
    {
	pl = find_living(str);
    }
    if (!objectp(pl))
    {
	notify_fail("No such living object.\n");
	return 0;
    }
    pl->update_hooks();
    write("Updated command hooks for " + str + "(" + file_name(pl) + ").\n");
    return 1;
}

/* **************************************************************************
 * notify - Set whether wizard is to be notified of logins or not
 */

/*
 * Function name: notify_string
 * Description  : This function will return the notify flag in string form.
 * Arguments    : int nf - the notify level in int form.
 * Returns      : string - the notify level in string form.
 */
nomask string
notify_string(int nf)
{
    string nstring;

    nstring = "";

    if (nf & 1)
	nstring += "A";
    if (nf & 2)
	nstring += "W";
    if (nf & 4)
	nstring += "L";
    if (nf & 8)
	nstring += "D";
    if (nf & 16)
	nstring += "I";
    if (nf & 32)
	nstring += "X";
    if (nf & 64)
	nstring += "B";
    if (nf & 128)
	nstring += "S";

    return nstring;
}

nomask int
notify(string what)
{
    int i, nf;

    CHECK_SO_WIZ;

    nf = this_player()->query_notify();

    if (!strlen(what))
    {
        if (!nf)
        {
            write("You have the notifier turned off.\n");
	    return 1;
	}
	write("Your notification level is: " + notify_string(nf) + ".\n");
	return 1;
    }

    if ((what == "clear") ||
	(what == "O") ||
	(what == "off"))
    {
	if (!nf)
	{
	    write("The notifier was already shut off.\n");
	    return 1;
	}
	nf = 0;
    }
    else
    {
	if (what[0] == '+')
	{
	    what = what[1..];
	    if (strlen(what) < 3 || !SECURITY->exist_player(what))
	    {
		write("Nie ma postaci o tym imieniu.\n");
		return 1;
	    }
	    if (this_player()->query_notified(what))
	    {
		write("Osoba o tym imieniu juz jest wpisana na liste.\n");
		return 1;
	    }
	    if (!this_player()->add_notified(what))
	    {
		write("Zbyt duzo imion wpisanych do notify. By dodac " +
		    "nowe, musisz usunac jakies stare.\n");
		return 1;
	    }

	    write("Ok.\n");
	    return 1;
	}
	if (what[0] == '-')
	{
	    what = what[1..];
	    if (!this_player()->query_notified(what))
	    {
		/* Na wszelki wypadek, gdyby wartosc w mappingu == 0 */
		this_player()->remove_notified(what);
		write("Osoba o tym imieniu nie jest wpisana na liste.\n");
		return 1;
	    }
	    this_player()->remove_notified(what);
	    write("Ok.\n");
	    return 1;
	}
	if (what[0] == '?')
	{
	    write("Osoby wpisane na liste 'notify': " +
		COMPOSITE_WORDS(this_player()->query_notified()) + ".\n");
	    return 1;
	}

	for (i = 0; i < strlen(what); i++)
	{
	    switch (what[i])
	    {
	    case 'A':
		nf ^= 1;
		break;
	    case 'W':
		nf ^= 2;
		break;
	    case 'L':
		nf ^= 4;
		break;
	    case 'D':
		nf ^= 8;
		break;
	    case 'I':
		nf ^= 16;
		break;
	    case 'X':
		nf ^= 32;
		break;
	    case 'B':
		nf ^= 64;
		break;
	    case 'S':
		nf ^= 128;
		break;
	    default:
		write("Strange notify state: " + extract(what, i, i) + ".\n");
		break;
	    }
	}
    }

    this_interactive()->set_notify(nf);
    write("Ok.\n");
    return 1;
}

/* **************************************************************************
 * odmimie - Odmien swoje lub czyjes imie
 */
 
nomask int
odm_imie(string str)
{
    string *odmiana, *stare, imie;
    object ob;
    int x, y, len;
    
    if (str)
    {
        str = lower_case(str);
        
        if (wildmatch("*,*", str))
            ob = find_player(imie = (odmiana = explode(str, ","))[0]);
        else
            ob = find_player(imie = str);
        if (!ob)
        {
            write("Nie moge znalezc gracza o imieniu " + 
                capitalize(imie) + ".\n");
            return 1;
        }
        
        if (!sizeof(odmiana))
        {
            write("Oto odmiana imienia " + ob->query_name(1) + ":\n" +
                implode(ob->query_imiona(), ", ") + ".\n");
        }
        else
        {
            stare = ob->query_imiona();
            ob->remove_names(stare);
            
            if (sizeof(odmiana) != 6)
            {
                write("Odmiana musi zawierac 6 przypadkow oddzielonych " +
                    "przecinkiem.\n");
                return 1;
            }
            
            x = -1;
            while (++x < 6)
            {
                len = strlen(odmiana[x]); y = -1;
                while (++y < len && odmiana[x][y] == ' ')
                    ;
                
                if (y <= len)
                    odmiana[x] = odmiana[x][y..];
                    
                odmiana[x] = lower_case(odmiana[x]);
            }
            
            if (!ob->ustaw_imiona(odmiana))
            {
                write("Nie udalo mi sie ustawic odmiany.\n");
//                ob->ustaw_imiona(stare);
                return 1;
            }
            if (this_interactive() != ob && WIZ_CHECK < WIZ_ARCH)
                SECURITY->log_syslog("ODM_IMIE",
                sprintf("%s %10s -> %-10s \n Nowa odmiana: (%s)\n\n", 
                ctime(time()), this_interactive()->query_real_name(), 
                imie, str), -1);
                
            write("Ok, odmiana ustawiona.\n");
        }
    }
    else
    {
        write("Odmiana twojego imienia wyglada nastepujaco:\n" + 
            implode(this_interactive()->query_imiona(), ", ") + ".\n");
    }
    
    return 1;
}

/* **************************************************************************
 * odmprzym - Odmien swoje przymiotniki
 */

nomask int
odm_przym(string str)
{
    string *przym, *przym2, *przym3, przym4;
    int nr, x;

    CHECK_SO_WIZ;
    

    
    if (!str)
    {
        przym = (string *)this_interactive()->query_adjs();
        if (!(nr = sizeof(przym)))
        {
            write("Nie masz zadnych przymiotnikow zdefiniowanych.\n");
            return 1;
        }
        else write("Oto odmiana twoich przymiotnikow:\n");
        {
            nr /= 6;
            x = 1;
            while (x <= nr)
            {
                write(x + ": " + implode(przym[(x-1)*6..x*6], ", ") + ".\n");
                x++;
            }
            if (sizeof(przym)%6 != 0)
                write(x + ": " + implode(przym[(x-1)*6..], ", ") + ".\n");
        }
    }
    else
    {
        przym = explode(str, "|");
        przym3 = ({});
        for (x = 0; x < sizeof(przym); x++)
        {
            przym2 = explode(przym[x], ",");
            if(sizeof(przym2) != 6)
            {
                write("Zla liczba przypadkow w " +
                    LANG_SORD((x+1), PL_NAR, PL_MESKI_NOS_NZYW) +
                    " przymiotniku.\n");
                return 1;
            }
            przym3 += przym2;
        }
        przym = (string *)this_interactive()->query_adjs();
        this_interactive()->remove_adj(przym);
        if (!(this_interactive()->set_adj(przym3)))
        {
            write("Zbyt wiele przymiotnikow (maks. 2), lub sa one zbyt " +
                "dlugie.\n");
            this_interactive()->set_adj(przym);
            return 1;
        }
        write("Ok.\n");
    }
    return 1;
}

static mixed *old_rank = 0;
static int old_time = -1;
#define MIN_DIFF_TIME 300

/* **************************************************************************
 * ranking - Print a ranking list of the domains
 */

nomask int
rank_sort(mixed *elem1, mixed *elem2)
{
    if (elem1[1] < elem2[1]) return -1;
    if (elem1[1] == elem2[1]) return 0;
    return 1;
}

nomask int
ranking(string dom)
{
    string *doms, sg;
    mixed *mems; /* Array of array of members */
    int il, q, c, s, cmd, aft;

    CHECK_SO_WIZ;

    if (!sizeof(old_rank) || (time() - old_time) > MIN_DIFF_TIME)
    {
	doms = SECURITY->query_domain_list();
	for (mems = ({}), il = 0; il < sizeof(doms); il++)
	{
	    mems = mems + ({ ({ doms[il] }) +
			   SECURITY->query_domain_members(doms[il]) });
	}

	for (mems = ({}), il = 0; il < sizeof(doms); il++)
	{
	    q = SECURITY->query_domain_qexp(doms[il]);
	    c = SECURITY->query_domain_cexp(doms[il]);
	    cmd = SECURITY->query_domain_commands(doms[il]);
#ifdef DOMAIN_RANKWEIGHT_FORMULA
	    s = DOMAIN_RANKWEIGHT_FORMULA(q, c);
#else
	    s = 100 + (q / 25) + ((-c) / 10000);
#endif
	    mems += ({ ({ doms[il], (cmd * s) / 100,
			  s, cmd, q, c }) });
	}

	mems = sort_array(mems, rank_sort);

	old_rank = mems;
	old_time = time();
    }
    else
	mems = old_rank;

    if (stringp(dom))
    {
	if (sscanf(dom, "%d", aft) != 1)
	{
	    dom = capitalize(dom);
	    for (aft = (sizeof(mems) - 5), il = 0; il < sizeof(mems); il++)
		if (dom == mems[il][0])
		    aft = il;
	}
    }

    write("Ranking of domains:     Rank#     Weight   Base Rank    Quest   Combat\n-------------------\n");

    for (il = sizeof(mems) -1; il >= 0; il--)
    {
	q = sizeof(mems) - il;
	if ((!stringp(dom) && (q < 11)) ||
	    (stringp(dom) && (ABS(il - aft) < 3)) ||
	    dom == "All")
	{
	    sg = (mems[il][2] < 0 ? "-" + ABS(mems[il][2] / 100) : " " +
		ABS(mems[il][2] / 100));
	    write(sprintf("%2d: %-12s     %8d     %4s.%02d   %8d %8d %8d\n",
			  q,
			  mems[il][0], mems[il][1],
			  sg, ABS(mems[il][2] % 100),
			  mems[il][3], mems[il][4], mems[il][5]));
	}
    }

    return 1;
}

/* **************************************************************************
 * regret - regret an application to a domain
 */
nomask int
regret(string dom)
{
    CHECK_SO_WIZ;

    return SECURITY->regret_application(dom);
}

/* **************************************************************************
 * review - review move messages
 */
nomask int
review(string str)
{
    object tp;

    CHECK_SO_WIZ;

    if (!stringp(str))
	tp = this_interactive();
    else
	tp = find_player(str);

    if (!objectp(tp))
    {
	notify_fail("No such player: " + str +".\n");
	return 0;
    }

    write("mout:\t" + tp->query_m_out() +
	  "\nmin:\t" + tp->query_m_in() +
	  "\nmmout:\t" + tp->query_mm_out() +
	  "\nmmin:\t" + tp->query_mm_in() + "\n");
    return 1;
}

/* **************************************************************************
 * rsupport - show what another mud support
 */
nomask int
rsupport(string str)
{
    CHECK_SO_WIZ;
#ifdef UDP_MANAGER
    return UDP_MANAGER->cmd_support(str);
#else
    notify_fail("No udp manager active.\n");
    return 0;
#endif
}

/* **************************************************************************
 * rwho - show people on other muds
 */
nomask int
rwho(string str)
{
    CHECK_SO_WIZ;
#ifdef UDP_MANAGER
    return UDP_MANAGER->cmd_rwho(str);
#else
    notify_fail("No udp manager active.\n");
    return 0;
#endif
}

/* **************************************************************************
 * sanction - sanction actions
 */
nomask int
sanction(string str)
{
    CHECK_SO_WIZ;

    return SECURITY->sanction(str);
}

/* **************************************************************************
 * second - note someone as second.
 */
nomask int
second(string what)
{
    string *args, *seconds;
    int rval = 1;
    int size;
    object player;

    if (!stringp(what))
	what = "l l";

    args = explode(what, " ");
    size = sizeof(args);

    notify_fail("Zla skladnia. Obejrzyj '?second'.\n");

    switch (args[0])
    {
    case "a":
    case "add":
        if (size != 2)
            return 0;
            
	rval = this_player()->add_second(args[1]);
	if (!rval)
	{
	    write("Nie ma takiej postaci, albo nie jest ona graczem.\n");
	    return 1;
	}
	break;

    case "r":
    case "remove":
        if (size != 2)
            return 0;

	rval = this_player()->remove_second(args[1]);
	if (!rval)
	{
	    write("Nie masz takiego seconda.\n");
	    return 1;
	}
	
	break;

    case "l":
    case "list":
	if (size == 1)
	{
	    if (!sizeof(this_player()->query_seconds()))
		write("Nie masz zarejestrowanych zadnych secondow.\n");
	    else write("Zarejestrowales nastepujacych secondow: " +
		implode(this_player()->query_seconds(), " ") + "\n");
	    return 1;
	}
	else if (size == 2)
	{
	    args[1] = lower_case(args[1]);
	    player = find_object(args[1]);
	    if (!player)
		player = SECURITY->finger_player(args[1]);
	    if (!player)
	    {
		write("Nie ma postaci o imieniu " +
		    capitalize(args[1]) + ".\n");
		return 1;
	    }
	    
	    if (!SECURITY->query_wiz_rank(args[1]))
	    {
		write(capitalize(args[1]) + " nie jest czarodziejem.\n");
		return 1;
	    }
	    
	    seconds = player->query_seconds();
	    if (!seconds)
	    {
		write("Nie masz pojecia, jakich " + capitalize(args[1]) + 
		    " moze miec secondow.\n");
		return 1;
	    }
	    
	    if (!sizeof(seconds))
	    {
		write(capitalize(args[1]) + " nie ma zadnych secondow.\n");
		return 1;
	    }
	    
	    write(capitalize(args[1]) + " ma nastepujacych secondow: " +
		COMPOSITE_WORDS(seconds) + ".\n");
	    return 1;
	}

	break;
    default:
        return 0;	
    }

    write("Ok.\n");

    return 1;
}

/* **************************************************************************
 * setmout - set move out message.
 */
nomask int
set_m_out(string m)
{
    CHECK_SO_WIZ;

    if (!strlen(m))
    {
        write("Your m-out: " + this_interactive()->query_m_out() + "\n");
        return 1;
    }

    this_interactive()->set_m_out(m);
    write("M-out changed to: " + m + "\n");
    return 1;
}

/* **************************************************************************
 * setmin - set move in message.
 */
nomask int
set_m_in(string m)
{
    CHECK_SO_WIZ;

    if (!strlen(m))
    {
        write("Your m-in: " + this_interactive()->query_m_in() + "\n");
        return 1;
    }

    if (wildmatch("*[.!]", m))
    {
        notify_fail("Please observe that there should not be a period to " +
            "this text. Consult the help page if you are doubtful.\n");
        return 0;
    }

    this_interactive()->set_m_in(m);
    write("M-in changed to: " + m + "\n");
    return 1;
}

/* **************************************************************************
 * setmmout - set teleport out message.
 */
nomask int
set_mm_out(string m)
{
    CHECK_SO_WIZ;

    if (!strlen(m))
    {
        write("Your mm-out: " + this_interactive()->query_mm_out() + "\n");
        return 1;
    }

    this_interactive()->set_mm_out(m);
    write("MM-out changed to: " + m + "\n");
    return 1;
}

/* **************************************************************************
 * setmmin - set teleport in message.
 */
nomask int
set_mm_in(string m)
{
    CHECK_SO_WIZ;

    if (!strlen(m))
    {
        write("Your mm-in: " + this_interactive()->query_mm_in() + "\n");
        return 1;
    }

    this_interactive()->set_mm_in(m);
    write("MM-in changed to: " + m + "\n");
    return 1;
}

/* **************************************************************************
 * start - set start point.
 */
nomask int
start(string str)
{
    CHECK_SO_WIZ;

    if (!stringp(str))
    {
	write("Your default starting location: " +
	    this_interactive()->query_default_start_location() + "\n");
	return 1;
    }

    if (str == "valid")
    {
	str = implode(SECURITY->query_list_def_start(), ", ");
	write("Available starting locations:\n" + break_string(str, 76, 3)
	      + "\n");
	return 1;
    }

    if (str == "here")
    {
	str = file_name(environment(this_interactive()));
    }

    if (!this_interactive()->set_default_start_location(str))
    {
	write("<" + str + "> is not a valid starting location.\n");
	return 1;
    }
    
    write("Ok, od teraz startujesz w '" + str + "'.\n");
    return 1;
}

/* **************************************************************************
 * title - change the title
 */
nomask int
title(string t)
{
    CHECK_SO_WIZ;

    if (!stringp(t))
    {
	write("Your title is '" + this_interactive()->query_title() + "'.\n");
	return 1;
    }

    this_interactive()->set_title(t);
    write("Ok.\n");
    return 1;
}

/* **************************************************************************
 * unmetflag - Set the unmetflag, a wizard can decide: met/unmet with everyone
 */
nomask int
unmetflag(string str)
{
    CHECK_SO_WIZ;

    if (stringp(str) &&
	(str != "npc"))
    {
        notify_fail("Syntax: unmetflag [\"npc\"].\n");
        return 0;
    }

    this_interactive()->set_wiz_unmet(((str == "npc") ? 2 : 1));
    write("Ok.\n");
    return 1;
}

/* **************************************************************************
 * wizopt - Change/view the wizard options
 */
nomask int
wizopt(string arg)
{
    string *args;

    if (!stringp(arg))
    {
	wizopt("autopwd");
	return 1;
    }
    
    args = explode(arg, " ");
    if (sizeof(args) == 1)
    {
	switch(arg)
	{
	case "autopwd":
	case "pwd":
	    write("Auto pwd:     " + (this_player()->query_option(OPT_AUTO_PWD) ? "On" : "Off") + "\n");
	    break;

	default:
	    return notify_fail("Syntax error: No such wizard option.\n");
	    break;
	}
	return 1;
    }

    switch(args[0])
    {
    case "autopwd":
    case "pwd":
	if (args[1] == "on")
	    this_player()->set_option(OPT_AUTO_PWD, 1);
	else
	    this_player()->set_option(OPT_AUTO_PWD, 0);
	wizopt("autopwd");
	break;

    default:
	return notify_fail("Syntax error: No such wizard option.\n");
	break;
    }
    return 1;
}

/* **************************************************************************
 * whereis - Display the location of a player or living.
 */
nomask int
whereis(string str)
{
    object obj;
    object *live;
    object *dead;
    int    verbose;

    if (!stringp(str))
    {
	notify_fail("Whereis who?\n");
	return 0;
    }

    verbose = sscanf(lower_case(str), "-v %s", str);

    if (!objectp(obj = find_player(str)))
    {
	if (!objectp(obj = find_living(str)))
	{
	    notify_fail("No player or NPC '" + capitalize(str) + "' found.\n");
	    return 0;
	}

	write("No player '" + capitalize(str) +
	    "' logged in. We found an NPC though.\n");
    }

    if (!objectp(obj = environment(obj)))
    {
	write(capitalize(str) + " is sitting in the void.\n");
	return 1;
    }

    write("File: " + file_name(obj) + "\n");
    if (!verbose)
    {
	write("Room: " + obj->short(this_player()) + "\n");
	return 1;
    }

    write(obj->long(0));

    live = FILTER_LIVE(dead = all_inventory(obj));
    dead = FILTER_SHOWN(dead - live);
    if (sizeof(dead) &&
	strlen(str = COMPOSITE_DEAD(dead, 0)))
    {
	write(break_string((capitalize(str) + "."), 76) + "\n");
    }
    if (sizeof(live) &&
	strlen(str = COMPOSITE_LIVE(live, 0)))
    {
	write(break_string((capitalize(str) + "."), 76) + "\n");
    }
    return 1;
}

 /* **************************************************************************
 * whichsoul - Find out which soul defines a particular command.
 */

/*
 * Function name: print_whichsoul
 * Description  : Searches a type of souls for the command desired.
 * Arguments    : string *soul_list - the souls to search through.
 *                string cmd - the command to look for.
 *                string type - the type of soul.
 * Returns      : int - number of times the command was found.
 */
static nomask int
print_whichsoul(string *soul_list, string cmd, string type)
{
    int    index = -1;
    int    size = sizeof(soul_list);
    string soul_id;
    string *cmds;
    int    found = 0;

    while(++index < size)
    {
	cmds = m_indices(soul_list[index]->query_cmdlist());
	if (member_array(cmd, cmds) > -1)
	{
	    soul_id = soul_list[index]->get_soul_id();
	    soul_id = (strlen(soul_id) ? soul_id : "noname");
	    write(sprintf("%-7s  %-15s  %-s\n", type, soul_id,
		 soul_list[index]));
	    found++;
	}
    }

    return found;
}

int
whichsoul(string str)
{
    object target;
    string *cmds;
    int    found = 0;

    if (!stringp(str))
    {
	notify_fail("Syntax: whichsoul [<person>] <command>\n");
	return 0;
    }

    cmds = explode(str, " ");
    switch(sizeof(cmds))
    {
	case 1:
	    target = this_player();
	    break;

	case 2:
	    target = find_player(lower_case(cmds[0]));
	    if (!objectp(target))
	    {
		notify_fail("There is no player called " +
		    capitalize(cmds[0]) + " in the game.\n");
		return 0;
	    }
	    str = cmds[1];
	    break;

	default:
	    notify_fail("Too many arguments. Syntax: whichsoul " +
		"[<person>] <command>\n");
	    return 0;
    }

    write("Scanning for " + str + " on " + target->query_name() + ".\n");

    cmds = target->local_cmd();
    if (member_array(str, cmds) > -1)
    {
	write("Found at least once in local commands. (add_action's)\n");
    }

    write("Type     Soulname         Filename\n");

    found += print_whichsoul(target->query_wizsoul_list(), str, "wizard");
    found += print_whichsoul(target->query_tool_list(),    str, "tool");
    found += print_whichsoul(target->query_cmdsoul_list(), str, "command");

    if (!found)
    {
	write("Command " + str + " not found in any souls.\n");
    }

    return 1;
}
