/*
 * /secure/player_tool.c
 *
 * This object can be used by the helpers of the playerarch to access
 * playerfiles. It is autoloadable.
 *
 * /Mercade, April 5 1995
 */

#pragma no_inherit
#pragma no_shadow
#pragma save_binary
#pragma strict_types

inherit "/std/object";

#include <filepath.h>
#include <macros.h>
#include <std.h>
#include <stdproperties.h>

#define PRIVATE		("/private")
#define PRIVATE_DIR(p)	((p) + PRIVATE)
#define PLAYERS		("/players")
#define PLAYERS_DIR(p)	(PRIVATE_DIR(p) + PLAYERS)
#define BACKUP		("/player_tool")
#define BACKUP_DIR	(PLAYERS + BACKUP)

#define CHECK_SO_ACCESS if (!valid_access()) return 0

/*
 * Global variable. This variable is saved.
 *
 * arch_helpers - mapping that contains the names of the non-arches that
 *                are allowed to use this tool. The value of the mapping is
 *                the person that allowed the wizard.
 */
private mapping arch_helpers = ([ ]);

/*
 * Global variable. This variable is not saved.
 *
 * alphabet - array with the letters of the alphabet.
 */
private static string *alphabet = explode(ALPHABET, "");

/*
 * Prototypes.
 */
static nomask int playeraccess(string str);
static nomask int playerput(string str);
static nomask int playerget(string str);

/*
 * Function name: create_object
 * Description  : Constructor. Called when this object is created.
 */
nomask void
create_object()
{
    ustaw_nazwe( ({ "mis", "misia", "misiowi", "misia", "misiem", "misiu" }),
        ({ "misie", "misiow", "misiom", "misie", "misiami", "misiach" }),
        PL_MESKI_NOS_ZYW);

    set_long(break_string("It is a player tool that can be used to copy " +
	"playerfiles from the secured directory of the playerfiles to the " +
	"directory players in your private directory. Later, the modified " +
	"files can be put back. There is a help-page on all supported " +
	"commands. Apart from the archwizards and keepers, only people who " +
	"have been granted special access to this tool can use this tool. " +
	"The command 'playeraccess list' can be used by everyone to see who " +
	"has that access.", 75) + "\nThe commands supported are:\n" +
"    playeraccess - manage the list of people with access to this tool\n" +
"    playerget    - copy a playerfile to your private directory\n" +
"    playerput    - copy a playerfile back from your private directory\n");

    remove_prop(OBJ_I_VALUE);
    remove_prop(OBJ_I_VOLUME);
    remove_prop(OBJ_I_WEIGHT);

    add_prop(OBJ_I_NO_STEAL,    1);
    add_prop(OBJ_I_NO_TELEPORT, 1);
    add_prop(OBJ_S_WIZINFO,
	"Examine the player tool for information. /Mercade.\n");

    setuid();
    seteuid(getuid());
    if (!restore_object(MASTER))
    {
	tell_object(this_interactive(),
	    "Could not restore the player tool. This means that no people " +
	    "other than archwizards/ keepers have been granted access.\n");

	arch_helpers = ([ ]);
    }
}

/*
 * Function name: init
 * Description  : This function is called when someone 'comes close' to
 *                this object to add the necessary commands to him/her.
 */
nomask public void
init()
{
    ::init();

    add_action(playeraccess, "playeraccess");
    add_action(playerget,    "playerget");
    add_action(playerput,    "playerput");
}

/*
 * Function name: valid_access
 * Description  : This function checks whether the currently interactive
 *                player is allowed to execute commands on this tool.
 * Returns      : int 1/0 - true if the player may execute commands.
 */
