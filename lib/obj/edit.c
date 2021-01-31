/*
 * /obj/edit.c
 *
 * This object is a basic line editor. It supports some simple functions
 * that allow you to manipulate text you are writing. If you want to allow
 * a player to edit a text, clone this object and make the following call:
 *
 * clone_object(EDITOR_OBJECT)->edit(mixed ffun, string text, int begin);
 *
 * All parameters are optional and have the following meaning:
 *
 * mixed ffun  - the name of the function called in the calling object when
 *               done editing. If omitted or 0, the function done_editing()
 *               is called. This can be a string or a functionpointer.
 * string text - if this parameter is added, this text will be the default
 *               text the player can edit and append to.
 * int begin   - start editing at this line number. Start counting at line 1.
 *
 * When the player is done editing, the function done_editing(string text)
 * is called in the object that called us. If you want another function to
 * be called, use the first parameter to set something different.
 *
 * NOTE: edit() operates on this_player(). If this_player() is not valid
 *       when edit() is called, nothing can be edited.
 */

#pragma no_inherit
#pragma save_binary
#pragma strict_types

inherit "/std/object";

#include <composite.h>
#include <files.h>
#include <macros.h>
#include <stdproperties.h>

#define RETURN_FUNCTION "done_editing"
#define EDITOR_ID       "_editor_"
#define EDIT_END        "**"

/* Prototype.
 */
static void input(string str);

/* Global variables. They are secure since we do not want people snooping.
 * The variable 'line' keeps the number of the next line in terms of code.
 * The player will type the first line (1) and the game has 'line' at 0.
 */
static private object  calling_ob;   /* the object that called us.   */
static private mixed   finished_fun; /* function to call when done.  */
static private string *lines;        /* the lines of the text.       */
static private int     line;         /* the number of the next line. */

/*
 * Function name: create_object
 * Description  : Called to create the object.
 */
public void
create_object()
{
    ustaw_nazwe( ({ "edytor", "edytora", "edytorowi", "edytor", "edytorem", 
        "edytorze" }), ({ "edytory", "edytorow", "edytorom", "edytory",
        "edytorami", "edytorach" }), PL_MESKI_NOS_NZYW);
        
    add_name(EDITOR_ID);

    dodaj_przym("liniowy", "liniowi");
    
    ustaw_nazwe( ({ "edytor liniowy", "edytora liniowego", "edytorowi " +
        "liniowemu", "edytor liniowy", "edytorem liniowym", 
        "edytorze liniowym" }), ({ "edytory liniowe", "edytorow liniowych", 
        "edytorom liniowym", "edytory liniowe", "edytorami liniowymi", 
        "edytorach liniowych" }), PL_MESKI_NOS_NZYW);

    ustaw_shorty( ({ "edytor liniowy", "edytora liniowego", "edytorowi " +
        "liniowemu", "edytor liniowy", "edytorem liniowym", 
        "edytorze liniowym" }), ({ "edytory liniowe", "edytorow liniowych", 
        "edytorom liniowym", "edytory liniowe", "edytorami liniowymi", 
        "edytorach liniowych" }), PL_MESKI_NOS_NZYW);

    set_long("It is a basic line editor. You should never see this. In " +
	"order to get information on it, examine the sourcecode.\n");

    remove_prop(OBJ_I_VALUE);
    remove_prop(OBJ_I_VOLUME);
    remove_prop(OBJ_I_WEIGHT);
    add_prop(OBJ_M_NO_DROP,     1);
    add_prop(OBJ_M_NO_GET,      1);
    add_prop(OBJ_M_NO_GIVE,     1);
    add_prop(OBJ_M_NO_STEAL,    1);
    add_prop(OBJ_M_NO_TELEPORT, 1);

    set_no_show();

    setuid();
    seteuid(getuid());
}

/*
 * Function name: display
 * Description  : Shows the previous lines before the current line.
 * Arguments    : int num - the number of lines to show.
 *                          -1 - show the whole text with line numbers.
 *                          -2 - show the whole text without line numbers.
 */
