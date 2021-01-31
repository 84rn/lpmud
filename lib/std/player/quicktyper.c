/*
 * /std/player/quicktyper.c
 *
 * This module is the quicktyper. It takes care of aliases and do
 * sequences.
 */

#include <macros.h>
#include <std.h>
#include <options.h>

#define ALIAS_LENGTH (query_wiz_level() ? 45 : 30)

/*
 * Global variables, all static, i.e. non-savable.
 */
static private string last_command = "";
static private int    paused       = 0;
static private string do_sequence  = "";
static private int    do_alarm     = 0;

/* 
 * Prototypes.
 */
static nomask int alias(string str);
static nomask int doit(string str);
static nomask int old_doit(string str);
static nomask int resume(string str);
static nomask int old_resume(string str);
static nomask int unalias(string str);

/*
 * Function name: init_cmdmodify
 * Description  : The quicktyper commands are added to the player.
 */
static nomask void
init_cmdmodify()
{
    add_action(alias,   "alias");	// alias.
    add_action(old_doit,"do");		// wykonaj.
    add_action(doit,	"wykonaj");	
    add_action(resume,  "dokoncz");	// dokoncz.
    add_action(unalias, "unalias");	// usunieta. ( alias - )
}

/*
 * Function name: modify_command
 * Description  : This function is called when it is time to modify a
 *                command. It resolves aliases and takes care of the
 *                remaining history functionality.
 * Arguments    : string str - The command to modify.
 * Returns      : string     - The modified command.
 */
nomask public string
modify_command(string str)
{
    string *words;
    string *subst_words;

    if (!strlen(str))
	return 0;

    /* Player wants to repeat the last command. */
    if (str == "%%")
    {
        if (this_player()->query_option(OPT_ECHO))
            write("Wykonuje: " + last_command + "\n");

        return last_command;
    }

    words = explode(str, " ");

    /* Resolve for aliases. */
    if (m_alias_list[words[0]])
    {
        /* Replace the first word with the aliased string. */
        words[0] = m_alias_list[words[0]];

        /* If the aliased string containts the text '%%', this means to
         * replace that '%%' with the remaining words of the command line. */
        if (wildmatch("*%%*", words[0]))
        {
            subst_words = explode(words[0], "%%");
            str = subst_words[0] + implode(words[1..] - ({""}), " ")
                + implode(subst_words[1..], "");

            /* Usuwanie spacji */
            str = implode(explode(str, " ") - ({""}), " ");
	}
	else
            /* Usuwanie spacji */
            str = implode(words - ({""}), " ");
    }
    else
        /* Usuwanie spacji */
        str = implode(words - ({""}), " ");

    /* Save the last command given to be retrieved with %%. */
    last_command = str;
    return str;
}

/*
 * Function name: alias
 * Description  : Make an alias, or display one or all current alias(es).
 * Arguments    : string str - the command line argument.
 * Returns      : int - 1/0 - success/failure.
 */
static nomask int
alias(string str)
{
    int    index;
    int    size;
    string a;
    string cmd;
    string *list;

    /* No-one can be forced to make an alias. */
    if (this_interactive() != this_object())
    {
	return 0;
    }

    /* List all aliases. */
    if (!stringp(str))
    {

	list = sort_array(m_indices(m_alias_list));
	size = sizeof(list);
	
	if (!size)
	{
	     write("Nie zdefiniowal" + koncowka("es", "as") + " jeszcze " +
	         "zadnych aliasow.\n");
	     return 1;
	}
	
	index = -1;

	write("Twoje aktualne aliasy to:\n\n");
	while(++index < size)
	{
	    write(sprintf("%-8s: %s\n", list[index],
		m_alias_list[list[index]]));
	}
	return 1;
    }

    /* List one alias. */
    if (m_alias_list[str])
    {
	write(sprintf("%-8s: %s\n", str, m_alias_list[str]));
	return 1;
    }

    /* Add a new alias, must consist of a name and a value, else we assume
     * the player wanted to display a non-existant alias.
     */
    if (sscanf(str, "%s %s", a, cmd) != 2)
    {
	notify_fail("Nie ma takiego aliasa.\n");
	return 0;
    }
    /* Gracz moze rowniez chciec usunac alias.
     */
    if (a == "-")
    {
	if (!m_alias_list[cmd]) 
	{
    	    notify_fail("Alias '" + cmd + "' nie istnieje!\n");
    	    return 0;
 	}
 	
 	write("Ok, alias '" + cmd + "' usuniety. Oznaczal " + 
 	    m_alias_list[cmd] + ".\n");
	m_alias_list = m_delete(m_alias_list, cmd);
	
	return 1;
    }

    /* Delete the alias if is already exists.*/
    if (m_alias_list[a])
    {
        write("Poprzednio '" + a + "' oznaczalo '" + m_alias_list[a] +
            "'.\n");
	m_alias_list = m_delete(m_alias_list, a);
    }

    /* See whether there is room for yet another alias. */
    if (m_sizeof(m_alias_list) >= ALIAS_LENGTH)
    {
        write("Przykro mi, ale nie mozesz miec wiecej aliasow, niz " +
            ALIAS_LENGTH + ". Usun troche, a bedziesz mogl" + 
            this_player()->koncowka("", "a") + " dodac nowe.\n");
            
	return 1;
    }
    
    if (a == "alias")
    {
        write("Niestety, nie mozesz aliasowac komendy 'alias'.\n");
        return 1;
    }

    m_alias_list[a] = cmd;
    write("Alias '" + a + "' od teraz oznacza '" + cmd + "'.\n");
    return 1;
}

