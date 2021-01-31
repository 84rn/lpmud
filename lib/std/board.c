/*
 * /std/board.c
 *
 * The standard board code. Several options can be set:
 *
 * set_board_name(name)	 Path to the save directory for the board (no
 *			 default, this variable MUST be set) Old notes
 *			 will be stored in a directory with the same name,
 *			 with _old appended, if that option is checked.
 * set_num_notes(n)	 Set max number of notes. (default 30)
 * set_silent(n)	 0 = Tell room about read and examine of the board.
 *			 1 = silent read & examine. (default)
 * set_remove_rank(n)	 Minimum rank required to remove other players
 *			 notes. (default: WIZ_LORD, ie lords++)
 * set_remove_str(str)	 The string to send to the player if a remove
 *			 failed. (default "Only a Lord or higher can remove
 *			 other peoples notes.\n")
 * set_show_lvl(n)	 0 = Don't show wizard-rank in note header.
 *			 1 = show rank of writer in note header. (default)
 * set_no_report(n)	 0 = Keep the central board notified. (default)
 *			 1 = Don't notify the central board.
 *                           This hides the board for tools.
 * set_keep_discarded(n) 0 = don't keep old notes (default).
 *			 1 = keep old notes.
 *
 * There are three functions you can use to restrict usage of the board.
 * Independantly of the 'normal' access rules these functions can be used
 * to block/allow additional access. The administration and the owners of
 * the board are always allowed to manipulate the boards. In addition,
 * everyone is allowed to read or remove his/her own note. Usually read
 * and write access are blocked the same way. The right to remove other
 * peoples notes should be allowed separately. Hence the different names
 * and uses of these three functions:
 *
 * int block_reader()
 * int block_writer()
 * int allow_remove()
 *
 * You should only use these functions for board-internal use. To check the
 * access rights for people externally, there are three other functions.
 * This is to ensure some basic access rights for people who need to have
 * that and prevent that those rights are affected by the three forementioned
 * functions. Never call the block_ and allow_ functions externally. Always
 * call check_ externally. The functions return true to block access.
 *
 * int check_reader()
 * int check_writer()
 * int check_remove()
 *
 * The header format is:
 * subject (37)  0..36
 * rank    (12) 38..49 (optional, eg. "Praktykantka")
 * author  (11) 51..61 (eg. "Silvathraec")
 * length   (3) 63..65
 * date     (7) 68..74 (2 characters for day, 4 for month, eg. "30 VIII")
 */

#pragma save_binary
#pragma strict_types

inherit "/std/object";

#include <files.h>
#include <macros.h>
#include <std.h>
#include <stdproperties.h>

/* It is not allowed to read notes larger than 100 lines without more. This
 * is done to prevent errors when trying to write too much text to the screen
 * of the player at once.
 */
#define MAX_NO_MREAD      100
#define MIN_HEADER_LENGTH   3
#define MAX_HEADER_LENGTH  37
#define MIN_RANK_LENGTH     3
#define MAX_RANK_LENGTH    12
#define MIN_NAME_LENGTH     3
#define MAX_NAME_LENGTH    11
#define RANK_BEGIN         38
#define AUTHOR_BEGIN       51
#define AUTHOR_END         61
#define LENGTH_BEGIN       63
#define LENGTH_END         65

#define HEADER_FORMAT      "%-37s %12s %-11s %s  %-7s"
#define RENAME_HEADER_FORMAT   "%s%12s %-11s%s"
#define LEGAL_CHARS        "abcdefghijklmnopqrstuvwxyz'-"
#define GUEST_NAME         "gosc"
#define MIN_WRITE_AGE	   43200

/*
 * Global variables. They are not savable, the first two are private too,
 * which means that people cannot dump them.
 */
static private mixed   headers = ({ });
static private mapping writing = ([ ]);

static string  board_name = "";
static string  remove_str = "Tylko Lord lub administrator moze kasowac notki innych graczy.\n";
static int     keep_discarded = 0;
static int     notes = 30;
static int     silent = 1;
static int     msg_num;
static int     remove_rank = WIZ_LORD;
static int     show_lvl = 1;
static int     no_report = 0;
static int     fuse = 0;

/*
 * Prototypes.
 */
public nomask  int list_notes(string str);
public nomask  int new_msg(string msg_head);
public varargs int read_msg(string what_msg, int mr);
public nomask  int remove_msg(string what_msg);
public nomask  int rename_msg(string str);
public nomask  int store_msg(string str);
nomask private string *extract_headers(string str);

/*
 * Function name: set_num_notes
 * Description  : Set the maximum number of notes on the board.
 * Arguments    : int n - the number of notes on the board. (default: 30)
 */
public nomask void
set_num_notes(int n)
{
    notes = (fuse ? notes : n);
}

/*
 * Function name: set_silent
 * Description  : Set this to make the board silent. That means that
 *                bystanders are not noticed of people examining the
 *                board and reading notes.
 * Arguments    : int n - true if silent (default: true)
 */
public nomask void
set_silent(int n)
{
    silent = (fuse ? silent : (n ? 1 : 0));
}