static void
display(int num)
{
    int index;
    int start;
    int end;
    int size;

    /* Not possible to list negative line numbers. 0 Lines is nonsense too. */
    if ((num == 0) ||
	(num < -2) ||
	(!(size = sizeof(lines))))
    {
	return;
    }

    /* Argument -2 is a signal to print the whole text without line
     * numbers. We have to be careful with too long strings, so we
     * split it in printable chunks.
     */
    if (num == -2)
    {
	index = 0;
	while(size > (index * 10))
	{
	    write(implode(lines[(index * 10)..((index * 10) + 9)], "\n") +
		"\n");
	    index++;
	}
	return;
    }

    /* Argument -1 is a signal to print the whole text. Else, set the
     * number of lines to display.
     */
    if (num == -1)
    {
	start = 0;
	end = size - 1;
    }
    else
    {
	end = (line - 1);
	start = ((num > end) ? 0 : (end - num + 1));
    }

    for (index = start; index <= end; index++)
    {
	write(sprintf("%2d]%s\n", (index + 1), lines[index]));
    }
}

/*
 * Function name: finished
 * Description  : Called when the user is finished editing a text. It will call
 *                the report function in the calling object.
 * Arguments    : string str - the text entered.
 */
static void 
finished(string str)
{
    if (functionp(finished_fun))
    {
        function f = finished_fun;
        f(str);
    }
    else
    {
        call_other(calling_ob, finished_fun, str);
    }
}

/*
 * Function name: edit
 * Description  : Lets the player edit a text. Both paramters described
 *                below are optional. Note that the function operates on
 *                this_player.
 * Argmuents    : string ffun - the name of the function to call in the
 *                              object that called us when the players is
 *                              done editing. If not, a default function,
 *                              defined in RETURN_FUNCTION is used.
 *                string str  - the text the players wants to edit.
 *                int begin   - start editing at line x. (start counting
 *                              at line 1).
 */
public varargs void
edit(mixed ffun, string str = "", int begin = 0)
{
    calling_ob = previous_object();

    if (functionp(ffun))
    {
        finished_fun = ffun;
    }
    else if (!stringp(ffun) || !strlen(ffun))
    {
        finished_fun = RETURN_FUNCTION;
    }
    else
    {
        finished_fun = ffun;
    }

    /* If this_player does not exist, nothing can be edited. */
    if (!objectp(this_player()))
    {
        finished(str);
        remove_object();
        return;
    }

    /* We move the editor into the player to save on linkdeath. */
    move(this_player(), 1);

    lines = (strlen(str) ? explode(str, "\n") : ({ }) );
    line  = sizeof(lines);

    if (begin &&
	(begin < line))
    {
	line = begin - 1;
    }

/*
    write("Extended editor! Type ~? for help and " + EDIT_END +
	" to finish editing.\n");
*/
    write("Wpisz ~?, zeby uzyskac pomoc, lub " + EDIT_END + 
        ", by zakonczyc edycje.\n");
    display(10);

    write(sprintf("%2d]", (line + 1)));
    input_to(input);
}

/*
 * Function name: add_cc
 * Description  : When writing mail, add people to the CC-list.
 * Arguments    : string arg - the command line argument.
 */
static void
add_cc(string arg)
{
    string *names;
    string *error;

    if (function_exists("create_object", calling_ob) != MAIL_READER)
    {
        write("Nie mozesz nikogo dodac do listy CC, gdyz nie piszesz " +
	    "zadnego listu.\n");
        return;
    }

    if (!strlen(arg))
    {
        write("Nie dodano zadnych imion do listy CC.\n");
        return;
    }

    names = explode(calling_ob->cleanup_string(arg), " ");
    if (sizeof(error = calling_ob->check_mailing_list(names)))
    {
	write("Nie ma takiego adresata (gracza, domeny, aliasa): " + 
		COMPOSITE_WORDS(error) + ".\n");
        return;
    }

    write((sizeof(names) > 1 ? "Imiona" : "Imie") + " dodane do listy CC.\n");
    calling_ob->editor_add_to_cc_list(names);
}

/*
 * Function name: delete
 * Description  : Player wants to delete one or more lines. The format
 *                is '3' or '3-5' or '3,5' or mixed: '3,5-7'.
 * Arguments    : string arg - the command line argument.
 */
