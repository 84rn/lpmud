/*
 * /secure/master/guild.c
 *
 * This subpart of SECURITY contains the code related to the administration
 * of guilds for this mud. Having the necessary information in a central place
 * may make it easier to maintain a good overview.
 */

#include "/sys/const.h"
#include "/sys/language.h"
#include "/sys/log.h"

/*
 * Global variable. It is stored in the KEEPERSAVE.
 *
 * guilds: ([ (string) guild short name; lower case:
 *            ({ (string) guild long name; mixed capitation,
 *               (string) guild domain name; capitalized,
 *               (int) guild type; binary bits,
 *               (string) guild style; lower case,
 *               (string *) guild masters; lower case,
 *               (int) closed/testing/open,
 *            })
 *         ])
 */
private mapping guilds = ([ ]);

/*
 * Definitions.
 */
#define GUILD_LONG_NAME  0
#define GUILD_DOMAIN     1
#define GUILD_TYPE       2
#define GUILD_STYLE      3
#define GUILD_MASTERS    4
#define GUILD_PHASE      5

#define GUILD_DEVELOPMENT 0
#define GUILD_TESTING     1
#define GUILD_OPEN        2
#define GUILD_CLOSED      3

#define GUILD_PHASES ({ "development", "testing", "open", "closed" })

/*
 * Function name: load_guild_defaults
 * Description  : In case of a malfunction of the KEEPERSAVE restoration,
 *                this initialises the relevant data structures.
 */
static void
load_guild_defaults()
{
    guilds = ([ ]);
}

/*
 * Function name: query_guild_short_name
 * Description  : Find the short name for any long guild name given.
 * Arguments    : string long_name - the long name of the guild.
 * Returns      : string - the short name of the guild, else 0.
 */
public string
query_guild_short_name(string long_name)
{
    string *names = m_indices(guilds);
    int    size = sizeof(names);

    while(--size >= 0)
    {
	if (guilds[names[size]][GUILD_LONG_NAME] == long_name)
	{
	    return names[size];
	}
    }

    return 0;
}

/*
 * Function name: set_guild_long_name
 * Description  : This will set the long name for a guild that already
 *                exists. That it exists is not checked here, so that must
 *                be done prior to calling this function.
 * Arguments    : string short_name - the short name of the guild. Must be
 *                    lower case since it is an internal call.
 *                string long_name - the long name of the guild.
 */
static void
set_guild_long_name(string short_name, string long_name)
{
    guilds[short_name][GUILD_LONG_NAME] = long_name;
}

/*
 * Function name: query_guild_long_name
 * Description  : Query the long name of a particular guild. 
 * Arguments    : string short_name - the short name of the guild.
 * Returns      : string - the long name of the guild, else 0.
 */
public string
query_guild_long_name(string short_name)
{
    short_name = lower_case(short_name);

    if (pointerp(guilds[short_name]))
    {
	return guilds[short_name][GUILD_LONG_NAME];
    }

    return 0;
}

/*
 * Function name: set_guild_style
 * Description  : This will set the style for a guild that already exists.
 *                That it exists is not checked here, so that must be done
 *                prior to calling this function.
 * Arguments    : string short_name - the short name of the guild. Must be
 *                    lower case since it is an internal call.
 *                string style - the style of the guild.
 */
static void
set_guild_style(string short_name, string style)
{
    guilds[short_name][GUILD_STYLE] = style;
}

/*
 * Function name: query_guild_style
 * Description  : Query the style of a particular guild. 
 * Arguments    : string short_name - the short name of the guild.
 * Returns      : string - the style of the guild, else 0.
 */
public string
query_guild_style(string short_name)
{
    short_name = lower_case(short_name);

    if (pointerp(guilds[short_name]))
    {
	return guilds[short_name][GUILD_STYLE];
    }

    return 0;
}

/*
 * Function name: set_guild_phase
 * Description  : This will set the phase for a guild that already exists.
 *                That it exists is not checked here, so that must be done
 *                prior to calling this function.
 * Arguments    : string short_name - the short name of the guild. Must be
 *                    lower case since it is an internal call.
 *                string phase - the phase of the guild.
 */
