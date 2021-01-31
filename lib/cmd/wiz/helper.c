/*
 * /cmd/wiz/helper.c
 *
 * This special soul contains some commands useful for wizards with a
 * helper function. The following commands are supported:
 *
 * - pinfo
 */

#pragma no_clone
#pragma no_inherit
#pragma strict_types
#pragma save_binary

inherit "/cmd/std/command_driver";

#include <macros.h>
#include <std.h>

#define ALLOWED_WIZARD_FILE "/cmd/wiz/helpers"
#define ALLOWED_LIEGE_COMMANDS ({ "pinfo" })
#define PINFO_EDIT_DONE     "pinfo_edit_done"
#define PINFO_WRITE_DONE    "pinfo_write_done"

#define CHECK_ALLOWED       \
    if (!may_use_command()) \
    {                       \
        return 0;           \
    }

/*
 * Global variables.
 */
private static mapping allowed_wizards = ([ ]);
private static mapping pinfo_edit = ([ ]);

/*
 * Prototype.
 */
string strip_comments(string text);

/*
 * Function name: create
 * Description  : Constructor. Called at creation.
 */
nomask void
create()
{
    string *lines;
    int    index;
    string *args;

    SECURITY->set_helper_soul_euid();

    if (file_size(ALLOWED_WIZARD_FILE) != -1)
    {
        lines = explode(strip_comments(read_file(ALLOWED_WIZARD_FILE)), "\n") -
            ({ "" });
        index = sizeof(lines);
        while(--index >= 0)
        {
            args = explode(lower_case(lines[index]), " ") - ({ "" });
            allowed_wizards[args[0]] = args[1..];
        }
    }
}

/*
 * Function name: get_soul_id
 * Description  : Return a proper name of the soul in order to get a nice
 *                printout. People can also use this name with the "allcmd"
 *                command.
 * Returns      : string - the name.
 */
nomask string
get_soul_id()
{
    return "helper";
}

/*
 * Function name: query_cmd_soul
 * Description  : Identify this as a command soul.
 * Returns      : int 1 - always.
 */
nomask int
query_cmd_soul()
{
    return 1;
}

/*
 * Function name: using_soul
 * Description  : Called when a wizard hooks onto the soul.
 * Arguments    : object wizard - the wizard hooking onto the soul.
 */
public void
using_soul(object wizard)
{
    SECURITY->set_helper_soul_euid();
}

/*
 * Function name: query_cmdlist
 * Description  : The list of verbs and functions. Please add new in
 *                alphabetical order.
 * Returns      : mapping - ([ "verb" : "function name" ])
 */
nomask mapping
query_cmdlist()
{
    return ([
        "pinfo" : "pinfo",
        ]);
}

/*
 * Function name: may_use_command
 * Description  : Tests whether a particular wizard may use a the command.
 *                Arches++ can use all. The function operates on
 *                this_interactive() and query_verb().
 * Returns      : int 1/0 - allowed/disallowed. 
 */
nomask int
may_use_command(string verb = query_verb())
{
    string name = this_interactive()->query_real_name();

    /* No name, or no verb, no show. */
    if (!strlen(verb) ||
	!strlen(name))
    {
	return 0;
    }

    switch(SECURITY->query_wiz_rank(name))
    {
	case WIZ_ARCH:
	case WIZ_KEEPER:
	    /* Arches and keepers do all. */
	    return 1;

	case WIZ_LORD:
	    /* Lieges have some special commands. */
            if (member_array(verb, ALLOWED_LIEGE_COMMANDS) >= 0)
	    {
		/* Set the euid of the object right. */
		return 1;
	    }

	    /* Intentionally no break; */

	default:
	    /* Wizard may have been allowed on the commands. */
            if (member_array(verb, allowed_wizards[name]) >= 0)
	    {
		/* Set the euid of the object right. */
		return 1;
	    }
    }

    /* All others, no show. */
    return 0;
}

/*
 * Function name: strip_comments
 * Description  : Take a text and strip all comments from that file in the
 *                form "slash asterisk ... asterisk slash". It will assume
 *                that the comment-markers are always paired.
 * Arguments    : string text - the text to strip comments from.
 * Returns      : string - the text without comments.
 */