static void
delete(string arg)
{
    int    *range = ({ });
    string *parts;
    int     left;
    int     right;
    int     index;
    int     index2;

    if (!strlen(arg))
    {
        write("Nie podano numeru lini do skasowania.\n");
        return;
    }

    arg   = implode(explode(arg, " "), "");
    parts = explode(arg, ",");

    for (index = 0; index < sizeof(parts); index++)
    {
	if (sscanf(parts[index], "%d-%d", left, right) == 2)
        {
	    if ((left < 1) || (left > sizeof(lines)) ||
		(right < 1) || (right > sizeof(lines)))
	    {
		range = 0;
		break;
	    }

	    if (right < left)
	    {
		index2 = left;
		left   = right;
		right  = index2;
	    }

	    for (index2 = left; index2 <= right; index2++)
	    {
		range -= ({ index2 });
		range += ({ index2 });
	    }
        }
	else
	{
	    index2 = atoi(parts[index]);

	    if ((index2 < 1) || (index2 > sizeof(lines)))
	    {
		range = 0;
		break;
	    }

	    range -= ({ index2 });
	    range += ({ index2 });
	}
    }

    if ((!pointerp(range)) ||
	(!sizeof(range)))
    {
	write("Blad skladniowy przy kasowaniu. Nie usunieto zadnych " +
	    "linijek.\n");
	return;
    }

    range = sort_array(range);
    for(index = (sizeof(range) - 1); index >= 0; index--)
    {
	if (range[index] <= line)
	{
	    line--;
	}
	lines[range[index] - 1] = 0;
    }

    lines -= ({ 0 });
}

/*
 * Function name: restore
 * Description  : When your link dies and you are editing something, the
 *                edit buffer may have been saved. The restore command
 *                restores it for you.
 */
#ifdef EDITOR_SAVE_OBJECT
static void
restore()
{
    string message;

    setuid();
    seteuid(getuid());

    /* The function restore() in the EDITOR_SAVE_OBJECT operates on
     * this_player().
     */
    message = EDITOR_SAVE_OBJECT->restore();

    if (!strlen(message))
    {
	write("Przykro mi, ale nie przechowala sie zadna twoja notka.\n");
	return;
    }

    lines = explode(message, "\n");
    line  = sizeof(lines);

    display(10);
}
#endif EDITOR_SAVE_OBJECT

/*
 * Function name: replace
 * Description  : When you want to replace a single line with new text,
 *                it is easier to use this command than to delete the old
 *                line(s), possibly change the point of entry and then
 *                type the new text.
 * Arguments    : string arg - the line that needs to be replaced in the
 *                             format: <num> <text>
 */
static void
replace(string arg)
{
    int num;

    if (!strlen(arg) ||
        (sscanf(arg, "%d %s", num, arg) != 2))
    {
	write("Skladnia: ~c <numer linijki> <nowy tekst>\n");
	return;
    }

    if ((num < 1) ||
	(num > sizeof(lines)))
    {
	write("W tekscie nie ma linii " + num + ".\n");
	return;
    }

    lines[num - 1] = arg;
    display(10);
}

/*
 * Function name: import
 * Description  : "Normal" wizards may ftp a file into Genesis and then
 *                import it into the editor.
 * Arguments    : string arg - the filename of the file to import.
 * Returns      : int        - if trye, the player added an EDIT_END
 *                             in his file, so we finish editing.
 */
static int
import(string arg)
{
    string *import_lines;
    string  import_text = EDITOR_SECURITY->import(arg);

    /* If this happens, an error message is printed by the EDITOR_SECURITY. */
    if (!strlen(import_text))
    {
	return 0;
    }

    import_lines = explode(import_text, "\n");

    /* Append line at the end of the text, or if there is no text yet,
     * just append it to the empty array.
     */
    if (line == sizeof(lines))
    {
	lines += import_lines;
    }
    /* Insert before the first line. */
    else if (line == 0)
    {
	lines = import_lines + lines;
    }
    /* Insert somewhere between lines. */
    else
    {
	lines = lines[..(line - 1)] + import_lines + lines[line..];
    }

    /* If the player included the EDIT_END in the imported file, the
     * editor will finish editing.
     */
    if (member_array(EDIT_END, import_lines) != -1)
    {
	lines -= ({ EDIT_END });

	if (objectp(calling_ob))
	{
	    arg = (sizeof(lines) ? (implode(lines, "\n") + "\n") : "");
            finished(arg);
	}
	else
	{
	    write("\nBLAD EDYTORA. Brak wywolujacego obiektu. " +
	        "Edycja przerwana.\n");
	}

	remove_object();
	return 1;
    }

    /* Adjust the number of the next line to be added and display the
     * last part of the message imported.
     */
    line += sizeof(import_lines);
    display(10);

    return 0;
}