/*
 * Function name: remove_do_alarm
 * Description  : This is a service function. Call it to remove the do-alarm
 *                from a player if necessary.
 */
public nomask void
remove_do_alarm()
{
    remove_alarm(do_alarm);
    do_alarm = 0;
}

/*
 * Function name: do_chain
 * Description  : Do next command in the do chain.
 */
static nomask void
do_chain()
{
    string cmd;
    string rest;

    /* Player is in combat, no do sequences allowed. */
    if (objectp(query_attack()))
    {
	remove_alarm(do_alarm);
	paused = 1;

	tell_object(this_object(),
	    "Wykonywnie sekwencji komend zostalo zwieszone, gdyz " +
	    "jestes zangazowan" + this_player()->koncowka("y", "a") +
	    " w walke. Pozniej bedziesz mogl" + 
	    this_player()->koncowka("", "a") + " 'dokonczyc' sekwencje.\n");
	return;
    }

    /* Get the first part of the command and execute it. */
    if (sscanf(do_sequence, "%s,%s", cmd, rest) == 2)
    {
	if (strlen(rest))
	{
	    tell_object(this_object(), "Wykonuje: " + cmd + "\n");
	    do_sequence = rest;
	    this_object()->command(cmd);
	    return;
	}
	do_sequence = cmd;
    }

    remove_alarm(do_alarm);
    tell_object(this_object(), "Wykonuje: " + do_sequence + "\n");
    this_object()->command(do_sequence);
    tell_object(this_object(), "Wykonywanie sekwencji zakonczone.\n");
    do_sequence = "";
}

/* 
 * Function name: doit
 * Description  : Do a sequence of commands.
 * Arguments    : string str - the command line argument.
 * Returns      : int - 1/0 - success/failure.
 */
static nomask int
doit(string str)
{
    /* Access failure. You cannot be forced to 'do' anything. */
    if (this_interactive() != this_object())
    {
	return 0;
    }

    /* No argument. If a 'do' is going on, pause it. */
    if (!strlen(str))
    {
	if (!strlen(do_sequence))
	{
	    notify_fail("Stosowanie: wykonaj komende1,komende2,...\n");
	    return 0;
	}

	write("Zawieszone 'wykonaj'.\n" +
	      "W razie potrzeby mozesz je 'dokonczyc'.\n");
	remove_alarm(do_alarm);
	paused = 1;
	return 1;
    }

    /* If a 'do' is going on and it is not paused, reject the call. */
    if (strlen(do_sequence) &&
	!paused)
    {
	write("Jest juz wykonywana inna sekwencja:\n  " + do_sequence + "\n");
	return 1;
    }

    /* There is a 'do', but it has been paused, skip the paused commands. */
    if (strlen(do_sequence))
    {
	write("Zaniechana sekwencja:\n   " + do_sequence + "\n");
	paused = 0;
    }

    /* Disallow do-commands to be executed while the player is in combat. */
    if (objectp(query_attack()))
    {
	write("Nie mozesz 'wykonywac' zadnych sekwencji, gdyz jestes " +
	    "zaangazowan" + this_player()->koncowka("y", "a") + 
	    " w walke.\n");
	return 1;
    }

    /* Start the new 'do'. The first alarm is in 0.0 seconds since we want
     * that to be executed immediately.
     */
    do_sequence = str;
    do_alarm = set_alarm(0.0, 2.0, do_chain);

    return 1;
}

static nomask int
old_doit(string str)
{
    notify_fail("Komenda 'do' zostala wycofana. Zamiast niej mozesz " + 
        "mozesz uzyc 'wykonaj'.\n");
        
    return 0;
}

/* 
 * Function name: resume
 * Description  : Resume the processing of the commands in the do chain.
 * Arguments    : string str - the command line argument.
 * Returns      : int - 1/0 - success/failure.
 */
static nomask int
resume(string str)
{
    /* Access failure. You cannot be forced to resume. */
    if (this_interactive() != this_object())
    {
	return 0;
    }

    /* No argument possible. */
    if (stringp(str))
    {
	notify_fail("Dokoncz co? Po prostu 'dokoncz' aby podjac " +
	    "zawieszone wykonywanie sekwencji.\n");
	return 0;
    }

    /* There is no 'do' going on or it isn't paused. */
    if (!strlen(do_sequence) ||
	!paused)
    {
	write("Co chcesz dokonczyc? Zadna sekwencja nie zostala " +
	    "zawieszona.\n");
	return 1;
    }

    /* Disallow players to resume a do while they are in combat. */
    if (objectp(query_attack()))
    {
	write("Nie mozesz dokonczyc wykonywania sekwencji komend, gdyz "+
	    "jestes zaangazowan" + this_player()->koncowka("y", "a") + 
	    " w walke.\n");
	return 1;
    }

    /* Resume the 'do'. */
    paused = 0;
    do_alarm = set_alarm(0.0, 2.0, do_chain);
    return 1;
}

static nomask int
old_resume(string str)
{
    notify_fail("Komenda 'resume' zostala wycofana. Zamiast niej " +
        "mozesz uzyc 'dokoncz'.\n");
}

/*
 * Function name: unalias
 * Description  : Remove an alias.
 * Arguments    : string str - the command line argument.
 * Returns      : int - 1/0 - success/failure.
 */
static nomask int
unalias(string str)
{
    notify_fail("Komenda 'unalias' zostala usunieta. Zamiast niej " +
        "mozesz uzyc 'alias -'.\n");
        
    return 0;
}