public string
strip_comments(string text)
{
    string left;
    string middle;
    string right;

    /*
     * See whether there is a begin marker /*. If so, then we parse out the
     * start-end combination for comments, and everything in between.
     */
    while(wildmatch("*/\\**", text))
    {
	sscanf(("$" + text + "$"), "%s/*%s*/%s", left, middle, right);
	text = extract((left + right), 1, -2);
    }

    return text;
}

/* ***************************************************************************
 * PINFO: Edit/view the information file on a player.
 */

/*
 * Function name: pinfo_write_done
 * Description  : Called from the editor when the wizard is done writing
 *                the text for the file on the player.
 * Arguments    : string text - the text to add to the file.
 */
public void
pinfo_write_done(string text)
{
    string wname = this_player()->query_real_name();

    if (MASTER_OB(previous_object()) != EDITOR_OBJECT)
    {
	write("Illegal call to pinfo_edit_done().\n");
	return;
    }

    if (!stringp(pinfo_edit[wname]))
    {
	write("No pinfo_edit information. Impossible! Please report!\n");
	return;
    }

    if (!strlen(text))
    {
        write("Pinfo aborted.\n");
        return;
    }

    /* Make sure we have the proper euid. */
    SECURITY->set_helper_soul_euid();

    write_file(pinfo_edit[wname], ctime(time()) + " " + capitalize(wname) +
        " (" + capitalize(WIZ_RANK_NAME(SECURITY->query_wiz_rank(wname))) +
	"):\n" + text + "\n");
    pinfo_edit = m_delete(pinfo_edit, wname);
    write("Information saved.\n");
}

/*
 * Function name: pinfo_edit_done
 * Description  : Called from the editor when the wizard is done editing
 *                the text for the file on the player.
 * Arguments    : string text - the text to add to the file.
 */
public void
pinfo_edit_done(string text)
{
    string wname = this_player()->query_real_name();

    if (MASTER_OB(previous_object()) != EDITOR_OBJECT)
    {
	write("Illegal call to pinfo_edit_done().\n");
	return;
    }

    if (!stringp(pinfo_edit[wname]))
    {
	write("No pinfo_edit information. Impossible! Please report!\n");
	return;
    }

    if (!strlen(text))
    {
        write("Pinfo aborted.\n");
        return;
    }

    /* Make sure we have the proper euid. */
    SECURITY->set_helper_soul_euid();

    rm(pinfo_edit[wname]);
    write_file(pinfo_edit[wname], text + "\n" + ctime(time()) + " " +
	capitalize(wname) + " (" +
	capitalize(WIZ_RANK_NAME(SECURITY->query_wiz_rank(wname))) +
	"):\nRe-edited the previous text.\n\n");
    pinfo_edit = m_delete(pinfo_edit, wname);
    write("Information saved.\n");
}

/*
 * Nazwa funkcji: pinfo_info
 * Opis         : Na potrzeby fingera, sprawdza, czy istnieje pinfo o
 *                fingerowanym graczu, ktore this_player() moze przeczytac.
 * Argumenty:   : name - imie fingerowanego
 */
public nomask int
pinfo_info(string name)
{
    string wname = this_player()->query_real_name();

    SECURITY->set_helper_soul_euid();

    return file_name(previous_object()) == WIZ_CMD_APPRENTICE &&
           may_use_command("pinfo") &&
           (!(SECURITY->query_wiz_rank(name)) ||
            (SECURITY->query_wiz_rank(wname) == WIZ_LORD &&
             SECURITY->query_wiz_dom(wname) ==
                 SECURITY->query_wiz_dom(name)) ||
            SECURITY->query_wiz_rank(wname) >= WIZ_ARCH) &&
           file_size(PINFO_FILE(name)) > 0;
}

