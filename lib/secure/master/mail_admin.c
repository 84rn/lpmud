/*
 * /secure/master/mail_admin.c
 *
 * This part of SECURITY is the mail administrator. Because the mail system
 * uses a data-structure that cannot be manipulated directly (hashing and
 * individual message files), we need a central module to control the system.
 * The arch-level "mailadmin" command can be used to access the functionality
 * in this module.
 *
 * The following subcommands are supported:
 * - distribute all message-files over another set of hash-directories;
 * - export a the mail-folder of a player to file;
 * - print statistics about the mail system;
 * - purge all mailboxes and message-files that have reference to players
 *   that do not exist any longer, then purge all message-files that are
 *   not pointed at by any mail folder.
 */

#include "/sys/composite.h"
#include "/sys/mail.h"

#define DIR_NAME_MESSAGE(t) (MSG_DIR + "d" + (t))
#define MAX_REHASH_CYCLE  (1000)
#define MAX_MAILBOX_PURGE ( 500)
#define MAX_MESSAGE_PURGE (  50)
#define LINE_LENGTH       (  77)
#define SPACES ("                                 ")

/*
 * Global variables.
 *
 * mail_wizard  - the wizard handling the mail administrator when an alarm
 *                is running.
 * mail_players - the names of all players. The values and indices can be
 *                capitalized or not dependant on the status of the module.
 * mail_system  - mapping with the names of the players with a mailbox and
 *                the numbers of all message files.
 * mail_alarm   - the alarm-id if an alarm is running.
 */
static private string  mail_wizard  = 0;
static private mapping mail_players = 0;
static private mapping mail_system  = 0;
static private int     mail_alarm   = 0;

/*
 * Function name: restore_mail
 * Description  : Restore the mail-file of a player from disk.
 * Arguments    : string name - the name of the player.
 * Returns      : mapping     - the mail of the player.
 */
static mapping
restore_mail(string name)
{
    mapping mail;

    mail = restore_map(FILE_NAME_MAIL(name));

    if ((!mappingp(mail)) ||
	(m_sizeof(mail) != M_SIZEOF_MAIL))
    {
	return 0;
    }

    return mail;
}

/*
 * Function name: save_mail
 * Description  : Save the mail-file of a player to disk.
 * Arguments    : mapping mail - the mail of the player.
 *                string  name - the name of the player.
 */
static void
save_mail(mapping mail, string name)
{
    save_map(mail, FILE_NAME_MAIL(name));
}

/*
 * Function name: restore_message
 * Description  : Restore an individual message from disk.
 * Arguments    : int number - the time of the message.
 * Returns      : mapping  - the message restored.
 */
static varargs mapping
restore_message(int number)
{
    mapping message;

    message = restore_map(FILE_NAME_MESSAGE(number, HASH_SIZE));

    if ((!mappingp(message)) ||
	(m_sizeof(message) != M_SIZEOF_MSG))
    {
	return 0;
    }

    return message;
}

/*
 * Function name: save_message
 * Description  : Save the individual message to disk.
 * Arguments    : mapping message - the message.
 *                int     number    - the current time to save.
 */
static void
save_message(mapping message, int number)
{
    save_map(message, FILE_NAME_MESSAGE(number, HASH_SIZE));
}

/*
 * Function name: mail_tell_wizard
 * Description  : Give a message to the wizard handling the mailadmin
 *                command.
 * Arguments    : string str - the message to give to the wizard.
 */
void
mail_tell_wizard(string str)
{
    object wizard = find_player(mail_wizard);

    if (objectp(wizard))
    {
	tell_object(wizard, str);
    }
}

/*
 * Function name: load_players
 * Description  : A small service function that loads the names of all
 *                existing players into a mapping. The indices are the
 *                letters of the alphabet. The values are arrays with the
 *                names, in lower case.
 */
static void
load_players()
{
    int index = -1;
    string *files;
    string letter;

    if (m_sizeof(mail_players))
    {
	return;
    }
    
    mail_players = ([]);
    while(++index < 26)
    {
	letter = ALPHABET[index..index];

	files = get_dir(PLAYER_FILE(letter + "*.o"));
	files = map(files, &extract(, 0, -3));
	
	mail_players[letter] = files;
    }
}