/*
 * Function name: set_no_report
 * Description  : If you set this to true, the central board master is
 *                not notified of new notes. The only thing this does is
 *                preventing MBS from showing the board.
 * Arguments    : int n - true if the central board master should not
 *                        be notified. (default: false)
 */
public nomask void
set_no_report(int n)
{
    no_report = (fuse ? no_report : (n ? 1 : 0));
}

/*
 * Function name: set_board_name
 * Description  : This function sets the path in which the notes on the
 *                board are kept. This must _always_ be set!
 * Arguments    : strint str - the pathname of the board. It should not
 *                             end with a /.
 */
public nomask void
set_board_name(string str)
{
    board_name = (fuse ? board_name : str);
}

/*
 * Function name: query_board_name
 * Description  : Return the name of the board. This is the path of the
 *                directory in which the notes are stored.
 * Returns      : string - the name of the board (path-name).
 */
public nomask string
query_board_name()
{
    return board_name;
}

/*
 * Function name: set_remove_rank
 * Description  : With this function you can set the minimum rank people
 *                must have to be able to remove other peoples notes.
 *                NOTE: The argument to this function must be a define
 *                      from std.h, ie WIZ_NORMAL, WIZ_LORD, etc.
 * Arguments    : int n - the minimum level (default WIZ_LORD, ie lords++).
 */
public nomask void
set_remove_rank(int n)
{
    remove_rank = (fuse ? remove_rank : n);
}

/*
 * Function name: set_remove_str
 * Description  : Set the string to be printed when a player tries to
 *                remove a note while he is not allowed to do so.
 * Arguments    : string str - the message (default: Only a Lord is
 *                             allowed to remove other peoples notes.)
 */
public nomask void
set_remove_str(string str)
{
    remove_str = (fuse ? remove_str : str);
}

/*
 * Function name: set_show_lvl
 * Description  : Set this if you want the rank of the authors of the notes
 *                added to the header of the notes.
 * Arguments    : int n - true if the rank should be show (default: true)
 */
public nomask void
set_show_lvl(int n)
{
    show_lvl = (fuse ? show_lvl : (n ? 1 : 0));
}

/*
 * Function name: set_keep_discarded
 * Description  : Set this if you want old notes to be kept. They will be
 *                kept if in a directory with the same name as the
 *                actual notes, though with _old appended to it.
 * Arguments    : int n - true if old notes should be kept (default: false)
 */
public nomask void
set_keep_discarded(int n)
{
    keep_discarded = (fuse ? keep_discarded : (n ? 1 : 0));
}

/*
 * Function name: query_author
 * Description  : Return the name of the author of a certain note. Observe
 *                that the note-numbers start counting at 1, like the way
 *                they appear on the board.
 * Argument     : int note - the number of the note to find the author of.
 * Returns      : string - the name of the author or 0 if the note doesn't
 *                         exist.
 */
public nomask string
query_author(int note)
{
    if (note < 1 || note > msg_num)
	return 0;

    return lower_case(explode(headers[note - 1][0][AUTHOR_BEGIN..AUTHOR_END],
                              " ")[0]);
}

/*
 * Function name: no_special_fellow
 * Description  : Some people can always handle the board. If you own the
 *                board (same euid), if the board is in your domain, or
 *                if you are a member of the administration or the Lord of
 *                the domain the board it in, you can always meddle.
 * Returns      : int - 1/0 - true if (s)he is not a special fellow.
 */
nomask public int
no_special_fellow()
{
    string name = this_interactive()->query_real_name();
    string euid = geteuid();

    /* I'm an arch or keeper, so I can do anything. */
    if (SECURITY->query_wiz_rank(name) >= WIZ_ARCH)
    {
	return 0;
    }

    /* The board is mine */
    if (name == euid)
    {
	return 0;
    }

    /* The board is my domains */
    if (euid == SECURITY->query_wiz_dom(name))
    {
	return 0;
    }

    /* I am Lord and the board is of one of my wizards. */
    if ((SECURITY->query_wiz_rank(name) == WIZ_LORD) &&
	(SECURITY->query_wiz_dom(name) == SECURITY->query_wiz_dom(euid)))
    {
	return 0;
    }

    return 1;
}

/*
 * Function name: block_reader
 * Description  : If this_player() is not allowed to read notes of other
 *                people on this board, this function should be used to
 *                block access. If you print an error message on failure,
 *                please check whether this_player() is in the environment
 *                of the board.
 * Returns      : int - true if the player is NOT allowed to read.
 */
public int
block_reader()
{
    return 0;
}

/*
 * Function name: check_reader
 * Description  : This function will return true if this_player() is allowed
 *                to read on the board. Note that players may always read
 *                their own notes.
 * Arguments    : int note - an optional argument for a particular note to
 *                           be checked. If omitted give general access to
 *                           the board.
 * Returns      : int 1/0 - true if the player is prohibited to read.
 */
public nomask varargs int
check_reader(int note = 0)
{
    if (note &&
	(this_player()->query_real_name() == query_author(note)))
    {
	return 0;
    }

    return (block_reader() ? no_special_fellow() : 0);
}