/*
 * Function name: checkrange
 * Description  : Checks whether the current line is valid after the
 *                player used ~i or ~a. If not, set it to something valid.
 */
static void
checkrange()
{
    if (line < 0)
    {
	write("Niewlasciwy numer linii. Zamieniam na pierwsza linie.\n");
	line = 0;
    }

    if (line > sizeof(lines))
    {
	line = sizeof(lines);
	write("Niewlasciwy numer linii. Zmieniam na linie " + (line + 1) + 
	    ".\n");
    }
}

/*
 * Function name: autowrap
 * Description  : People who have used 80 characters or more in their text
 *                are prompted to have their message autowrapped for optimal
 *                readability
 * Arguments    : string str - the input.
 */
static void
autowrap(string str)
{
    str = (strlen(str) ? lower_case(str) : "t");
    switch(str[0])
    {
    case '~':
        if (str == "~q")
        {
            finished("");
            remove_object();
            return;
        }

    case 'n':
        write("Tekst nie zostanie sformatowany.\n");
        break;

    case 't':
        write("Tekst zostanie odpowiednio sformatowany.\n");
        lines = map(lines, &break_string(, 79));
        break;

    default:
        write("Odpowiedz t[ak], n[ie] lub ~q. Sformatowac? [tak] ");
        input_to(autowrap);
        return;
    }

    str = implode(lines, "\n") + "\n";
    finished(str);
    remove_object();
}

/*
 * Function name: input
[ * Description  : Called with input from the player.
 * Arguments    : string str - the text typed by the player.
 */