/*
 * Function name: print_tabulated
 * Descritpion  : This function will print a string such that the first line
 *                is started at the first column of the screen, but if a
 *                line-break if necessary, all following lines will receive
 *                an intent. Lines will be broken after LINE_LENGTH chars.
 * Arguments    : string text - the text to print. This line should not be
 *                              terminated by a newline.
 *                int length  - the number of spaces to indent.
 * Returns      : string - the return text. This text will be terminated by
 *                         a newline.
 */
string
print_tabulated(string text, int length)
{
    string *tmp;

    if (strlen(text) <= LINE_LENGTH)
    {
	return (text + "\n");
    }

    /* Isn't this a cute return statement? ;-) */
    tmp = explode(break_string(text, LINE_LENGTH), "\n");
    return (implode( ({ tmp[0] }) +
	(explode(break_string(implode(tmp[1..], " "),
	(LINE_LENGTH - (length + 2))), "\n")),
	("\n" + SPACES[1..length])) + "\n");
}

/*
 * Function name: wrap_text
 * Description  : This will take a text and make sure that none of the lines
 *                in the text exceed LINE_LENGTH characters. All lines longer
 *                than that will be broken. Lines will not be merged.
 * Arguments    : string - the text to be wrapped.
 * Returns      : string - the wrapped text.
 */
string
wrap_text(string text)
{
    string *lines;
    int    size;

    if (!strlen(text))
    {
	return "";
    }

    lines = explode(text, "\n");
    size = sizeof(lines);
    while(--size >= 0)
    {
	if (strlen(lines[size]) > LINE_LENGTH)
	{
	    lines[size] = break_string(lines[size], LINE_LENGTH);
	}
    }

    return implode(lines, "\n");
}

/*
 * Function name: export_mail
 * Description  : With this function the mail-folder of a player can be
 *                exported to a file.
 * Arguments    : string name - the lower case name of the player whose
 *                              mailbox is to be exported.
 *                string path - the name of the output file.
 * Returns      : int 1/0 - success/failure.
 */
static int
export_mail(string name, string path)
{
    mapping mail;
    mixed   messages;
    int     index;
    int     size;
    string  text;

    /* Get the output path and test its valitity by writing the header. */
    path = FTPATH(this_player()->query_path(), path);
    if (file_size(path) != -1)
    {
	notify_fail("File " + path + " already exists.\n");
	return 0;
    }

    /* This may fail if there is no such directory, for instance. */
    if (!write_file(path, "Mailbox of: " + capitalize(name) +
	"\nPrinted at: " + ctime(time()) + "\n\n"))
    {
	notify_fail("Failed to write header of " + path + "\n");
	return 0;
    }

    /* Read the mail file. */
    mail = restore_mail(name);
    if (!mappingp(mail))
    {
	notify_fail("No correct mail folder for player " + name + ".\n");
	return 0;
    }

    /* Loop over all messages. */
    messages = mail[MAIL_MAIL];
    index = -1;
    size = sizeof(messages);
    while(++index < size)
    {
	mail = restore_message(messages[index][MAIL_DATE]);
	
	text = "Message: " + (index + 1) + "\nFrom   : " +
	      messages[index][MAIL_FROM] + "\n" +
	      (messages[index][MAIL_REPLY] ? "Reply  : " : "Subject: ") +
	      messages[index][MAIL_SUBJ] + "\n";
	
	if (mail[MSG_TO] != capitalize(name))
	{
	    text += print_tabulated("To     : " +
		COMPOSITE_WORDS(explode(mail[MSG_TO], ",")), 9);
	}
	
	if (mail[MSG_CC] != "")
	{
	    text += print_tabulated("CC     : " +
		COMPOSITE_WORDS(explode(mail["cc"], ",")), 9);
	}

	/* Write the message to file and print a sequence number to the
	 * wizard. Notice that the index of the loop is also increased in
	 * this write-statement.
	 */	
	write_file(path, text + "Date   : " +
	    MAKE_DATE(messages[index][MAIL_DATE]) +
            DATE_YEAR(messages[index][MAIL_DATE]) +
	    "\n\n" + wrap_text(mail[MSG_BODY]) + "\n\n");
    }

    write("Mail folder written for " + capitalize(name) + ".\nFilename: " +
	path + "\n");
    return 1;
}

