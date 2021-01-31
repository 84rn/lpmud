/*
 * /cmd/wiz/lord.c
 *
 * This object holds the lord commands. Some commands may also be performed
 * by stewards. The following commands are supported:
 *
 * - accept  [stewards too]
 * - demote  [stewards too]
 * - deny    [stewards too]
 * - expel   [stewards too]
 * - liege
 * - liegee
 * - limit
 * - promote [stewards too]
 * - short
 * - trainee [stewards too]
 */

#pragma no_clone
#pragma no_inherit
#pragma save_binary
#pragma strict_types

inherit "/cmd/std/command_driver";

#include <flags.h>
#include <log.h>
#include <macros.h>
#include <std.h>
#include <stdproperties.h>

#define CHECK_SO_LORD 	 if (WIZ_CHECK < WIZ_LORD) return 0; \
			 if (this_interactive() != this_player()) return 0
#define CHECK_SO_STEWARD if (WIZ_CHECK < WIZ_STEWARD) return 0; \
			 if (this_interactive() != this_player()) return 0

/* **************************************************************************
 * Return a list of which souls are to be loaded.
 * The souls are listed in order of command search.
 */
nomask string *
get_soul_list()
{
    return ({ WIZ_CMD_LORD,
              WIZ_CMD_HELPER,
	      WIZ_CMD_NORMAL,
	      WIZ_CMD_APPRENTICE,
	      MBS_SOUL });
}

/* **************************************************************************
 * Return a proper name of the soul in order to get a nice printout.
 */
nomask string
get_soul_id()
{
    return WIZNAME_LORD;
}

/* **************************************************************************
 * The list of verbs and functions. Please add new in alfabetical order.
 */
nomask mapping
query_cmdlist()
{
    return ([
	     "accept":"accept",

	     "demote":"demote",
	     "deny":"deny",

	     "expel":"expel",

	     "liege":"liege",
	     "liegee":"liege",
	     "limit":"limit",

	     "promote":"promote",

	     "short":"short",

	     "trainee":"trainee",
	     ]);
}

/* **************************************************************************
 * Here follows the actual functions. Please add new functions in the 
 * same order as in the function name list.
 * **************************************************************************/

/* **************************************************************************
 * accept - accept someone into a domain.
 */
nomask int
accept(string name)
{
    CHECK_SO_STEWARD;

    return SECURITY->accept_application(name);
}

/* **************************************************************************
 * demote - demote a player to a lower level.
 */
nomask int
demote(string str)
{
    string name;
    int    level;

    CHECK_SO_STEWARD;

    if (!stringp(str))
    {
	notify_fail("Syntax: demote <name> <rank> / <level>\n");
	return 0;
    }

    /* Demotion to a new level with the same rank. */
    if (sscanf(str, "%s %d", name, level) == 2)
    {
	name = lower_case(name);
	if (level >= SECURITY->query_wiz_level(name))
	{
	    notify_fail("Demotions should be used to lower the level.\n");
	    return 0;
	}

	return SECURITY->wizard_change_level(name, level);
    }

    /* Demotion to a new rank. */
    if (sscanf(str, "%s %s", name, str) != 2)
    {
	notify_fail("Syntax: demote <name> <rank> / <level>\n");
	return 0;
    }

    if ((level = member_array(str, WIZ_N)) == -1)
    {
	if ((level = member_array(str, WIZ_S)) == -1)
	{
	    notify_fail("There is no rank called '" + str + "'.\n");
	    return 0;
	}
    }

    name = lower_case(name);
    level = WIZ_R[level];

    if (level >= SECURITY->query_wiz_rank(name))
    {
	notify_fail("Demotions should be used to lower the rank.\n");
	return 0;
    }

    return SECURITY->wizard_change_rank(name, level);
}

/* **************************************************************************
 * deny - deny the request from an apprentice.
 */
nomask int
deny(string name)
{
    CHECK_SO_STEWARD;

    return SECURITY->deny_application(name);
}

/* **************************************************************************
 * expel - expel a wizard from a domain.
 */