nomask int
pinfo(string str)
{
    string *args;
    string name;
    string wname = this_player()->query_real_name();
    int    rank = SECURITY->query_wiz_rank(wname);
    string cmd;
    string text;
    string file;
    object editor;

    CHECK_ALLOWED;

    if (!strlen(str))
    {
	notify_fail("Syntax: pinfo [r / t / w / d / e] <name> [<text>]\n");
	return 0;
    }

    args = explode(str, " ");
    args = ((sizeof(args) == 1) ? ( ({ "r" }) + args) : args);

    cmd = args[0];
    name = lower_case(args[1]);
    if (sizeof(args) > 2)
    {
	text = implode(args[2..], " ");
    }

    /*
     * Access check. The following applies:
     *
     * - arches/keepers can do all.
     * - lieges can only access information on their subject wizards.
     * - people allowed for the command can handle mortal players.
     */
    switch(rank)
    {
    case WIZ_ARCH:
    case WIZ_KEEPER:
        /* They can do all. */
	break;

    case WIZ_LORD:
        /* Can only handle their subject wizards. */
	if ((SECURITY->query_wiz_dom(wname) ==
	     SECURITY->query_wiz_dom(name)) &&
	    (SECURITY->query_wiz_rank(name) < rank))
	{
	    break;
	}

	/* Intentionally no break. Could be an allowed user. */

    default:
        /* May not handle wizards here. */
	if (SECURITY->query_wiz_rank(name))
	{
	    write("You may not handle the file on " + capitalize(name) +
		" as that player is a wizard.\n");
	    return 1;
	}
    }

    /* Make sure we have the proper euid. */
    SECURITY->set_helper_soul_euid();

    file = PINFO_FILE(name);

    switch(cmd)
    {
    case "d":
	if (rank < WIZ_ARCH)
	{
	    notify_fail("Only an arch++ can delete pinfo.\n");
	    return 0;
	}

	if (file_size(file) == -1)
	{
	    write("There is no pinfo on " + capitalize(name) + ".\n");
	    return 1;
	}

	rm(file);
	write("Removed pinfo on " + capitalize(name) + ".\n");
	return 1;

    case "e":
	if (rank < WIZ_ARCH)
	{
	    notify_fail("Only an arch++ can edit pinfo.\n");
	    return 0;
	}

	if (file_size(file) == -1)
	{
	    write("There is no pinfo on " + capitalize(name) + ".\n");
	    return 1;
	}

	pinfo_edit[wname] = file;
	text = read_file(file);
	clone_object(EDITOR_OBJECT)->edit(PINFO_EDIT_DONE, text,
	    sizeof(explode(text, "\n")));
	return 1;

    case "m":
    case "r":
	if (file_size(file) == -1)
	{
	    write("There is no pinfo on " + capitalize(name) + ".\n");
	    return 1;
	}

	text = read_file(file);

	/* Automatically invoke more, or wizards request. */
	if ((cmd == "m") ||
	    (sizeof(explode(text, "\n")) > 100))
	{
	    this_player()->more(text);
	}
	else
	{
	    write(text);
	}

	return 1;

    case "t":
	if (file_size(file) == -1)
	{
	    write("There is no pinfo on " + capitalize(name) + ".\n");
	    return 1;
	}

	tail(file);
	return 1;

    case "w":
	if (file_size(file) == -1)
	{
	    write("Writing pinfo file on " + capitalize(name) + ".\n");
	}
	else
	{
	    write("Appending pinfo file on " + capitalize(name) + ".\n");
	}

	if (strlen(text))
	{
	    if (strlen(text) > 75)
	    {
		text = break_string(text, 75);
	    }

	    write_file(file, ctime(time()) + " " + capitalize(wname) + " (" +
		capitalize(WIZ_RANK_NAME(SECURITY->query_wiz_rank(wname))) +
		"):\n" + text + "\n\n");
	}
	else
	{
	    pinfo_edit[wname] = file;
	    clone_object(EDITOR_OBJECT)->edit(PINFO_WRITE_DONE);
	}

	return 1;

    default:
	notify_fail("Syntax: pinfo [r / t / w / d / e] <name> [<text>]\n");
	return 0;
    }

    write("Impossible end of pinfo(). Please report.\n");
    return 1;
}