/*
 * Function name: purge_crosscheck
 * Description  : The crosscheck itself is coded in a different object.
 *                We call that object.
 */
static void
purge_crosscheck()
{
    mail_tell_wizard("MAIL ADMINISTRATOR ->> Garbage collection started.\n");

    /* Set this to signal that the garbage collector is active. */
    mail_alarm = -1;

    GARBAGE_COLLECTOR->start_garbage_collector();
}

/*
 * Function name: purge_crosscheck_done
 * Description  : When the crosscheck is in progress, 
 */
public void
purge_crosscheck_done()
{
    if (previous_object() != find_object(GARBAGE_COLLECTOR))
    {
	return;
    }

    mail_tell_wizard("MAIL ADMINISTRATOR ->> Garbage collection finished.\n");
    
    mail_wizard = 0;
    mail_system = 0;
    mail_players = 0;
    mail_alarm = 0;
}

/*
 * Function name: purge_many_messagefiles
 * Description  : This will loop over a directory or more to load all
 *                messagefiles in it to see which recipients are still valid.
 * Arguments    : int directory - the directory to process next.
 */
static void
purge_many_messagefiles(int directory)
{
    mapping message;
    string *names;
    string *valid_names;
    int     index1;
    int     size1;
    int     index2;
    int     size2;
    int     errors;
    int     number;
    int     purged = 0;

    set_auth(this_object(), "root:root");

    /* Loop over directories until the maximum number of message files
     * is processed. We do not want eval-cost problems.
     */
    while((purged < MAX_MESSAGE_PURGE) &&
	m_sizeof(mail_system))
    {
	index1 = -1;
	size1 = sizeof(mail_system[directory]);
	while(++index1 < size1)
	{
	    /* Only process files following the filename convention. */
	    if (sscanf(mail_system[directory][index1], "m%d.o", number) != 1)
	    {
		continue;
	    }

	    message = restore_message(number);
	    if (!strlen(message[MSG_ADDRESS]))
	    {
		rm(FILE_NAME_MESSAGE(number, HASH_SIZE) + ".o");
		continue;
	    }

	    /* Test all names in the recipients for existance. */
	    valid_names = ({ });
	    names = explode(message[MSG_ADDRESS], ",");
	    size2 = sizeof(names);
	    index2 = -1;
	    while(++index2 < size2)
	    {
		if (member_array(names[index2],
		    mail_players[names[index2][0]]) > -1)
		{
		    valid_names += ({ names[index2] });
		}
	    }

	    /* Usuwa powtarzajace sie nazwy */
	    names = valid_names + ({}); valid_names = ({});
	    while(sizeof(names))
	    {
	        valid_names += ({ names[0] });
	        names -= ({ names[0] });
	    }

	    /* No names removed. Continue. */
	    if (sizeof(valid_names) == size2)
	    {
		continue;
	    }

	    if (sizeof(valid_names))
	    {
		message[MSG_ADDRESS] = implode(valid_names, ",");
		save_message(message, number);
		continue;
	    }

	    rm(FILE_NAME_MESSAGE(number, HASH_SIZE) + ".o");
	}

	purged += sizeof(mail_system[directory]);

	mail_system = m_delete(mail_system, directory);
	directory++;
    }

    /* If there is nothing left, we are done. */
    if (!m_sizeof(mail_system))
    {
	mail_tell_wizard("MAIL ADMINISTRATOR ->> Done purging messages.\n");
	mail_alarm = set_alarm(5.0, 0.0, purge_crosscheck);
	return;
    }

    /* More to do... Tell the wizard and set the alarm. */
    mail_tell_wizard("MAIL ADMINISTRATOR ->> Purged till directory d" +
	(directory - 1) + ".\n");
    mail_alarm = set_alarm(5.0, 0.0, &purge_many_messagefiles(directory) );
}