nomask int
expel(string wizname)
{
    CHECK_SO_STEWARD;

    /* We have to make sure the steward does not expel his Lord. */
    if ((SECURITY->query_wiz_rank(wizname) == WIZ_LORD) &&
	(WIZ_CHECK == WIZ_STEWARD))
    {
	notify_fail("You cannot expel your Lord from the domain.\n");
	return 0;
    }

    return SECURITY->expel_wizard_from_domain(wizname);
}

/* **************************************************************************
 * liege  - send a message on the liege-line.
 * liegee - emote a message on the liege-line.
 */
nomask int
liege(string str)
{
    int busy;

    CHECK_SO_LORD;

    if (!stringp(str))
    {
        notify_fail(capitalize(query_verb()) + " what?\n");
        return 0;
    }

    busy = this_interactive()->query_prop(WIZARD_I_BUSY_LEVEL);
    if (busy & BUSY_F)
    {
	write("WARNING: You are currently 'busy F'.\n");
    }
    else if (busy & BUSY_L)
    {
	write("WARNING: Your 'busy L' state is automatically switched off.\n");
	this_interactive()->add_prop(WIZARD_I_BUSY_LEVEL, (busy ^ BUSY_L));
    }

    return WIZ_CMD_APPRENTICE->line((WIZNAME_LORD + " " + str),
	(query_verb() == "liegee"), BUSY_L);
}

/* **************************************************************************
 * limit - limit the number of players in a domain.
 */
nomask int
limit(string str)
{
    string dname;
    string name;
    int    max;

    CHECK_SO_LORD;

    name = this_interactive()->query_real_name();
    dname = SECURITY->query_wiz_dom(name);

    /* No argument, default to the wizards own domain. */
    if (!stringp(str))
    {
	if (!strlen(dname))
	{
	    notify_fail("You are not in any domain. Strange!\n");
	    return 0;
	}

	str = dname;
    }

    /* Argument only a domain name: print the current limit for that domain. */
    if ((max = SECURITY->query_domain_max(str)) > 0)
    {
	write("Maximum number of members in " + capitalize(str) + " is " +
	    max + ".\n");
	return 1;
    }

    /* Lords may only set their own domain. */
    if (SECURITY->query_wiz_rank(name) <= WIZ_LORD)
    {
	if (sscanf(str, "%d", max) != 1)
	{
	    notify_fail("Syntax: limit <maximum>\n");
	    return 0;
	}

	if ((max > SECURITY->query_domain_max(dname)) &&
	    (max > SECURITY->query_default_domain_max()))
	{
	    notify_fail("You may not raise the limit beyond the default " +
		"maximum (" + SECURITY->query_default_domain_max() + ").\n");
	    return 0;
	}
    }
    else
    {
	if (sscanf(str, "%s %d", dname, max) != 2)
	{
	    notify_fail("Syntax: limit <domain> <maximum>\n");
	    return 0;
	}
    
	dname = capitalize(dname);
	if (SECURITY->query_dom_num(dname) == -1)
	{
	    notify_fail("No domain " + dname + ".\n");
	    return 0;
	}
    }

    if (sizeof(SECURITY->query_domain_members(dname)) > max)
    {
	notify_fail("Cannot set the maximum to " + max + " as there are " +
	    "already " + sizeof(SECURITY->query_domain_members(dname)) +
	    " wizards in " + str + ".\n");
	return 0;
    }

    if (SECURITY->set_domain_max(dname, max))
    {
	write("Maximum number of wizards for " + dname + " set to " + max +
	    ".\n");
    }
    else
    {
	write("Failed to set maximum number of wizards for " + dname +
	    " to " + max + ". This is impossible. Please report this.\n");
    }

    return 1;
}

/* **************************************************************************
 * promote - promote a wizard to a higher level.
 */