/*
 * Function name: block_writer
 * Description  : If this_player() is not allowed to write notes on this
 *                board, this function should be used to block access.
 *                If you print an error message on failure, please check
 *                whether this_player() is in the environment of the board.
 * Returns      : int 1/0 - true if the player is NOT allowed to write.
 */
public int
block_writer()
{
    return 0;
}

/*
 * Function name: check_writer
 * Description  : This function will return true if this_player() is allowed
 *                to write on the board.
 * Returns      : int 1/0 - true if the player may not write.
 */
public nomask int
check_writer()
{
    return (block_writer() ? no_special_fellow() : 0);
}

/*
 * Function name: allow_remove
 * Description  : This function checks whether this_player() is allowed
 *                to remove notes from this board. If you print an error
 *                message on failure, please check whether this_player()
 *                is in the environment of the board. This function works
 *                independant of the set_remove_rank function.
 * Returns      : int - true if the player is allowed to remove notes.
 */
public int
allow_remove()
{
    return 0;
}

/*
 * Function name: check_remove
 * Description  : This function will return true if this_player() is allowed
 *                to remove (other peoples) notes from the board.
 * Arguments    : int note - an optional argument for a particular note to
 *                           be checked. If omitted give general access to
 *                           the board.
 * Returns      : int 1/0 - true if the player may not remove the note.
 */
public nomask varargs int
check_remove(int note = 0)
{
    if (note &&
	(this_player()->query_real_name() == query_author(note)))
    {
	return 0;
    }

    return (no_special_fellow() &&
	    (SECURITY->query_wiz_rank(this_player()->query_real_name()) <
	     remove_rank) &&
	    !allow_remove());
}

/*
 * Function name: load_headers
 * Description  : Load the headers when the board is created. This is done
 *                in a little alarm since it is also possible to configure
 *                the board by cloning it and then calling the set-functions
 *                externally. This function also sets the fuse that makes
 *                it impossible to alter the board-specific properties.
 */
private nomask void
load_headers()
{
    string *arr;

    /* Set the fuse to make it impossible to alter any of the board-specific
     * properties.
     */
    fuse = 1;

    seteuid(getuid());

    arr = get_dir(board_name + "/b*");
    msg_num = sizeof(arr);
    headers = (msg_num ? map(arr, extract_headers) : ({ }) );
}

/*
 * Function name: create_board
 * Description  : Since create_object() is nomasked, you must redefine this
 *                function in order to create your own board.
 */
void
create_board()
{
}

/*
 * Function name: create_object
 * Description  : Create the object. Use create_board() to set up the
 *                board yourself.
 */
public nomask void
create_object()
{
    ustaw_nazwe(({"tablica", "tablicy", "tablicy", "tablice", "tablica",
                  "tablicy"}),
                ({"tablice", "tablic", "tablicom", "tablice", "tablicami",
                  "tablicach" }), PL_ZENSKI);

    dodaj_przym("ogloszeniowy", "ogloszeniowi");

    ustaw_shorty(({"tablica ogloszeniowa", "tablicy ogloszeniowej",
                   "tablicy ogloszeniowej", "tablice ogloszeniowa",
                   "tablica ogloszeniowa", "tablicy ogloszeniowej"}),
                 ({"tablice ogloszeniowe", "tablic ogloszeniowych",
                   "tablicom ogloszeniowym", "tablice ogloszeniowe",
                   "tablicami ogloszeniowymi", "tablicach ogloszeniowcych"}),
                 PL_ZENSKI);

    set_long("Jest to tablica ogloszeniowa, na ktorej mozesz umieszczac "
           + "adresowane do innych notki ku nauce, przestrodze, rozwadze czy "
           + "tez uciesze czytelnikow.\n");

    add_prop(OBJ_M_NO_GET,
             "Tablica ogloszeniowa jest zbyt mocno przymocowana.\n");

    create_board();

    /* We must do this with an alarm because it is also possible to clone
     * the /std/board.c and confugure it with the set_functions.
     */
    set_alarm(0.5, 0.0, load_headers);

    enable_reset();
}

/*
 * Function name: reset_board
 * Description  : Since reset_object() is nomasked, you must mask this
 *                function at reset time. Enable_reset() is already
 *                called, so you do not have to do that yourself.
 */
void
reset_board()
{
/* 
(!!!) czekam na dobry pomysl na zamiane. Alvin.
    if (!random(5))
    {
	tell_room(environment(),
	    "A small gnome appears and secures some notes on the " +
	    short() + " that were loose.\nThe gnome then leaves again.\n");
    }
*/
}

/*
 * Function name: reset_object
 * Description  : Every half hour or about, the object resets. Use
 *                reset_board() for user reset functionality.
 */
nomask void
reset_object()
{
    reset_board();
}

/*
 * Nazwa funkcji: count_notes
 * Opis         : Funkcja zwraca informacje na temat liczby notek na tablicy.
 */
public nomask string
count_notes()
{
    string str;

    if (!msg_num)
        return capitalize(short(PL_MIA)) + " jest pust"
             + koncowka("y", "a", "e") + ".\n";

    str = capitalize(short(PL_MIA)) + " zawiera "
        + LANG_SNUM(msg_num, PL_BIE, PL_ZENSKI) + " ";

    if (msg_num == 1)
        str += "notke";
    else if ((msg_num % 100) / 10 == 1 || !(msg_num % 10) ||
             msg_num % 10 > 4)
        str += "notek";
    else
        str += "notki";

    return str;
}