/*
 * Function name: purge_messagefiles
 * Description  : All message files will be checked for recipients that are
 *                no longer existant. Those will be removed from the message
 *                files.
 */
static void
purge_messagefiles()
{
    int index;

    /* Prepare the players mapping for message-purging. Capitalize the
     * names of the players because they are capitalized in the message
     * recipients.
     */
    index = -1;
    while(++index < 26)
    {
	mail_players[capitalize(ALPHABET[index..index])[0]] =
	    map(mail_players[ALPHABET[index..index]], capitalize);
	mail_players = m_delete(mail_players, ALPHABET[index..index]);
    }

    /* We also read all message-directories, so we don't have to do
     * that later.
     */
    index = -1;
    mail_system = ([ ]);
    while (++index < HASH_SIZE)
    {
	mail_system[index] = get_dir(DIR_NAME_MESSAGE(index) + "/m*.o");
    }

    mail_alarm = set_alarm(1.0, 0.0, &purge_many_messagefiles(0));
}

/*
 * Function name: purge_mailboxes
 * Description  : Loops over the alphabet and tests all mailboxes in a
 *                directory to see whether the player attached to it is
 *                an existing player. I don't think alarms are necessary
 *                here.
 */
static void
purge_mailboxes()
{
    string *files;
    int    found = 0;
    int    index;
    int    size;
    int    letter = -1;
    int    purged = 0;

    set_auth(this_object(), "root:root");

    /* Loop over all letters of the alphabet. */
    while(++letter < 26)
    {
	/* Get the names of all mailboxes for this letter. */
	files = get_dir(FILE_NAME_MAIL(ALPHABET[letter..letter] + "*.o"));
	files = map(files, &extract(, 0, -3));
	found += sizeof(files);

	/* Subtract all existing players. */
	files -= mail_players[ALPHABET[letter..letter]];

	if (size = sizeof(files))
	{
	    purged += size;
	    index = -1;
	    while(++index < size)
	    {
		rm(FILE_NAME_MAIL(files[index] + ".o"));
	    }
	}
    }

    mail_tell_wizard("MAIL ADMINISTRATOR ->> Checked " + found +
	" files and purged " + purged + " mailboxes.\n" +
	"Next stage: purging non-existant names from message files.\n");

    mail_alarm = set_alarm(5.0, 0.0, purge_messagefiles);
}

/*
 * Function name: purge_mail
 * Description  : This function will purge the mail system. It will do
 *                the following:
 *                - remove all mailboxes from players that do no longer exist;
 *                - check all message files and remove the names of all
 *                  recipients who no longer exist;
 *                - cross check all message files with all mailboxes and
 *                  remove those message files that are no longer connected
 *                  to any mailbox.
 * Returns      : int 1/0 - success/failure.
 */
static int
purge_mail()
{
    load_players();

    write("Purge started.\n");
    write("Starting with removing mailboxes of non-existant players\n");

    mail_wizard = this_player()->query_real_name();

    mail_alarm = set_alarm(1.0, 0.0, purge_mailboxes);

    return 1;
}

/*
 * Function name: rehash_many
 * Description  : This function is called with an alarm to re-hash
 *                parts of the total mail structure without getting into
 *                problems with eval-cost.
 * Arguments    : int new_hash  - the new hash size.
 *                int directory - the next directory to test.
 */
static void
rehash_many(int new_hash, int directory)
{
    int index;
    int size;
    int errors;
    int number;
    int hashed = 0;

    set_auth(this_object(), "root:root");

    /* Loop over directories until the maximum number of message files
     * is processed. We do not want eval-cost problems.
     */
    while((hashed < MAX_REHASH_CYCLE) &&
	m_sizeof(mail_system))
    {
	index = -1;
	size = sizeof(mail_system[directory]);
	while(++index < size)
	{
	    /* Only process files following the filename convention. */
	    if (sscanf(mail_system[directory][index], "m%d.o", number) != 1)
	    {
		continue;
	    }

	    rename(FILE_NAME_MESSAGE(number, HASH_SIZE),
		FILE_NAME_MESSAGE(number, new_hash));
	}

	hashed += sizeof(mail_system[directory]);

	mail_system = m_delete(mail_system, directory);
	directory++;
    }

    /* If there is nothing left, we are done. */
    if (!m_sizeof(mail_system))
    {
	mail_tell_wizard("MAIL ADMINISTRATOR ->> DONE REHASHING.\n");
	mail_wizard = 0;
	mail_system = 0;
	return;
    }

    /* More to do... Tell the wizard and set the alarm. */
    mail_tell_wizard("MAIL ADMINISTRATOR ->> Rehashed till directory d" +
	(directory - 1) + ".\n");
    mail_alarm = set_alarm(5.0, 0.0, &rehash_many(new_hash, directory) );
}