static void
set_guild_phase(string short_name, string phase)
{
    int position;

    if ((position = member_array(phase, GUILD_PHASES)) != -1)
    {
	guilds[short_name][GUILD_PHASE] = position;
    }
}

/*
 * Function name: query_guild_phase
 * Description  : Query the phase of a particular guild. 
 * Arguments    : string short_name - the short name of the guild.
 * Returns      : string - the phase of the guild, else 0.
 */
public string
query_guild_phase(string short_name)
{
    short_name = lower_case(short_name);

    if (pointerp(guilds[short_name]))
    {
	return GUILD_PHASES[guilds[short_name][GUILD_PHASE]];
    }

    return 0;
}

/*
 * Function name: set_guild_type
 * Description  : This will set the type for a guild that already exists.
 *                That it exists is not checked here, so that must be done
 *                prior to calling this function.
 * Arguments    : string short_name - the short name of the guild. Must be
 *                    lower case since it is an internal call.
 *                int type - the type of the guild, a binary conjunction of
 *                    G_RACE, G_LAYMAN and/or G_OCCUPATIONAL.
 */
static void
set_guild_type(string short_name, int type)
{
    guilds[short_name][GUILD_TYPE] = type;
}

/*
 * Function name: query_guild_type
 * Description  : Query the type of a particular guild. This is a conjunction
 *                of G_RACE, G_LAYMAN and/or G_OCCUPATIONAL.
 * Arguments    : string short_name - the short name of the guild.
 * Returns      : int - the type of the guild, else 0.
 */
public int
query_guild_type(string short_name)
{
    short_name = lower_case(short_name);

    if (pointerp(guilds[short_name]))
    {
	return guilds[short_name][GUILD_TYPE];
    }

    return 0;
}

/*
 * Function name: query_guild_type_int
 * Description  : Converts a string with the letters of the guild type into
 *                the equivalent number.
 * Arguments    : string type - the type, i.e. "LO".
 * Returns      : int type - the resulting binary type.
 */
public int
query_guild_type_int(string type)
{
    string *parts;
    int    size;
    int    result = 0;

    if (!strlen(type))
    {
	return 0;
    }

    parts = explode(lower_case(type), "");
    size = sizeof(parts);
    while(--size >= 0)
    {
	switch(parts[size])
	{
	case "r":
	    result |= G_RACE;
	    break;

	case "l":
	    result |= G_LAYMAN;
	    break;

	case "o":
	    result |= G_OCCUPATIONAL;
	    break;
	}
    }

    return result;
}

/*
 * Function name: query_guild_type_string
 * Description  : Query the type of a particular guild. The result is a string
 *                with one letter for each registered type.
 * Arguments    : int type - the type to convert.
 * Returns      : string - the type of the guild, else "".
 */
public string
query_guild_type_string(int type)
{
    string result = "";

    if (type & G_RACE)
    {
	result += "R";
    }
    if (type & G_LAYMAN)
    {
	result += "L";
    }
    if (type & G_OCCUPATIONAL)
    {
	result += "O";
    }

    return result;
}

/*
 * Function name: query_guild_type_long_string
 * Description  : Query the type of a particular guild. The result is a string
 *                with the full type name for each registered type. The list
 *                is capitalized and comma separated.
 * Arguments    : int type - the type to convert.
 * Returns      : string - the type of the guild, else "".
 */
public string
query_guild_type_long_string(int type)
{
    string *result = ({ });

    if (type & G_RACE)
    {
	result += ({ "Race" });
    }
    if (type & G_LAYMAN)
    {
	result += ({ "Layman" });
    }
    if (type & G_OCCUPATIONAL)
    {
	result += ({ "Occupational" });
    }

    return implode(result, ", ");
}

/*
 * Function name: set_guild_domain
 * Description  : This will set the domain for a guild that already exists.
 *                That it exists is not checked here, so that must be done
 *                prior to calling this function.
 * Arguments    : string short_name - the short name of the guild. Must be
 *                    lower case since it is an internal call.
 *                string domain - the domain the guild is in (capitalized).
 */
static void
set_guild_domain(string short_name, string domain)
{
    guilds[short_name][GUILD_DOMAIN] = domain;
}

