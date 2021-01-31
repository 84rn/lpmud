#pragma save_binary

/*
 * /d/Genesis/lib/post.c
 *
 * Funcions needed to make a room a post office. Moditied from the
 * post office from the Rockfriend guild created by Mercade.
 *
 * /Glindor, 10 October 1994
 *
 * Revision history:
 */

/*
 * You need to call post_init() from init()
 * and post_leave_inv() from leave_inv() in your room
 * to make it a post office.
 */

#define MAILCHECKER   "/secure/mail_checker"
#define MAILREADER    "/secure/mail_reader"
#define MAILREADER_ID "_reader_"

string
get_std_use()
{
    return "Check the mailreader for instructions.\n";
/*"Available commands are:\n" +
        "from [new]       List all [unread] headers.\n" +
	"read             Start up the mailreader command mode.\n" +
	"read <message>   Read message number <message> from your mailbox.\n" +
	"mail <name>      Mail to player(s) <name>.\n";
*/
}

/*
 * Function name: add_aliases
 * Description  : This function can do things to the mailreader before it
 *                is given to the player, like adding local aliases.
 * Arguments    : reader - the mailreader object
 */
void
add_aliases(object reader)
{}

void
post_init()
{
    object tp = this_player();
    object reader;

    /* If the player does not already have a mailreader (ie he is not a
     * wizard), then we clone a reader.
     */

    if (!objectp(reader = present(MAILREADER_ID, tp)))
    {
	seteuid(getuid());
	reader = clone_object(MAILREADER);

	/* We set the alias to the reader and move it into the player.
	 */
	add_aliases(reader);
	reader->move(tp, 1);
    }
}

void
post_leave_inv(object obj, object to)
{
    object reader;

    /* A wizard gets to keep his/her mailreader. */
    if (obj->query_wiz_level())
    {
	return;
    }

    /* Remove mailreader from players. */
    if (reader = present(MAILREADER_ID, obj))
	reader->remove_object();
}

/*
 * Function name: mail_message
 * Description  : This function is called from query_mail to give the player
 *                a message about the (new) mail in his mailbox.
 * Arguments    : new - ""/"NEW"/"UNREAD" depending on the status of
 *                      the mailbox.
 */
void
mail_message(string new)
{
    write("There is"+new+" mail for you in the nearest post office.\n");
}

/*
 * Function name: query_mail
 * Description  : This function is called when a player logs on to give him
 *                a message about the (new) mail in his mailbox.
 * Arguments    : silent - Set to 1 to not tell the player about the mail.
 * Returns      : 0 - no mail
 *                1 - mail, all read
 *                2 - new mail
 *                3 - unread mail
 */
public int
query_mail(int silent)
{
    int    mail = MAILCHECKER->query_mail();
    string new;

    if (mail == 0)
    {
	return 0;
    }
    if (silent)
    {
	return mail;
    }

    switch(mail)
    {
        case 2:
	    new = " NEW";
	    break;

	case 3:
	    new = " UNREAD";
	    break;

	default:
	    new = "";
    }

    mail_message(new);

    return mail;
}