/*
 * Function name: rehash_mail
 * Description  : When too many message files are placed in a single
 *                directory, you may want to re-hash the messages and
 *                spread them over more directories.
 * Arguments    : string str - the command line argument with the new hash.
 * Returns      : int    1/0 - success/failure
 */
static int
rehash_mail(string str)
{
    int index;
    int new_hash = atoi(str);

    if (new_hash < 10)
    {
	notify_fail("New hash size " + new_hash + " must be at least 10.\n");
	return 0;
    }

    if (new_hash == HASH_SIZE)
    {
	notify_fail("New hash size " + new_hash +
	    " is equal to the current hash size. No action.\n");
	return 0;
    }

    if (new_hash > HASH_SIZE)
    {
	write("Adding new directories where necessary...\n");

        index = HASH_SIZE;
	while (++index < new_hash)
	{
	    switch(file_size(DIR_NAME_MESSAGE(index)))
	    {
	    case -2:
		/* Already a directory. */
		break;

	    case -1:
		mkdir(DIR_NAME_MESSAGE(index));
		break;

	    default:
		write("ERROR: " + DIR_NAME_MESSAGE(index) + " is a file.\n");
		write("Cannot continue re-hasing.\n");
		return 1;
	    }
	}
    }

    mail_wizard = this_player()->query_real_name();

    /* Since the newly hashed messages will be written to the same
     * directories, we have to get all directory contents first.
     */
    mail_system = ([ ]);
    index = -1;
    while (++index < HASH_SIZE)
    {
	mail_system +=
	    ([ index : get_dir(DIR_NAME_MESSAGE(index) + "/m*.o") ]);
    }

    write("Re-hashing started... Changing hash size from " + HASH_SIZE +
	" to " + new_hash + ". Note that after re-hasing is done, the " +
	"value of the definition HASH_SIZE must be adjusted in mail2.h and " +
	"all mail-related modules/objects be updated!\n");

    mail_alarm = set_alarm(1.0, 0.0, &rehash_many(new_hash, 0) );

    return 1;
}

/*
 * Function name: report_statistics
 * Description  : This function will do the actual reporting about the
 *                mail system.
 * Arguments    : int boxes    - the number of mailboxes found.
 *                int messages - the number of messages found in the boxes.
 *                int files    - the number of individual message files.
 */
static void
report_statistics(int boxes, int messages, int files)
{
    mail_tell_wizard("MAIL ADMINISTRATOR ->> Done gathering statistics.\n" +
	"A total of " + boxes + " mailboxes was found, containing a total " +
	"of " + messages + " mail messages. These messages are stored in " +
	files + " individual message files. On average, " +
	(files / HASH_SIZE) + " message files are stored in each of the " +
	HASH_SIZE + " directories.\n");

    mail_wizard = 0;
    mail_alarm = 0;
}

/*
 * Function name: count_messages
 * Description  : This function loops over all message directories and counts
 *                the number of messages found in them. Note that this
 *                function is not protected against eval-cost.
 * Arguments    : int boxes    - the number of mailboxes found.
 *                int messages - the number of messages found in the boxes.
 */
static void
count_messages(int boxes, int messages)
{
    int index = -1;
    int message_files = 0;

    while (++index < HASH_SIZE)
    {
	message_files += sizeof(get_dir(DIR_NAME_MESSAGE(index) + "/m*.o"));
    }

    mail_alarm = set_alarm(2.0, 0.0,
	&report_statistics(boxes, messages, message_files));
}