static nomask int
valid_access()
{
    string name;

    /* Sanity check. Only truely interactive people can use this tool. */
    if (this_player() != this_interactive())
    {
	notify_fail("Invalid interactive player. No access.\n");
	return 0;
    }

    /* Wizard must carry the tool in order to use it. */
    if (environment() != this_interactive())
    {
	notify_fail("You are not carrying the player tool. No access.\n");
	return 0;
    }

    name = this_interactive()->query_real_name();

    /* Arches and keepers have access. */
    if (SECURITY->query_wiz_rank(name) >= WIZ_ARCH)
    {
	return 1;
    }

    /* Arch-helpers, added to this tool, may use it too. */
    if (arch_helpers[name])
    {
	return 1;
    }

    notify_fail("You are neither a member of the administration, nor a " +
	"registered helper to the playerarch, so you may not use this tool.\n");
    return 0;
}

/*
 * Function name: valid_filename
 * Description  : This function tests whether the filename of the playerfile
 *                contains only valid characters, i.e. letters of the
 *                alphabet.
 * Arguments    : string name - the name to test.
 * Returns      : int 1/0 - success/failure.
 */
static nomask int
valid_filename(string name)
{
    if (!strlen(name))
    {
	return 0;
    }

    return (!sizeof(explode(name, "") - alphabet));
}

/*
 * Function name: playeraccess
 * Description  : This function can be used to manage the list of people who
 *                have access to this tool.
 * Arguments    : string str - the command line argument.
 * Returns      : int 1/0 - success/failure.
 */
nomask static int
playeraccess(string str)
{
    string *args;
    string name = this_interactive()->query_real_name();
    int    size;
    int    index = -1;

    /* Default to 'list'. */
    if (!strlen(str))
    {
	str = "list";
    }

    /* Everyone may issue the list-command, but only arches may enter other
     * commands.
     */
    if ((str != "l") &&
	(str != "list"))
    {
	CHECK_SO_ACCESS;

	if (SECURITY->query_wiz_rank(name) < WIZ_ARCH)
	{
	    notify_fail("Only arches and keepers may use subcommands " +
		"other than 'list'.\n");
	    return 0;
	}
    }

    args = explode(lower_case(str), " ");

    switch(args[0])
    {
    case "a":
    case "add":
	/* Syntax error. Incorrect number of arguments. */
	if (sizeof(args) != 2)
	{
	    notify_fail("Syntax: playeraccess a[dd] <name>\n");
	    return 0;
	}

	/* Only 'full' wizards can be granted access. */
	if (SECURITY->query_wiz_rank(args[1]) < WIZ_NORMAL)
	{
	    notify_fail("Not a full wizard: " + capitalize(args[1]) + ".\n");
	    return 0;
	}

	/* Check is he/she isn't already listed. */
	if (arch_helpers[args[1]])
	{
	    notify_fail("Already registered in this tool: " +
		capitalize(args[1]) + ".\n");
	    return 0;
	}

	/* All checks successful, add the wizard and save this object. */
	arch_helpers[args[1]] = name;

	setuid();
	seteuid(getuid());
	save_object(MASTER);

	write("Granted " + capitalize(args[1]) +
	    " access to the player tool.\n");
	return 1;

    case "l":
    case "list":
	if (sizeof(args) != 1)
	{
	    notify_fail("Syntax: playeraccess l[ist]\n");
	    return 0;
	}

	size = m_sizeof(arch_helpers);
	if (!size)
	{
	    write("No wizards other than archwizards/ keepers have access " +
		"to the player tool.\n");
	    return 1;
	}

	write("The following wizard" + ((size == 1) ? " has" : "s have") +
	    " access to the player tool other than archwizards/ keepers:\n" +
	    "    Wizard        Granted by\n    -----------   -----------\n");

	args = m_indices(arch_helpers);
	while(++index < size)
	{
	    write(sprintf("    %-11s   %-11s\n", capitalize(args[index]),
		capitalize(arch_helpers[args[index]])));
	}
	return 1;

    case "r":
    case "remove":
	/* Syntax error. Incorrect number of arguments. */
	if (sizeof(args) != 2)
	{
	    notify_fail("Syntax: playeraccess r[emove] <name>\n");
	    return 0;
	}

	/* Check whether he/she is listed. */
	if (!arch_helpers[args[1]])
	{
	    notify_fail("Not registered in this tool: " +
		capitalize(args[1]) + ".\n");
	    return 0;
	}

	/* All checks successful, remove the wizard and save this object. */
	arch_helpers = m_delete(arch_helpers, args[1]);

	setuid();
	seteuid(getuid());
	save_object(MASTER);

	write("Revoked the access to the player tool from " +
	    capitalize(args[1]) + ".\n");
	return 1;

    default:
	notify_fail("No such subcommand. See 'help playeraccess'.\n");
	return 0;
    }

    notify_fail("Error in playeraccess(). Should never happen.\n");
    return 0;
}