/*
 * Nazwa funkcji: list_headers
 * Opis         : Funkcja zwraca naglowki z podanego zakresu.
 */
public nomask string
list_headers(int start, int end)
{
    string str = "";

    /* Check whether the range is valid. */
    if (end < 1 || end > msg_num)
        end = msg_num;

    if (start > end)
        start = end;
    else if (start < 1)
        start = 1;

    start -= 2;

    /* If the player is not allowed to read the board, only display the
     * notes the player wrote him/herself.
     */
    if (check_reader())
    {
        string name = this_player()->query_real_name();

        while (++start < end)
            if (name == query_author(start + 1))
                str += sprintf("%2d: %s\n", start + 1, headers[start][0]);
    }
    else
        while (++start < end)
            str += sprintf("%2d: %s\n", start + 1, headers[start][0]);

    return str;
}

/*
 * Function name: long
 * Description  : This function returns the long description on the board.
 * Arguments    : string item    - pseudo-item do opisania.
 *                object for_obj - obiekt dla ktorego przenzczony jest opis.
 * Returns      : string - the long description.
 */
public nomask string
long(string item = 0, object for_obj = this_player())
{
    string str;

    if (item)
        return ::long(item, for_obj);

    str = check_call(query_long(), this_player())
        + "Komendy: napisz notke, usun [notke] <numer>, "
        + "przejrzyj [notki] [<odkad-dokad>],\n"
        + "         przeczytaj [notke] <numer>, "
        + "przeczytaj poprzednia/nastepna [notke]"
        + (this_player()->query_wiz_level() ?
           ",\n         zachowaj [notke] <numer> <nazwa pliku>" : "") + "\n\n"
        + count_notes();

    if (!msg_num)
        return str + ".\n";

    if (msg_num < 17)
        return str + ":\n" + list_headers(1, msg_num);

    return str + ". Oto najnowsze z nich:\n"
         + list_headers(msg_num - 15, msg_num);
}

/*
 * Function name: init
 * Description  : Link the commands to the player.
 */
public void
init()
{
    ::init();

    /* Only interactive players can write notes on boards. */
    if (!query_ip_number(this_player()))
    {
	return;
    }

    add_action(list_notes, "przejrzyj");
    add_action(new_msg,    "napisz");
    add_action(read_msg,   "przeczytaj");
//    add_action(read_msg,   "mread");
    add_action(remove_msg, "usun");
    add_action(rename_msg, "rename");
    add_action(store_msg,  "zachowaj");
}

/*
 * Function name: extract_headers
 * Description  : This is a map function that reads the note-file and
 *                extracts the headers of the note.
 * Arguments    : string str - the name of the note.
 * Returns      : string* - ({ header-string, note-name })
 */
private nomask string *
extract_headers(string str)
{
    string title;
    int time;

    seteuid(getuid());

    if (sscanf(str, "b%d", time) != 1)
	return 0;
    str = "b" + time;

    if (!stringp(title = read_file(board_name + "/" + str, 1, 1)))
	return 0;

    return ({ extract(title, 0, strlen(title) - 2), str });
}

/*
 * Function name: query_latest_note
 * Description  : Find and return the name of the last note on the board.
 * Returns      : string - the filename (without path) of the last note
 *                         on the note or 0 if no notes are on the board.
 */
public nomask string
query_latest_note()
{
    return (msg_num ? headers[msg_num - 1][1] : 0);
}

/*
 * Function name: query_num_messages
 * Description  : Find a return the number of messages on this board.
 * Returns      : int - the number of messages on the board.
 */
public nomask int
query_num_messages()
{
    return msg_num;
}

/*
 * Function name: list_notes
 * Description  : With this command a player can list only a portion of
 *                the board.
 * Arguments    : string str - the command line argument.
 * Returns      : int 1/0 - success/failure.
 */
public nomask int
list_notes(string str)
{
    string from, to;
    int start, end;

    if (!str || str == "notki")
        str = "-";
    else
        sscanf(str, "notki %s", str);

    if (sscanf(str, "%s-%s", from, to) != 2 ||
        !(start = atoi(from)) && from != "" ||
        !(end = atoi(to)) && to != "")
    {
        notify_fail("Zly zakres. Prawidlowo powinien miec forme "
                  + "\"odkad-dokad\", \"odkad-\" lub \"-dokad\".\n");
        return 0;
    }

    if (!silent && present(this_player(), environment()))
        saybb(QCIMIE(this_player(), PL_MIA) + " przeglada " + short(PL_BIE)
            + ".\n");

    if (!msg_num)
        write(count_notes() + ".\n");
    else
        write(count_notes() + ":\n" + list_headers(start, end));

    return 1;
}

/*
 * Nazwa funkcji: get_subject
 * Opis         : Po pobraniu i sprawdzeniu poprawnosci tytulu notki odpala
 *                edytor liniowy.
 * Argumenty    : msg_head: Tytul notki.
 */