/*
 * Function name: query_guild_domain
 * Description  : Query the domain a particular guild is in.
 * Arguments    : string short_name - the short name of the guild.
 * Returns      : string - the domain the guild is in, else 0.
 */
public string
query_guild_domain(string short_name)
{
    short_name = lower_case(short_name);

    if (pointerp(guilds[short_name]))
    {
	return guilds[short_name][GUILD_DOMAIN];
    }

    return 0;
}

/*
 * Function name: add_guild_master
 * Description  : This will add a guildmaster to a guild that already exists.
 *                That it exists is not checked here, so that must be done
 *                prior to calling this function.
 * Arguments    : string short_name - the short name of the guild. Must be
 *                    lower case since it is an internal call.
 *                string master - the guildmaster to add.
 */
static void
add_guild_master(string short_name, string master)
{
    if (member_array(master, guilds[short_name][GUILD_MASTERS]) == -1)
    {
	guilds[short_name][GUILD_MASTERS] += ({ master });
    }
}

/*
 * Function name: remove_guild_master
 * Description  : This will remove a guildmaster from a guild that already
 *                exists. That it exists is not checked here, so that must
 *                be done prior to calling this function.
 * Arguments    : string short_name - the short name of the guild. Must be
 *                    lower case since it is an internal call.
 *                string master - the guildmaster to remove.
 */
static void
remove_guild_master(string short_name, string master)
{
    guilds[short_name][GUILD_MASTERS] -= ({ master });
}

/*
 * Function name: query_guild_masters
 * Description  : Query the registered guildmasters of a particular guild. 
 *                The answer will be sorted.
 * Arguments    : string short_name - the short name of the guild.
 * Returns      : string * - the guildmasters of the guild, or ({ }).
 */
public string *
query_guild_masters(string short_name)
{
    short_name = lower_case(short_name);

    if (pointerp(guilds[short_name]))
    {
	return secure_var(sort_array(guilds[short_name][GUILD_MASTERS]));
    }

    return ({ });
}

/*
 * Function name: query_guilds
 * Description  : Returns a list of the short names of all guilds. The answer
 *                will be sorted.
 * Returns      : string * - the list of short names.
 */
public string *
query_guilds()
{
    return sort_array(m_indices(guilds));
}

/*
 * Function name: guild_sort_styles
 * Description  : Sort function that sorts the guild names based on the
 *                style.
 * Arguments    : string guild1 - the short name of guild 1.
 *                string guild2 - the short name of guild 2.
 * Returns      : int - the sort answer -1/0/1.
 */
public int
guild_sort_styles(string guild1, string guild2)
{
    string style1 = query_guild_style(guild1);
    string style2 = query_guild_style(guild2);

    if (style1 == style2)
    {
	return 0;
    }
    if (style1 < style2)
    {
	return -1;
    }

    return 1;
}

/*
 * Function name: guild_filter_types
 * Description  : Filters all guilds of a particular type.
 * Arguments    : string short_name - the name of the guild to test.
 *                int type - the bit of the type to check.
 * Returns      : int 1/0 - passed or failed the filter test.
 */
public int
guild_filter_type(string short_name, int type)
{
    return (pointerp(guilds[short_name]) &&
	(guilds[short_name][GUILD_TYPE] & type));
}

/*
 * Function name: guild_command
 * Description  : This contains the code of the "guild" command for wizards.
 *                It may only be called from the apprentice soul.
 * Arguments    : string str - the command line argument.
 * Returns      : int 1/0 - success/failure.
 */