/*
 * Function name: count_mail_boxes
 * Description  : This function will loop over the mail directories and
 *                count all the mailboxes and all messages in it. Note that
 *                this function is not protected against eval-cost.
 * Arguments    : int letter   - the letter to process next.
 *                int boxes    - the number of mailboxes found.
 *                int messages - the number of messages found in the boxes.
 */
static void
count_mail_boxes(int letter, int boxes, int messages)
{
    int index = -1;
    int size;
    mapping mail;
    string *files;

    files = get_dir(FILE_NAME_MAIL(ALPHABET[letter..letter] + "*.o"));
    files = map(files, &extract(, 0, -3));
    size = sizeof(files);
    boxes += size;

    while(++index < size)
    {
	mail = restore_mail(files[index]);
	if (!mappingp(mail))
	{
	    boxes--;
	    continue;
	}

	messages += sizeof(mail[MAIL_MAIL]);
    }

    if (letter < 25)
    {
	mail_alarm = set_alarm(2.0, 0.0,
	    &count_mail_boxes(++letter, boxes, messages));
    }
    else
    {
	mail_alarm = set_alarm(2.0, 0.0,
	    &count_messages(boxes, messages));
    }
}

/*
 * Function mame: mail_statistics
 * Description  : This function will print various statistics about the
 *                mail system.
 * Returns      : int 1/0 - success/failure.
 */
static int
mail_statistics()
{
    mail_wizard = this_player()->query_real_name();
    mail_alarm = set_alarm(1.0, 0.0, &count_mail_boxes(0, 0, 0));

    write("Statictics gathering started. This will lag the game for " +
	"about a minute.\n");
    return 1;
}

/*
 * Function name: mailadmin
 * Description  : This contains the implementation of the "mailadmin" command
 *                in the arch-soul.
 * Arguments    : string str - the subcommand requested.
 * Returns      : int 1/0 - success/failure.
 */
int
mailadmin(string str)
{
    string *args;

    if (!CALL_BY(WIZ_CMD_ARCH))
    {
	notify_fail("This function may only be called from the arch-soul.\n");
	return 0;
    }

    if (strlen(mail_wizard) &&
	(str != "reset"))
    {
        write("The \"mailadmin\" command is busy for " + 
	    capitalize(mail_wizard) + ".\n");
	return 1;
    }

    if (!strlen(str))
    {
	notify_fail("Syntax: mailmaster <subcommand> [<arguments>]\n");
	return 0;
    }

    set_auth(this_object(), "root:root");

    args = explode(str, " ");
    switch(args[0])
    {
    case "export":
	if (sizeof(args) != 3)
	{
	    notify_fail("Syntax: mailadmin export <player name> <filename>\n");
	    return 0;
	}

	return export_mail(lower_case(args[1]), args[2]);
	/* notreached */

    case "purge":
	if (sizeof(args) != 1)
	{
	    notify_fail("Syntax: mailadmin purge\n");
	    return 0;
	}

	return purge_mail();
	/* notreached */

    case "rehash":
	if (sizeof(args) != 2)
	{
	    notify_fail("Syntax: mailadmin rehash <new hash size>\n");
	    return 0;
	}

	return rehash_mail(args[1]);
	/* notreached */

    case "reset":
	/* If the garbage collector is active, interrupt it. */
	if (mail_alarm == -1)
	{
	    GARBAGE_COLLECTOR->remove_object();
	}

	/* Set all variables to 0. */
	mail_wizard = 0;
	mail_system = 0;
	mail_players = 0;
	remove_alarm(mail_alarm);
	mail_alarm = 0;

	write("Reset the \"mailadmin\" command.\n");
	return 1;
	/* notreached */

    case "stats":
	if (sizeof(args) != 1)
	{
	    notify_fail("Syntax: mailadmin stats\n");
	    return 0;
	}

	return mail_statistics();
	/* notreached */

    default:
	notify_fail("Unknown subcommand to \"mailadmin\".\n");
	return 0;
    }

    write("Impossible end of \"mailadmin\". Please report this.\n");
    return 1;
}