public nomask void
get_subject(string msg_head)
{
    string *times;
    string date;
    string rank;

    switch (msg_head = implode(explode(msg_head, " ") - ({""}), " "))
    {
        case "":
            write("Brak tytulu notki.\n");
            return;

        case "~q":
            write("Pisanie notki zaniechane.\n");
            return;
    }

    if (strlen(msg_head) > MAX_HEADER_LENGTH)
    {
        write("Zbyt dlugi tytul notki, sprobuj jeszcze raz.\n");
        return;
    }

    if (strlen(msg_head) < MIN_HEADER_LENGTH)
    {
        write("Zbyt krotki tytul notki, sprobuj jeszcze raz.\n");
        return;
    }

    if (present(this_player(), environment()))
        saybb(QCIMIE(this_player(), PL_MIA) + " zaczyna pisac notke.\n");

    this_player()->add_prop(LIVE_S_EXTRA_SHORT,
        " piszac" + this_player()->koncowka("y", "a", "e") + " notke");

    /* We use an independant editor and therefore we must save the header
     * the player has typed. When the player is done editing, the header
     * will be used again to save the message. Don't you just love these
     * kinds of statements.
     */
    times = explode(ctime(time(), 1), " ");
    date = sprintf("%2s %-4s", times[1], times[2]);
    rank = show_lvl ?
        (SECURITY->query_wiz_pretitle(this_player()))[..MAX_RANK_LENGTH] : "";

    writing[this_player()] = sprintf(HEADER_FORMAT, msg_head, rank,
        capitalize(this_player()->query_real_name()), "???", date);

    seteuid(getuid());

    clone_object(EDITOR_OBJECT)->edit("done_editing", "");
}

/*
 * Nazwa funkcji: new_msg
 * Opis         : Gracz chce napisac notke.
 * Argumenty    : str: Argument komendy.
 */
public nomask int
new_msg(string str)
{
    if (str != "notke")
    {
        notify_fail("Co chcesz napisac? Moze notke?\n");
        return 0;
    }

    if (this_player()->query_real_name() == GUEST_NAME)
    {
        write("Jestes w Arkadii gosciem i jako taki nie jestes uprawniony do "
            + "pisania na tablicach ogloszeniowych. Jesli chcesz wziac "
            + "udzial w dyskusji, stworz sobie prawdziwa postac. Nowy gracz "
            + "jest zawsze mile widziany w Arkadii.\n");
        return 1;
    }
    
    if ((this_player()->query_age() < MIN_WRITE_AGE) &&
        (!this_player()->query_wiz_level()))
    {
	write("Niestety, osoby o wieku mniejszym niz 1 dzien nie moga " +
	    "zamiezczac notek na tablicach ogloszeniowych. Oberzyj troche " +
	    "swiata, zanim zaczniesz sie publicznie wypowiadac.\n");
	return 1;
    }

    /* Player is not allowed to write a note on this board. */
    if (check_writer())
    {
        notify_fail("Nie jestes uprawnion" + 
            this_player()->koncowka("y", "a", "y") + " by umieszczac "+
            "nowe notki.\n");
	return 0;
    }

    write("Podaj tytul notki: ");
    input_to(get_subject);
    return 1;
}

/*
 * Function name: block_discard
 * Description  : If this function returns 1, it will prevent the
 *		  removal of a note.
 * Arguments	: file - the file to remove
 */
public int
block_discard(string file)
{
    return 0;
}

/*
 * Function name: discard_message
 * Description  : When there are too many notes on the board, the oldest
 *                is discarded. It is either moved or deleted.
 * Arguments    : string file - the file (without path) to save.
 */
private nomask void
discard_message(string file)
{
    seteuid(getuid());

    if (block_discard(file))
	return;

    if (keep_discarded)
    {
	if (file_size(board_name + "_old") == -1)
	{
	    mkdir(board_name + "_old");
	}

	rename(board_name + "/" + file, board_name + "_old/" + file);
    }
    else
    {
	rm(board_name + "/" + file);
    }
}

/*
 * Function name: post_note_hook
 * Description  : This function is called when someone posts a note.
 * Arguments    : string head - the header of the note.
 */
public void
post_note_hook(string head)
{
    string dev_null;
    
    head = head[0..36];
    sscanf(head, "%s %s", head, dev_null);
    
    if (present(this_player(), environment()))
        saybb(QCIMIE(this_player(), PL_MIA) + " konczy pisanie notki " +
        "zatytulowanej \"" + head + "\", ktora otrzymuje numer " + 
        msg_num + ".\n");
}

/*
 * Function name: post_note
 * Description  : This function actually posts the note on the board,
 *                stores it to disk and notifies the board master.
 * Arguments    : string head    - the header of the note.
 *                string message - the message body.
 */