nomask int
promote(string str)
{
    string name;
    int    level;

    CHECK_SO_STEWARD;

    if (!strlen(str))
    {
	notify_fail("Syntax: promote <name> <rank> / <level>\n");
	return 0;
    }

    /* Promotion to a new level with the same rank. */
    if (sscanf(str, "%s %d", name, level) == 2)
    {
	name = lower_case(name);

	if (level <= SECURITY->query_wiz_level(name))
	{
	    notify_fail("Promotions should be used to raise the level.\n");
	    return 0;
	}

	return SECURITY->wizard_change_level(name, level);
    }

    /* Promotion to a new rank. */
    if (sscanf(str, "%s %s", name, str) != 2)
    {
	notify_fail("Syntax: promote <name> <rank> / <level>\n");
	return 0;
    }

    if ((level = member_array(str, WIZ_N)) == -1)
    {
	if ((level = member_array(str, WIZ_S)) == -1)
	{
	    notify_fail("There is no rank called '" + str + "'.\n");
	    return 0;
	}
    }

    name = lower_case(name);
    level = WIZ_R[level];

    if (level <= SECURITY->query_wiz_rank(name))
    {
	notify_fail("Promotions should be used to raise the rank.\n");
	return 0;
    }

    return SECURITY->wizard_change_rank(name, level);
}

/* **************************************************************************
 * short - set the short name of a domain.
 */
nomask int
short(string str)
{
    string dname;
    string name;
    string sname;
    int    size;

    CHECK_SO_LORD;

    name = this_interactive()->query_real_name();
    dname = SECURITY->query_wiz_dom(name);

    if (!stringp(str))
    {
	if (!strlen(dname))
	{
	    notify_fail("You are not in any domain. Strange!\n");
	    return 0;
	}

	str = dname;
    }

    if (strlen(sname = SECURITY->query_domain_short(str)))
    {
	write("Short name of " + capitalize(str) + " is " + sname + ".\n");
	return 1;
    }

    /* Lords may only set their own domain. */
    if (SECURITY->query_wiz_rank(name) == WIZ_LORD)
    {
	sname = str;
    }
    else
    {
	if (sscanf(str, "%s %s", dname, sname) != 2)
	{
	    notify_fail("Syntax: short <domain> <short>\n");
	    return 0;
	}

	dname = capitalize(dname);
	if (SECURITY->query_dom_num(dname) == -1)
	{
	    notify_fail("No domain " + dname + ".\n");
	    return 0;
	}
    }

    sname = lower_case(sname);
    if (strlen(sname) != 3)
    {
	notify_fail("The short name must be three letters long.\n");
	return 0;
    }

    /* Don't set the name if that name is already in use. */
    if (sizeof(filter(SECURITY->query_domain_list(),
	&operator(==)(sname) @ SECURITY->query_domain_short)))
    {
	notify_fail("The domain short name " + sname + " is already used.\n");
	return 0;
    }

    if (SECURITY->set_domain_short(dname, sname))
    {
	write("Short name for " + dname + " set to " + sname + ".\n");
    }
    else
    {
	write("Failed to set short name for " + dname + " to " + sname +
	    ". This is impossible. Please report this.\n");
    }

    return 1;
}
 
/* **************************************************************************
 * trainee - manage the list of trainees.
 */
nomask int
trainee(string str)
{
    string *words;

    CHECK_SO_STEWARD;

    /* No argument, perform the command 'finger trainees'. */
    if (!stringp(str))
    {
	return call_other(WIZ_CMD_APPRENTICE, "finger", "trainees");
    }

    words = explode(str, " ");
    if (sizeof(words) != 2)
    {
	notify_fail("Syntax: 'trainee a[dd] <name>' or " +
	    "'trainee r[emove] <name>'\n");
	return 0;
    }

    switch(words[0])
    {
    case "a":
    case "add":
	return SECURITY->add_trainee(words[1]);

    case "r":
    case "remove":
	return SECURITY->remove_trainee(words[1]);

    default:
	notify_fail("No subcommand '" + words[0] + "' to trainee.\n");
	return 0;
    }

    write("Fatal end of trainee() in " + MASTER + ". Please report this\n");
    return 1;
}
