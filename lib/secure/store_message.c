/*
 * /secure/store_message.c
 *
 * This is a part of the Genesis mail system and subject to the copyright
 * notice in the header of the mail reader. It is placed in the /secure
 * directory to give it access to all personal directories of the wizards
 * when they want to save mail.
 *
 * The function store_message() is called from the mail reader and may
 * only be called from the mail reader. It does not accept any other
 * calls.
 *
 * Messages will be written to the file ~wizname/private/mail/<filename>.
 *
 * /Mercade, October 8th 1994
 *
 * Revision history:
 */

#pragma no_clone
#pragma no_inherit
#pragma no_shadow
#pragma strict_types

#include <macros.h>
#include <std.h>

#define MAIL_READER "/d/Standard/postmaster/mail_reader"
#define ALLOWED     "abcdefghijklmnopqrstuvwxyz0123456789_ "

static private string *allowed = explode(ALLOWED, "");

/*
 * Function name: valid_characters
 * Description  : This function checks whether all characters in the
 *                intended filename are valid.
 * Arguments    : string name - the intended filename.
 * Returns      : int 1/0 - true if all characters are valid.
 */
nomask static int
valid_characters(string name)
{
    string *characters = explode(name, "");
    int     index;

    for (index = 0; index < sizeof(characters); index++)
    {
	if (member_array(characters[index], allowed) == -1)
	{
	    return 0;
	}
    }

    return 1;
}

/*
 * Function name: store_message
 * Description  : This function may be called from the mail reader when
 *                a wizard wants to write a message to file.
 * Arguments    : string message  - the message to write.
 *                string filename - the filename to write to.
 * Returns      : string - the resulting message.
 */
nomask public string
store_message(string message, string filename)
{
    string path;

    if (!CALL_BY(MAIL_READER))
    {
	return "Illegal call. Not called by the mail reader!";
    }

    if ((!objectp(this_player())) ||
	(!objectp(this_interactive())) ||
	(this_player() != this_interactive()))
    {
	return "Inconsistant interactive player.";
    }

    if (!strlen(message))
    {
	return "No message file passed.";
    }

    if (!strlen(filename))
    {
	return "No filename passed.";
    }

    if (!valid_characters(filename))
    {
	return "The file name " + filename + " contains characters that are " +
	    "not allowed. Only letters, numbers and the understore are " +
	    "allowed for security reasons.";
    }

    setuid();
    seteuid(getuid());

    path = SECURITY->query_wiz_path(this_interactive()->query_real_name()) +
	"/private";

    switch(file_size(path))
    {
    case -2:
	break;

    case -1:
	mkdir(path);
	if (file_size(path) != -2)
	{
	    return "Could not create private directory " + path +
		" and the message could therefore not be written.";
	}
	else
	{
	    write("Private directory " + path + " created.\n");
	}
	break;

    default:
	return "Your private directory is a file. It must be directory.";
    }

    path += "/mail";
    switch(file_size(path))
    {
    case -2:
	break;

    case -1:
	mkdir(path);
	if (file_size(path) != -2)
	{
	    return "Your mail directory " + path + " could not be created. " +
		"The message could therefore not be written.";
	}
	else
	{
	    write("Mail directory " + path + " created.\n");
	}
	break;

    default:
	return "Your mail directory is a file. It must be a directory.";
    }

    path += "/" + implode(explode(filename, " "), "_");

    if (file_size(path) != -1)
    {
	return "The message file " + path + " already exists. Nothing saved.";
    }

    if (!write_file(path, message))
    {
	return "File name " + path + " could not be written.";
    }

    return "Message stored in " + path + ".";
}