private nomask void
post_note(string head, string message)
{
    string author, fname;
    int    t;

    /* If there are too many notes on the board, remove them. */
    while(msg_num >= notes)
    {
        author = this_player()->query_real_name();

        /* Piata notka pod rzad */
        if (author == query_author(msg_num) &&
            author == query_author(msg_num - 1) &&
            author == query_author(msg_num - 2) &&
            author == query_author(msg_num - 3))
            log_file("board_recycle", ctime(time()) + ": "
                   + headers[msg_num - 1][0] + "\n");

	discard_message(headers[0][1]);
	headers = exclude_array(headers, 0, 0);
	msg_num--;
    }

    seteuid(getuid());

    /* If the directory doesn't exist, create it. */
    if (file_size(board_name) == -1)
    {
	mkdir(board_name);
    }

    /* Check that the message file isn't used yet. */
    fname = "b" + (t = time());
    while(file_size(board_name + "/" + fname) != -1)
    {
	fname = "b" + (++t);
    }

    /* Write the message to disk and update the headers. */
    write_file(board_name + "/" + fname, head + "\n" + message);
    headers += ({ ({ head, fname }) });
    msg_num++;

    /* Update the master board central unless that has been prohibited. */
    if (!no_report)
    {
	BOARD_CENTRAL->new_note(board_name, fname, MASTER_OB(environment()));
    }

    post_note_hook(head);
}

/*
 * Function name: done_editing
 * Description  : When the player is done editing the note, this function
 *                will be called with the message as parameter. If all
 *                is well, we already have the header.
 * Arguments    : string message - the note typed by the player.
 * Returns      : int - 1/0 - true if the note was added.
 */
public nomask int
done_editing(string message)
{
    string head;
    int size;

    this_player()->remove_prop(LIVE_S_EXTRA_SHORT);

    if (!strlen(message))
    {
	write("Nie wpisal" + this_player()->koncowka("es", "as", "es") + 
	    " zadnej informacji.\n");
	if (present(this_player(), environment()))
	{
	    saybb(QCIMIE(this_player(), PL_MIA) + " przerywa "+
	          "wpisywanie notki.\n");
	}

	writing = m_delete(writing, this_player());
	return 0;
    }

    if (!stringp(writing[this_player()]))
    {
    	write("Twoj naglowek ulegl zniszczeniu! Nie umiescil" +
    	    this_player()->koncowka("es", "as", "es") + " zadnej notatki.\n" +
    	    "Zglos blad!\n");
	return 0;
    }

    head = writing[this_player()];
    size = sizeof(explode(message, "\n"));
    head = head[..(LENGTH_BEGIN - 1)] + (size > 999 ? "***" :
           sprintf("%3d", size)) + head[(LENGTH_END + 1)..];
    writing = m_delete(writing, this_player());

    post_note(head, message);

    write("Ok.\n");
    return 1;
}

/*
 * Function name: create_note
 * Description  : This function can be called externally in order to write
 *                a note on a board generated by code. The note will then
 *                appear on the board just like any other note. There are a
 *                few restrictions to the note:
 *                - the euid of the object generating the note must be the
 *                  same as the euid of the board;
 *                - the name of the author may _not_ be the name of a player
 *                  in the game, mortal or wizard, nor may it be banished.
 *                In addition to that, you should ensure that the note is
 *                properly formatted and that it has newlines inserted at
 *                regular intervals, preferably before the 80th char/line.
 * Arguments    : string header - the header of the note (min: 3; max: 40)
 *                string author - the alleged author of the note (max: 11)
 *                string body   - the note itself.
 *                string rank   - optionally, the alleged rank (title, etc)
 *                                of author (max: 10)
 * Returns      : int 1/0 - success/failure.
 */
public nomask int
create_note(string header, string author, string body, string rank = "")
{
    string *times;
    string lower = lower_case(author);
    string head;
    int    index = -1;
    int    len;

    if (geteuid() != geteuid(previous_object()))
	return 0;

    /* The author may not be a real player and the length of the name
     * must be in the valid range. */
    len = strlen(lower);
    if (len > MAX_NAME_LENGTH || len < MIN_NAME_LENGTH ||
        SECURITY->exist_player(lower))
    {
	return 0;
    }

    /* Author's name may only be letters, dash (-), or apostrophe ('). */
    while(++index < len)
	if (!wildmatch("*" + lower[index..index] + "*", LEGAL_CHARS))
	{
	    return 0;
	}

    /* Rank has to be either empty or of valid length. */
    len = strlen(rank);
    if (len && (len > MAX_RANK_LENGTH || len < MIN_RANK_LENGTH))
        return 0;

    /* Header size must be correct and there must be a body too. */
    len = strlen(header);
    if (len < MIN_HEADER_LENGTH || len > MAX_HEADER_LENGTH || !strlen(body))
	return 0;

    len = sizeof(explode(body, "\n"));
    times = explode(ctime(time(), 1), " ");
    head = sprintf(HEADER_FORMAT, header, rank, author,
                   len > 999 ? "***" : sprintf("%3d", len),
                   sprintf("%2s %-4s", times[1], times[2]));

    post_note(head, body);

    tell_roombb(environment(), "Na " + short(PL_MIE)
              + " spotrzegasz notki ktorych wczesniej nie widzial"
              + koncowka("es", "as", "es") + ".\n", ({}), this_object());
    return 1;
}

/*
 * Function name: read_msg
 * Description  : Read a message on the board
 * Arguments    : string what_msg - the message number.
 *                int    mr       - read with more if true.
 * Returns      : int 1/0 - success/failure.
 */