/*
 * Function name: playerget
 * Description  : This function can be used to transfer/copy a playerfile
 *                from the secure directory for the playerfiles to the
 *                private directory of the wizard.
 * Arguments    : string str - the name of the player whose file to copy.
 * Returns      : int 1/0 - success/failure.
 */
nomask static int
playerget(string str)
{
    string name;
    string path;
    string file;
    string buffer;

    CHECK_SO_ACCESS;

    if (!strlen(str))
    {
	notify_fail("Syntax: playerget <name>\n");
	return 0;
    }

    str = lower_case(str);

    if (!valid_filename(str))
    {
	notify_fail("Not a valid playername '" + capitalize(str) +
	    "' since not all characters are letters.\n");
	return 0;
    }

    file = PLAYER_FILE(str) + ".o";
    if (file_size(file) <= 0)
    {
	notify_fail("No player named " + capitalize(str) + " found.\n");
	return 0;
    }

    name = this_interactive()->query_real_name();
    /* The arch-helpers should not be interested in wizards files. Arches
     * or keepers may do so though.
     */
    if (SECURITY->query_wiz_rank(str) &&
	(SECURITY->query_wiz_rank(name) < WIZ_ARCH))
    {
	notify_fail("No reason to playerget a wizards file.\n");
	return 0;
    }

    path = SECURITY->query_wiz_path(name);
    /* See if the target directory exists. */
    if (file_size(PLAYERS_DIR(path)) != -2)
    {
	/* See if the private directory exists. */
	if (file_size(PRIVATE_DIR(path)) != -2)
	{
	    /* Try to create the private directory. */
	    if (!mkdir(PRIVATE_DIR(path)))
	    {
		notify_fail("Failed to create directory " +
		    RPATH(PRIVATE_DIR(path)) + "\n");
		return 0;
	    }

	    write("Created directory " + RPATH(PRIVATE_DIR(path)) + "\n");
	}

	/* Try to create the players directory in the private directory. */
	if (!mkdir(PLAYERS_DIR(path)))
	{
	    notify_fail("Failed to create directory " +
		RPATH(PLAYERS_DIR(path)) + "\n");
	    return 0;
	}

	write("Created directory " + RPATH(PLAYERS_DIR(path)) + "\n");
    }

    /* Since there is no copy-efun, we have to read the playerfile first. */
    buffer = read_file(file);
    if ((!stringp(buffer)) ||
	(!strlen(buffer)))
    {
	notify_fail("Failed to read " + RPATH(file) + "\n");
	return 0;
    }

    /* Before we try to save the buffer, see whether the target is
     * available.
     */
    file = PLAYERS_DIR(path) + "/" + str + ".o";
    switch(file_size(file))
    {
    case -2:
	notify_fail("Target " + RPATH(file) + " is a directory.\n");
	return 0;

    case -1:
    case  0:
	break;

    default:
	if (!rm(file))
	{
	    notify_fail("Failed to remove previous copy of " + RPATH(file) +
		"\n");
	    return 0;
	}
    }

    if (!write_file(file, buffer))
    {
	notify_fail("Failed to write " + RPATH(file) + "\n");
	return 0;
    }

    write("Playerfile from " + capitalize(str) + " copied to " +
	RPATH(file) + "\n");
    return 1;
}

