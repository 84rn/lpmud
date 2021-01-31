/* 
 * /secure/admin/siteban.c
 *
 * This module maintains the list of banned sites
 */

static int checklist(string data, string ipnumber);
static int list_numbers(int which);
static int add_numbers(int which, string ipnumber);
static int remove_numbers(int which, string number);

#define NONEW   0
#define NOENTRY 1

string NoNewList;	/* The blocked sites in a string */
string NoEntryList;	/* The blocked sites in a string */

/*
 * Function name: check_newplayer
 * Description  : This function can be used to check whether someone is
 *                allowed to log in or create a new player.
 * Arguments    : string ipnumber - the ip number to check.
 * Returns      : int - 0 - oke to connect/create.
 *                      1 - no connections from this site allowed.
 *                      2 - no new players from this site allowed.
 */
int
check_newplayer(string ipnumber)
{
    if (strlen(NoEntryList) &&
	checklist(NoEntryList, ipnumber))
    {
	return 1;
    }
    
    if (strlen(NoNewList) &&
	checklist(NoNewList, ipnumber))
    {
	return 2;
    }

    return 0;
}

static int
checklist(string data, string ipnumber)
{
    mixed list, locklist, cmplist;
    int i, j, nope, s;
    
    list = explode(data, " ");
    
#if WHY_DONT_WE_USE_WILDMATCH
    for (i = 0 ; i < sizeof(list) ; i++) {
	locklist = explode(list[i], ".");
	cmplist = explode(ipnumber, ".");
	for (j = 0 ; j < sizeof(locklist) ; j++) {
	    nope = 1;
	    if (locklist[j] != cmplist[j]) {
		nope = 0;
		break;
	    }
	}
	
	if (nope)
	    return 1;
    }
    
    return 0;
#endif

    for (i = 0, s = sizeof(list); i < s; i++)
        if (wildmatch(list[i], ipnumber))
            return 1;

    return 0;
    
}

/*
 * Function name: lockout_site
 * Description:   Lock out newplayer logins from certain sites.
 */
int
lockout_site(string arg)
{
    mixed list;
    string args, cmd;
    int i;

    if (!CALL_BY(WIZ_CMD_ARCH))
    {
	return 0;
    }
    
    list = explode(arg, " ");
    
    cmd = list[0];
    
    if (sizeof(list) > 1) {
	for (i = 0 ; i < sizeof(list) - 1 ; i++) {
	    list[i] = list[i + 1];
	}
	list[i] = "";
	args = implode(list, " ");
    }
    
    if (cmd == "list")
	return list_numbers(NONEW);
    
    if (cmd == "add")
	return add_numbers(NONEW, args);
    
    if (cmd == "remove")
	return remove_numbers(NONEW, args);
    
    notify_fail("lockout: No such subcommand.\n");
    return 0;
}

/*
 * Function name: block_site
 * Description:   Lock out player logins from certain sites.
 */
int
block_site(string arg)
{
    mixed list;
    int i;
    string cmd, args;

    if (!CALL_BY(WIZ_CMD_ARCH))
    {
	return 0;
    }
    
    list = explode(arg, " ");
    
    cmd = list[0];
    
    if (sizeof(list) > 1) {
	for (i = 0 ; i < sizeof(list) - 1 ; i++)
	{
	    list[i] = list[i + 1];
	}
	list[i] = "";
	args = implode(list, " ");
    }
    
    if (cmd == "list") 
	return list_numbers(NOENTRY);
    
    if (cmd == "add") 
	return add_numbers(NOENTRY, args);
    
    if (cmd == "remove") 
	return remove_numbers(NOENTRY, args);
    
    notify_fail("block: No such subcommand.\n");
    return 0;
}

/*
 * Routines for banishing new player creation on a specific site.
 */

/*
 * Function name: add_numbers
 * Description:   Adds a site to the list
 */
static int
add_numbers(int which, string ipnumber)
{
    mixed oldlist, newlist;
    int i, j;
    
    if (!ipnumber) {
	notify_fail("Syntax: lockout/block add ipnumber\n");
	return 0;
    }
    
    if(which == NONEW) {
	if (NoNewList)
	    oldlist = explode(NoNewList, " ");
    }
    else {
	if (NoEntryList)
	    oldlist = explode(NoEntryList, " ");
    }
    
    i = 0;
    if (oldlist) {
	i = sizeof(oldlist);
	newlist = allocate(i + 1);
	for (j = 0 ; j < i ; j++) {
	    newlist[j] = oldlist[j];
	}
    }
    else 
	newlist = allocate(1);
    
    newlist[i] = ipnumber;
    if (which == NONEW)
	NoNewList = implode(newlist, " ");
    else
	NoEntryList = implode(newlist, " ");
    save_master();
    
    write("Added site: '" + ipnumber + "' to list.\n");
    return 1;
    
}

/*
 * Function name: remove_numbers
 * Description:   Removes a site from the list
 */
static int
remove_numbers(int which, string number)
{
    
    mixed oldlist, newlist;
    int nr, i, j, x;
    string command;
    
    if (!number) {
	notify_fail("Syntax: lockout/block remove ipnumber#\n");
	return 0;
    }
    
    if (which == NONEW) {
	if(!NoNewList) {
	    notify_fail("No ipnumbers stored in list.\n");
	    return 0;
	}
	oldlist = explode(NoNewList, " ");
    } else {
	if(!NoEntryList) {
	    notify_fail("No ipnumbers stored in list.\n");
	    return 0;
	}
	oldlist = explode(NoEntryList, " ");
    }
    
    i = sizeof(oldlist);
    
    (void)sscanf(number, "%d", nr);
    
    if (nr > i) {
	notify_fail("There is only " + i + " ipnumbers stored in the list.\n");
	return 0;
    }
    
    if (i == 1) {
	write("Number #1: '" + oldlist[0] + "' removed.\n");
	if (which == NONEW)
	    NoNewList = 0;
	else
	    NoEntryList = 0;
	save_master();
	return 1;
    }
    
    x = 0;
    newlist = allocate(i - 1);
    
    for (j = 0 ; j < i - 1 ; j++) {
	
	if (j + 1 == nr) {
	    command = oldlist[j];
	    x = 1;
	}
	
	newlist[j] = oldlist[j + x];
	
    }
    
    if (nr == i)
	command = oldlist[i - 1];
    
    write("Site #" + nr + ": '" + command + "' removed.\n");
    if (which == NONEW)
	NoNewList = implode(newlist, " ");
    else
	NoEntryList = implode(newlist, " ");
    save_master();
    return 1;
}

/*
 * Function name: list_numbers
 * Description:   List the stored numbers.
 */
static int
list_numbers(int which)
{
    
    mixed thelist;
    int i;
    
    if (which == NONEW) {
	if (!NoNewList) {
	    notify_fail("No numbers stored.\n");
	    return 0;
	}
	thelist = explode(NoNewList, " ");
    } else {
	if (!NoEntryList) {
	    notify_fail("No numbers stored.\n");
	    return 0;
	}
	thelist = explode(NoEntryList, " ");
    }
    
    for (i = 0 ; i < sizeof(thelist) ; i++)
	write("#" + (i + 1) + ": " + thelist[i] + "\n");
    
    return 1;
    
}