public nomask varargs int
read_msg(string what_msg, int mr)
{
    int note;
    string in;

    switch (what_msg)
    {
        case "poprzednia":
        case "poprzednia notke":
            note = this_player()->query_prop(PLAYER_I_LAST_NOTE) - 1;
            break;

        case "biezaca":
        case "biezaca notke":
        case "obecna":
        case "obecna notke":
            note = this_player()->query_prop(PLAYER_I_LAST_NOTE);
            break;

        case "nastepna":
        case "nastepna notke":
            note = this_player()->query_prop(PLAYER_I_LAST_NOTE) + 1;
            break;

        default:
            if (!what_msg)
            {
                notify_fail("Przegladasz pobieznie tablice, ale poniewaz nie "
                          + "mozesz sie zdecydowac na nic konkretnego, nie "
                          + "znajdujesz niczego ciekawego.\n");
                return 0;
            }

            if (!sscanf(what_msg, "notke %d", note) &&
                !sscanf(what_msg, "%d", note))
            {
                notify_fail("Ktora notke chcesz przeczytac?\n");
                return 0;
            }
    }

    if (msg_num < 1)
    {
        notify_fail("Na " + short(PL_MIE) + " nie ma zadnych notek.\n");
        return 0;
    }

    if (note < 1)
    {
        notify_fail("Chyba nie ma notki o takim numerze...\n");
        return 0;
    }

    if (note > msg_num)
    {
	if (msg_num == 1)
	    in = "jest";
	else if (msg_num % 10 > 1 && msg_num % 10 < 5 &&
		  (msg_num % 100 < 10 || msg_num % 100 > 20))
	    in = "sa";
	else
	    in = "jest ich";
	    
	notify_fail(sprintf("Nie mozesz przeczytac %s notki. " +
	    "Na %s %s tylko %s.\n", LANG_SORD(note, PL_DOP, PL_ZENSKI),
            short(PL_MIE), in, LANG_SNUM(msg_num, PL_MIA, PL_ZENSKI)));
	return 0;
    }

    /* See whether the player can read the note. */
    if (check_reader(note))
    {
    	notify_fail("Nie mozesz przeczytac tej notki.\n");
	return 0;
    }

    this_player()->add_prop(PLAYER_I_LAST_NOTE, note);
    if (present(this_player(), environment()))
    {
        write("Czytasz " + LANG_SORD(note, PL_BIE, PL_ZENSKI) + " notke.\n");
    }

    note--;
    if (!silent &&
	present(this_player(), environment()))
    {
	saybb(QCIMIE(this_player(), PL_MIA) + " czyta notke zatytulowana:\n" +
	    headers[note][0] + "\n");
    }

    seteuid(getuid());

/*
 * Automatyczny more
    if (!mr)
    {
	mr = (query_verb() == "mread");
    }

    if ((!mr) &&
	(atoi(headers[note][0][42..44]) > MAX_NO_MREAD))
    {
	write("Too long note. More automatically invoked.\n");
	mr = 1;
    }
*/
   mr = 1;

    if (mr == 1)
    {
	this_player()->more(headers[note][0] + "\n\n" +
	    read_file(board_name + "/" + headers[note][1], 2));
    }
    else
    {
	write(headers[note][0] + "\n\n");
	cat(board_name + "/" + headers[note][1], 2);
    }

    return 1;
}

/*
 * Function name: remove_msg
 * Description  : Remove a note from the board.
 * Arguments    : string what_msg - the message to remove.
 * Returns      : int 1/0 - success/failure.
 */
public nomask int
remove_msg(string what_msg)
{
    int note;

    if (!stringp(what_msg))
    {
	notify_fail("Ktora notke chcesz usunac?\n");
	return 0;
    }

    if (!sscanf(what_msg, "notke %d", note) &&
	!sscanf(what_msg, "%d", note))
    {
	notify_fail("Ktora notke chcesz usunac?\n");
	return 0;
    }

    if ((note < 1) ||
	(note > msg_num))
    {
	write("Zdaje sie, ze notka nr " + note + " nie istnieje.\n");
	return 1;
    }

    if (check_remove(note))
    {
	write(remove_str);
	return 1;
    }

    note--;
    if (present(this_player(), environment()))
    {
	saybb(QCIMIE(this_player(), PL_MIA) + " usuwa notke:\n" +
	      headers[note][0] + "\n");
    }

    discard_message(headers[note][1]);
    headers = exclude_array(headers, note, note);
    msg_num--;

    if ((note == msg_num) &&
	(!no_report))
    {
	BOARD_CENTRAL->new_note(board_name, note > 0 ? headers[note-1][1] : 0,
	    MASTER_OB(environment()));
    }

    write("Ok.\n");
    return 1;
}

/*
 * Function name: rename_msg
 * Description  : With this command, the wizard who wrote a note on his
 *                'own' board can rename the note to another players name.
 * Arguments    : string str - the command line argument.
 * Returns      : int 1/0 - success/failure.
 */