static void
input(string str)
{
    string arg;
    int    cmd;

    /* Player finished editing. Lets wrap it up and return operation
     * to the calling object.
     */
    if (str == EDIT_END)
    {
        if (!objectp(calling_ob))
        {
	    write("\nBLAD EDYTORA. Brak wywolujacego obiektu. " +
	        "Edycja przerwana.\n");
	    remove_object();
	    return;
	}

        /* If people type too long lines, ask them to auto-wrap. */
        if (sizeof(filter(lines, &operator(>=)(, 80) @ strlen)))
        {
	    write("We wpisanym tekscie znajduja sie linie o dlugosci " +
		"przekraczajacej 80 znakow. Mogloby one uczynic go " +
		"trudnym w odbiorze. Czy chcesz, aby edytor sformatowal " +
		"dla ciebie calosc do nieco bardziej czytelnej postaci? " +
		"[domyslnie: tak] ");
            input_to(autowrap);
            return;
        }

        str = (sizeof(lines) ? (implode(lines, "\n") + "\n") : "");
        finished(str);
        remove_object();
        return;
    }

    /* Player may start a line with ~~ to indicate that he wants to start
     * the line with a tilde '~'.
     */
    cmd = wildmatch("~*", str);
    if (cmd &&
	wildmatch("~~*", str))
    {
	cmd = 0;
	str = extract(str, 1);
    }

    /* Player entered yet another line, ie. not a special command. Lets
     * add it to the text.
     */
    if (!cmd)
    {
	/* Append line at the end of the text, or if there is no text yet,
	 * just append it to the empty array.
	 */
	if (line == sizeof(lines))
	{
	    lines += ({ str });
	}
	/* Insert before the first line. */
	else if (line == 0)
	{
	    lines = ({ str }) + lines;
	}
	/* Insert somewhere between lines. */
	else
	{
	    lines = lines[..(line - 1)] + ({ str }) + lines[line..];
	}

	line++;
	write(sprintf("%2d]", (line + 1)));
	input_to(input);
	return;
    }

    /* Player gave a special command. Lets extract the argument. */
    sscanf(str, "%s %s", str, arg);
    switch(str)
    {
    /* Append after line <arg>. Default: append after end of text. */
    case "~a":
        if (strlen(arg))
        {
            line = atoi(arg);
            checkrange();
        }
        else
        {
            line = sizeof(lines);
        }
        display(10);
        break;

    /* Replace one line with new text. */
    case "~c":
        replace(arg);
        break;

    /* Add people to your CC list when sending mail. */
    case "~cc":
        add_cc(arg);
        break;

    /* Delete line(s) <arg>. */
    case "~d":
        delete(arg);
        break;

    /* Player wants help on the commands.
     */
    case "~h":
    case "~?":
	write(
"Komendy edytora:\n\n" +
"<tekst>    Kolejna linijka tekstu.\n" +
EDIT_END + "         Koniec tekstu.\n" +
"~? or ~h   Informacja o komendach edytora (ktora wlasnie czytasz).\n" +
"~q         Anulowanie tekstu i przerwanie edycji.\n" +
"~l (~L)    Wyswietlenie calosci tekstu z(bez) numerami linii.\n" +
"~s [<num>] Wyswietlenie ostatnich <num> linii. Domyslnie: 10.\n" +
"~a [<num>] Pisanie na koncu tekstu. [Po linii <num>].\n" +
"~i [<num>] Wstawianie na poczatku tekstu. [Przed linia <num>].\n" +
"~c <num> <tekst>  Zastapienie linii <num> <tekstem>.\n" +
"~d <przedzial> Kasowanie w <przedziale>. Format: '3', '3-5', '3,5'\n" +
"           lub mieszany '3,5-6'.\n" +
    (this_player()->query_wiz_level() ?
"~m <plik>  Wpuszczenie pliku <plik> do edytora.\n" : "") +
#ifdef EDITOR_SAVE_OBJECT
"~r         Odzyskanie tekstu, ktory byl edytowany, gdy padlo polaczenie.\n" +
#endif EDITOR_SAVE_OBJECT
"~cc <imie> Dodaje podane osoby do listy CC, podczas pisania listu.\n" +
"!<komenda> Wykonanie <komendy> poza edytorem i powrot do edytora.\n");
	break;

    /* Insert in front of line <arg>. Default: insert before text. */
    case "~i":
        if (strlen(arg))
        {
            line = atoi(arg);
            line--;
            checkrange();
        }
        else
        {
            line = 0;
        }
        display(10);
        break;

    /* Show the complete text with line numbers. */
    case "~l":
        display(-1);
        break;

    /* Show the complete text without line numbers. */
    case "~L":
        display(-2);
        break;

    /* Import a file into the editor. (Wizards only.) */
    case "~m":
        if (import(arg))
        {
            return;
        }
        break;

    /* Cancel editing. No text is returned. */
    case "~q":
        if (objectp(calling_ob))
        {
            finished("");
        }
        else
        {
	    write("\nBLAD EDYTORA. Brak wywolujacego obiektu. " +
	        "Edycja przerwana.\n");
	}

	remove_object();
	return;
	break;

#ifdef EDITOR_SAVE_OBJECT
     /* Restore a message you were editing while you went linkdead. */
     case "~r":
        restore();
        break;
#endif EDITOR_SAVE_OBJECT

    /* Show <arg> lines. Default: 10 lines. */
    case "~s":
        display(strlen(arg) ? atoi(arg) : 10);
        break;

    /* We do not recognize the command. Give an error message.
     */
    default:
	write("Nieznana komenda. Wpisz ~?, zeby uzyskac pomoc, lub " + 
	    EDIT_END + ", by zakonczyc edycje.\n");
	break;
    }

    write(sprintf("%2d]", (line + 1)));
    input_to(input);
}

/*
 * Function name: linkdie
 * Description  : When the player holding this editor linkdies, the
 *                edit-buffer is saved by the EDITOR_SAVE_OBJECT.
 *                When (s)he re-links, the buffer can be restored and
 *                the message continued.
 */
#ifdef EDITOR_SAVE_OBJECT
public void
linkdie()
{
#ifdef OWN_STATUE
    if (!CALL_BY(OWN_STATUE))
    {
	return;
    }
#endif OWN_STATUE

    if (objectp(calling_ob))
    {
	call_other(calling_ob, finished_fun, "");
    }

    if (!sizeof(lines))
    {
	remove_object();
	return;
    }

    setuid();
    seteuid(getuid());

    EDITOR_SAVE_OBJECT->linkdie(implode(lines, "\n"));
    remove_object();
}
#endif EDITOR_SAVE_OBJECT