/*
 * Function name: playerput
 * Description  : This function can be used to transfer/copy a playerfile
 *                from the private directory of a wizard to the secure
 *                directory for the playerfiles.
 * Arguments    : string str - the name of the player whose file to copy.
 * Returns      : int 1/0 - success/failure.
 */
nomask static int
playerput(string str)
{
    string name;
    string file;
    string buffer;
    string backup;
    int    number = 1;

    CHECK_SO_ACCESS;

    if (!strlen(str))
    {
	notify_fail("Syntax: playerput <name>\n");
	return 0;
    }

    name = this_interactive()->query_real_name();
    str = lower_case(str);

    if (!valid_filename(str))
    {
	notify_fail("Not a valid player name '" + capitalize(str) +
	    "' since not all characters are letters.\n");
	return 0;
    }

    /* The arch-helpers should not be interested in wizards files. Arches
     * or keepers may do so though.
     */
    if (SECURITY->query_wiz_rank(str) &&
	(SECURITY->query_wiz_rank(name) < WIZ_ARCH))
    {
	notify_fail("No reason to playerput a wizards file.\n");
	return 0;
    }

    if (objectp(find_player(str)))
    {
	notify_fail("The player " + capitalize(str) + " is logged in. It is " +
	    "useless to playerput now.\n");
	return 0;
    }

    setuid();
    seteuid(getuid());

    file = PLAYERS_DIR(SECURITY->query_wiz_path(name)) + "/" + str + ".o";
    if (file_size(file) <= 0)
    {
	notify_fail("File " + RPATH(file) + " not found.\n");
	return 0;
    }

    /* Since there is no copy-efun, we have to read the playerfile first. */
    buffer = read_file(file);
    if ((!stringp(buffer)) ||
	(!strlen(buffer)))
    {
	notify_fail("Failed to read " + RPATH(file) + "\n");
	return 0;
    }

    file = PLAYER_FILE(str) + ".o";

    /* If there is a file, make a proper backup. */
    if (file_size(file) > 0)
    {
	backup = BACKUP_DIR + "/" + str + ".o.";

	if (file_size(BACKUP_DIR) != -2)
	{
	    if (!mkdir(BACKUP_DIR))
	    {
		notify_fail("Failed to create backup directory to backup " +
		    "the old version of the playerfile.\n");
		return 0;
	    }

	    write("Backup directory created: " + BACKUP_DIR + "\n");
	}

	while(file_size(backup + number) > 0)
	{
	    number++;
	}

	if (!rename(file, (backup + number)))
	{
	    notify_fail("Failed to make a backup of the original " +
		"playerfile in the secured playerfile directory.\n");
	    return 0;
	}

	write("Previous playerfile backed up with name " + str + ".o." +
	    number + " in the backup directory of the player tool.\n");
    }

    if (!write_file(file, buffer))
    {
	write("Failed to write " + file + "\n");
	return 1;
    }

    write("Playerfile from " + capitalize(str) +
	" copied back to the secured playerfiles directory.\n");
    return 1;
}

/*
 * Function name: query_arch_helpers
 * Description  : This function returns a mapping with the names of the
 *                people who have been allowed to use the player tool. In
 *                general this will be the arch-helpers.
 * Returns      : mapping - the mapping with the helpers of the player arch.
 *                          the indices are the helpers and the values the
 *                          people who allowed those helpers.
 */
public nomask mapping
query_arch_helpers()
{
    return ([ ]) + arch_helpers;
}

/*
 * Functio name: query_auto_load
 * Description : This function is called to test whether this object is
 *               autoloadable.
 * Returns     : string - the path to this object.
 */
public nomask string
query_auto_load()
{
    return MASTER + ":";
}