public nomask int
rename_msg(string str)
{
    string name;
    string lower;
    string rank;
    string note;
    string old_author;
    string renamer;
    int    num;
    int    index = -1;
    int    len;

    if (no_special_fellow())
    {
        notify_fail("Nie masz uprawnien do tego rodzaju operacji na tej "
                  + "tablicy.\n");
	return 0;
    }

    if (!str || sscanf(str, "note %d %s", num, name) != 2 &&
	        sscanf(str, "%d %s", num, name) != 2)
    {
        notify_fail("Pod kogo chcesz sie podszyc i w ktorej notce?\n"
                  + "Prawidlowa skladnia to: rename <numer notki> "
                  + "<nowy autor>" + (show_lvl ? " [<tytul autora>]" : "")
                  + "\n");
	return 0;
    }

    if ((num < 1) ||
	(num > msg_num))
    {
	notify_fail("There are only " + msg_num + " notes.\n");
	return 0;
    }

    renamer = this_player()->query_real_name();
    old_author = query_author(num);

    if (renamer != old_author &&
        (SECURITY->query_wiz_rank(renamer) < WIZ_ARCH ||
         SECURITY->exist_player(old_author)))
    {
	write("You are not the author of that note.\n");
	return 1;
    }

    if (show_lvl)
    {
        string *words = explode(name, " ");

        name = capitalize(words[0]);
        rank = capitalize(implode(words[1..], " "));
    }
    else
    {
        name = capitalize(name);
        rank = "";
    }

    len = strlen(lower = lower_case(name));
    if (len < MIN_NAME_LENGTH)
    {
        write("Imie '" + name + "' jest za krotkie.\n");
        return 1;
    }
    if (len > MAX_NAME_LENGTH)
    {
        write("Imie '" + name + "' jest za dlugie.\n");
        return 1;
    }
    if (SECURITY->exist_player(lower))
    {
        write("Nie mozesz podszywac sie pod istniejacego gracza.\n");
        return 1;
    }

    while(++index < len)
	if (!wildmatch("*" + lower[index..index] + "*", LEGAL_CHARS))
	{
            write("Imie '" + name + "' zawiera nielegalne znaki. Dozwolone "
                + "to duze i male litery, apostrof i lacznik.\n");
	    return 1;
	}

    if (show_lvl && (len = strlen(rank)))
    {
        if (len < MIN_RANK_LENGTH)
        {
            write("Tytul '" + rank + "' jest za krotki.\n");
            return 1;
        }
        if (len > MAX_RANK_LENGTH)
        {
            write("Tytul '" + rank + "' jest za dlugi.\n");
            return 1;
        }
    }

    seteuid(getuid());

    num--;
    old_author = implode(explode(headers[num][0][RANK_BEGIN..AUTHOR_END],
                                 " ") - ({""}), " ");
    headers[num][0] = sprintf(RENAME_HEADER_FORMAT,
        headers[num][0][..(RANK_BEGIN - 1)], rank, name,
        headers[num][0][(AUTHOR_END + 1)..]);
    note = headers[num][0] + "\n" +
	read_file(board_name + "/" + headers[num][1], 2);
    rm(board_name + "/" + headers[num][1]);
    write_file(board_name + "/" + headers[num][1], note);

    write("Autorem " + LANG_SORD(num + 1, PL_DOP, PL_ZENSKI)
        + " notki nie jest juz " + old_author + ", lecz " + (strlen(rank) ?
          rank + " " + name : name) + ".\n");
    return 1;
}

/*
 * Function name: store_msg
 * Description  : Store a message to disk.
 * Arguments    : string str - the message to store and the filename.
 * Returns      : int 1/0 - success/failure.
 */
nomask public int
store_msg(string str)
{
    int    note;
    string file;

    if (SECURITY->query_wiz_rank(this_player()->query_real_name()) <
	WIZ_NORMAL)
    {
	notify_fail("Niestety, nie mozesz nagrywac notek.\n");
	return 0;
    }

    if (!stringp(str))
    {
	notify_fail("Store which note?\n");
	return 0;
    }

    if ((sscanf(str, "notke %d %s", note, file) != 2) &&
	(sscanf(str, "%d %s", note, file) != 2))
    {
	notify_fail("Gdzie chcesz nagrac ktora notke?\n");
	return 0;
    }

    if ((note < 1) ||
	(note > msg_num))
    {
	write("Ta notka nie istnieje.\n");
	return 1;
    }

    if (check_reader(note))
    {
	write("Nie masz uprawnien zeby moc nagrac te notke.\n");
	return 1;
    }

    note--;
    str = (headers[note][0] + "\n\n" +
	   read_file((board_name + "/" + headers[note][1]), 2));
    EDITOR_SECURITY->board_write(file, str);

    /* We always return 1 because we don't want other commands to be
     * evaluated.
     */
    return 1;
}

/*
 * Function name: query_headers
 * Description  : Return the headers if this_player() is allowed to see
 *                them.
 * Returns      : mixed - the headers in the following format:
 *                        ({ ({ string header, string filename }) })
 *                        or just ({ }) if no read access.
 */
public nomask mixed
query_headers()
{
    return (check_reader() ? ({ }) : secure_var(headers) );
}