public int
guild_command(string str)
{
    string *args;
    int    num_args;
    string guild;
    string *names;
    int    num_guilds;
    string name;
    string domain;
    int    rank;
    int    index;
    int    type;

    /* Access failure. May only be called from apprentice soul. */
    if (file_name(previous_object()) != WIZ_CMD_APPRENTICE)
    {
	write("Call from illegal object. Must be apprentice soul.\n");
	return 0;
    }

    /* Default to "guild list" if there is no argument. */
    if (!strlen(str))
    {
	str = "list";
    }

    args = explode(str, " ");
    num_args = sizeof(args);
    name = this_interactive()->query_real_name();
    rank = query_wiz_rank(name);

    /* If no guilds are listed, all you can do is add. */
    if ((args[0] != "add") &&
	!m_sizeof(guilds))
    {
	write("No guilds registered. Add a guild first.\n");
	return 1;
    }

    switch(args[0])
    {

    /*
     * Subcommand: add [<domain>] <guild> <type> <style> <wizard> <long>
     */
    case "add":
	switch(rank)
	{
	case WIZ_STEWARD:
	case WIZ_LORD:
	    domain = query_wiz_dom(name);
	    break;

	case WIZ_ARCH:
	case WIZ_KEEPER:
	    domain = capitalize(args[1]);
	    if (query_domain_number(domain) < 0)
	    {
		notify_fail("\"" + domain +
		    "\" is not a valid domain name.\n");
		return 0;
	    }

	    args = exclude_array(args, 1, 1);
	    num_args--;
	    break;

	default:
	    notify_fail("Only the Liege or steward of your domain may " +
		"add a guild to the list. Admins will do, too.\n");
	    return 0;
	}

	if (num_args < 6)
	{
	    notify_fail("Too few arguments to \"guild add\".\n");
	    return 0;
	}

	guild = lower_case(args[1]);
	if (pointerp(guilds[guild]))
	{
	    notify_fail("A guild by the name of \"" + capitalize(guild) +
		"\" already exists.\n");
	    return 0;
	}
	else if (strlen(guild) > 10)
	{
	    notify_fail("Ten letters is long enough for a short name. " +
		"Please be more brief in your next attempt.\n");
	    return 0;
	}

	if (!(type = query_guild_type_int(args[2])))
	{
	    notify_fail("Guild type \"" + args[2] + "\" does not contain " +
		"any of R/L/O.\n");
	    return 0;
	}

	args[4] = lower_case(args[4]);
	if (query_wiz_rank(args[4]) < WIZ_NORMAL)
	{
	    notify_fail("The intended guildmaster \"" + capitalize(args[4]) +
		"\" must be a domain wizard.\n");
	    return 0;
	}

	/* Create the guild, save the master and return feedback. */
	guilds[guild] = ({ implode(args[5..], " "), domain, type,
	    lower_case(args[3]), ({ args[4] }), GUILD_DEVELOPMENT });
	save_master();

#ifdef GUILD_CMD_LOG
        this_object()->log_syslog(GUILD_CMD_LOG, ctime(time()) +
	    sprintf(" %-11s adds %s / %s.\n", capitalize(name),
	    capitalize(guild), query_guild_long_name(guild)));
#endif GUILD_CMD_LOG

	/* Give verbose feedback about the guild that was added. */
	write("Added guild with following information:\n");
	return guild_command("info " + guild);

    /*
     * Subcommand: info <guild>
     */
    case "info":
	switch(num_args)
	{
	case 2:
	    guild = lower_case(args[1]);
	    if (!pointerp(guilds[guild]))
	    {
		notify_fail("There is no guild named \"" + capitalize(guild) +
		    "\".\n");
		return 0;
	    }

	    write("Domain      : " + query_guild_domain(guild) + " (Phase: " +
		capitalize(query_guild_phase(guild)) + ")\n");
	    write("Short & Long: " + capitalize(guild) + "; " +
		query_guild_long_name(guild) + "\n");
	    type = guilds[guild][GUILD_TYPE];
	    write("Style & Type: " + capitalize(query_guild_style(guild)) +
		"; " + query_guild_type_long_string(type) + "\n");
	    names = map(query_guild_masters(guild), capitalize);
	    write("Guildmasters: " +
		(sizeof(names) ? COMPOSITE_WORDS(names) : "NONE!") + "\n");
	    return 1;

	default:
	    notify_fail("Too many arguments to \"guild info\".\n");
	    return 0;
	}

	/* Not reached. */

    /*
     * Subcommand: list [all]
     *             list short
     *             list styles
     *             list R/L/O
     */
    case "list":
	switch(num_args)
	{
	case 1:
	    args += ({ "all" });
	    num_args++;

	    /* Intentional fall through to next section. */

	case 2:
	    names = query_guilds();
	    num_guilds = sizeof(names);

	    switch(lower_case(args[1]))
	    {
	    case "all":
		/* No sorting. */
		break;

	    case "r":
	    case "l":
	    case "o":
		type = query_guild_type_int(args[1]);
		names = filter(names, &guild_filter_type(, type));
		num_guilds = sizeof(names);
		break;

	    case "short":
		write(sprintf("The following guilds are listed:\n\n%-80#s\n",
		    implode(query_guilds(), "\n")));
		return 1;

	    case "styles":
		names = sort_array(names, guild_sort_styles);
		break;

	    default:
		notify_fail("There is no subcommand \"" + args[1] +
		    "\" for \"guild list\".\n");
		return 0;
	    }

	    index = -1;

	    write("Short name P TYP Style   Long " +
		"name                           Guildmaster(s)\n\n");

	    while(++index < num_guilds)
	    {
		str = implode(map(query_guild_masters(names[index]),
		    capitalize), " ");
		write(sprintf("%-10s %-1s %-3s %-7s %-35s %-1s\n",
		    names[index],
		    capitalize(query_guild_phase(names[index]))[0..0],
		    query_guild_type_string(query_guild_type(names[index])),
		    query_guild_style(names[index]),
		    query_guild_long_name(names[index]),
		    strlen(str) ? str : "NONE!"));
	    }

	    return 1;

	default:
	    notify_fail("Too many arguments to \"guild list\".\n");
	    return 0;
	}

	/* Not reached. */

    /*
     * Subcommand: master <guild> add/remove [<wizard>]
     */
    case "master":
	switch(num_args)
	{
	case 3:
	    /* If you want to add or remove yourself, the name is appended
	     * automatically.
	     */
	    args += ({ name });
	    num_args++;

	    /* Intentional fall through to next section. */

	case 4:
	    guild = lower_case(args[1]);
	    if (!pointerp(guilds[guild]))
	    {
		notify_fail("There is no guild named \"" + capitalize(guild) +
		    "\".\n");
		return 0;
	    }

	    switch(rank)
	    {
	    case WIZ_NORMAL:
	    case WIZ_MAGE:
		if (lower_case(args[3]) != name)
		{
		    notify_fail("You may only add/remove yourself as " +
			"guildmaster.\n");
		    return 0;
		}

		/* Intentional fall through to next section. */

	    case WIZ_STEWARD:
	    case WIZ_LORD:
		if (query_guild_domain(guild) != query_wiz_dom(name))
		{
		    notify_fail("Your powers extend to " +
			query_wiz_dom(name) + " only, while the " +
			capitalize(guild) + " is administered by " +
			query_guild_domain(guild) + ".\n");
		    return 0;
		}

		break;

	    case WIZ_ARCH:
	    case WIZ_KEEPER:
		break;

	    default:
		notify_fail("Wizards that are not in any domain cannot " +
		    "be a guildmaster. If you are still registered, then " +
		    "ask the Liege of the domain to relieve you.\n");
		return 0;
	    }

	    args[3] = lower_case(args[3]);

	    switch(args[2])
	    {
	    case "add":
		if (query_wiz_rank(args[3]) < WIZ_NORMAL)
		{
		    notify_fail(capitalize(args[3]) +
			" is not a wizard in any domain.\n");
		    return 0;
		}

		add_guild_master(guild, args[3]);
		write("Added " + capitalize(args[3]) + " as guildmaster to " +
		    capitalize(guild) + ".\n");
		break;

	    case "remove":
		remove_guild_master(guild, args[3]);
		write("Removed " + capitalize(args[3]) +
		    " as guildmaster from " + capitalize(guild) + ".\n");
		break;

	    default:
		notify_fail("One can only add or remove a guildmaster.\n");
		return 0;
	    }

	    /* Save the master, update the log, and we are done. */
	    save_master();

#ifdef GUILD_CMD_LOG
           this_object()->log_syslog(GUILD_CMD_LOG, ctime(time()) +
		sprintf(" %-11s %ss %s to/from %s.\n", capitalize(name),
		args[2], capitalize(args[3]), capitalize(guild)));
#endif GUILD_CMD_LOG

	    return 1;

	default:
	    notify_fail("Incorrect number of arguments for " +
		"\"guild master\".\n");
	    return 0;
	}

	/* Not reached. */

    /*
     * Subcommand: phase <guild> <phase>
     */
    case "phase":
	switch(num_args)
	{
	case 3:
	    guild = lower_case(args[1]);
	    if (!pointerp(guilds[guild]))
	    {
		notify_fail("There is no guild named \"" + capitalize(guild) +
		    "\".\n");
		return 0;
	    }

	    args[2] = lower_case(args[2]);
	    if (member_array(args[2], GUILD_PHASES) == -1)
	    {
		notify_fail("The phase \"" + args[2] + "\" is not valid.\n");
	    }

	    switch(rank)
	    {
	    case WIZ_ARCH:
	    case WIZ_KEEPER:
		break;

	    case WIZ_STEWARD:
	    case WIZ_LORD:
		if (query_guild_domain(guild) == query_wiz_dom(name))
		{
		    break;
		}

		/* Intentionally fall through to default case. */

	    default:
		if (member_array(name, guilds[guild][GUILD_MASTERS]) == -1)
		{
		    notify_fail("The guild \"" + capitalize(guild) +
			"\" is not in your domain, nor are you a " +
			"registered guildmaster, Liege or administrator.\n");
		    return 0;
		}

		break;
	    }

	    /* Typing only a part of the phase will suffice. */
	    if (member_array(args[2], GUILD_PHASES) == -1)
	    {
		index = sizeof(GUILD_PHASES);
		type = 1;
		while(--index >= 0)
		{
		    if (GUILD_PHASES[index][0 .. (strlen(args[2]) - 1)] ==
			args[2])
		    {
			args[2] = GUILD_PHASES[index];
			type = 0;
			break;
		    }
		}

		if (type)
		{
		    notify_fail("There is no phase \"" + args[2] + "\".\n");
		    return 0;
		}
	    }

	    /* Set the phase, save the master, log and print feedback. */
	    set_guild_phase(guild, args[2]);
	    save_master();

#ifdef GUILD_CMD_LOG
            this_object()->log_syslog(GUILD_CMD_LOG, ctime(time()) +
		sprintf(" %-11s sets phase %s on %s.\n", capitalize(name),
		capitalize(args[2]), capitalize(guild)));
#endif GUILD_CMD_LOG

	    write("Set phase \"" + capitalize(args[2]) + "\" on \"" +
		capitalize(guild) + "\".\n");
	    return 1;

	default:
	    notify_fail("Incorrect number of arguments for " +
		"\"guild phase\".\n");
	    return 0;
	}

	/* Not reached. */

    /*
     * Subcommand: remove <guild>
     */
    case "remove":
	switch(num_args)
	{
	case 2:
	    guild = lower_case(args[1]);
	    if (!pointerp(guilds[guild]))
	    {
		notify_fail("There is no guild named \"" + capitalize(guild) +
		    "\".\n");
		return 0;
	    }

	    switch(rank)
	    {
	    case WIZ_STEWARD:
	    case WIZ_LORD:
		if (query_guild_domain(guild) != query_wiz_dom(name))
		{
		    notify_fail("Your powers extend to " +
			query_wiz_dom(name) + " only, while the " +
			capitalize(guild) + " is administered by " +
			query_guild_domain(guild) + ".\n");
		    return 0;
		}

		/* Intentional fall through to admin section. */

	    case WIZ_ARCH:
	    case WIZ_KEEPER:
		/* Remove the entry, save the master and give feedback. */
		guilds = m_delete(guilds, guild);
		save_master();

#ifdef GUILD_CMD_LOG
                this_object()->log_syslog(GUILD_CMD_LOG, ctime(time()) +
		    sprintf(" %-11s removes %s.\n", capitalize(name),
		    capitalize(guild)));
#endif GUILD_CMD_LOG

		write("Removed entry for \"" + capitalize(guild) +
		    "\" guild.\n");
		return 1;

	    default:
		notify_fail("Only the Liege or steward of your domain may " +
		    "remove a guild from the list. Admins will do, too.\n");
		return 0;
	    }

	default:
	    notify_fail("Too many arguments to \"guild remove\".\n");
	    return 0;
	}

	/* Not reached. */

    default:
	notify_fail("There is no subcommand \"" + args[0] +
	    "\" for \"guild\".\n");
	return 0;
    }

    write("Fatal end of switch() in guild() in SECURITY. Report this!\n");
    return 1;
}
