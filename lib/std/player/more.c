/*
 * /std/player/more.c
 *
 * You can call the function more() in two ways, depending on whether you
 * want the player to read in file or in whether you want him/her to read a
 * text you have stored in a variable. Note that if you want the player to
 * read directly from a file, the player must be able to read that file.
 * This may sound a bit silly, but it means that the player must have read
 * rights to that file. It is therefore advisable to always read the text
 * and pass a single string to the player. It is very unlikely that you will
 * want to make a player read something longer than 50KB in the first place.
 *
 * Reading text from a file:
 *     player->more(string filename, int start);
 *
 *     filename: the filename of the file to read
 *     start   : the first line to read in the file (starting with 1)
 *               WARNING: If start_line == 0, then only the filename
 *               will be printed to the player.
 *
 * Reading text from a variable:
 *     player->more(string text);
 *
 *     text    : the text to read, separated by \n (newlines)
 *               NOTE that text is a single string with all the lines
 *               to read concatenated, separated by \n, example:
 *               line1\nline2\nline3
 *
 * It is possible to let the more function generate a call to the object
 * that initially created us. Therefore, you have to add a THIRD argument
 * to the call to more(). If you read text from a variable, the second
 * argument must be 0. The third argument has the function-pointer to the
 * function you want to have called after more, making the call:
 *     player->more(arg1, arg2, function return_function);
 */

#include <stdproperties.h>
#include <options.h>

#define MORE_PAGESIZE   20
#define MORE_PROMPT	write("-- linia " + lineno			\
			    + (numlines > 1 ? "/" + numlines : "")	\
			    + " -- (uzyj '?' by uzyskac pomoc) -- ")
#define MORE_DONE     if (functionp(ret_func)) { ret_func(); }
#define MORE_INPUT    input_to(&input_to_more(, lines, filename, \
					ret_func, first, lineno))

/*
 * Observe that the variable lineno will contain the number to the last
 * printed line in the terminology of the player. For the player, the first
 * line will be line 1, whereas in the text from memory, the first line is
 * line 0 and in the file the first line is line 1, though internally the
 * file line number starts at 0 too. Confusing neh?
 */

/*
 * Function name: input_to_more
 * Description  : After the some part of the text has been printed, the
 *                next command of the player is fed into this function.
 *                It controls what the player wants to read next.
 * Arguments    : string answer     - the command by the player.
 *                string *lines     - the lines of the text (if in text).
 *                string filename   - the file to print (if any).
 *                function ret_func - the function to return to (if any).
 *                int first         - the first line to print.
 *                int lineno        - the last line printed.
 */
static nomask void
input_to_more(string answer, string *lines, string filename,
	      function ret_func, int first, int lineno)
{
    int    index;
    int    pagesize;
    string error;
    int    numlines = sizeof(lines);

    pagesize = (((pagesize = query_option(OPT_MORE_LEN)) < 5) ?
    	MORE_PAGESIZE : pagesize);
    answer = lower_case(answer);

    if (stringp(answer) &&
	(sscanf(answer, "%d", index) == 1))
    {
	lineno = index + pagesize - 1;
	answer = "Goto";
    }

    switch (answer)
    {
    case "":
    case "n":
	lineno += pagesize;
	break;

    /* Since we use the lower case of answer, this can never be triggered
     * by the command of a player, so we can use it as a flag to check on
     * numerical input.
     */
    case "Goto":
	break;

    case "q":
    case "quit":
    case "x":
    case "exit":
	MORE_DONE;
	return;

    case "?":
    case "h":
    case "help":
	write(
"\nDostepne komendy:\n\n" +
"   <enter>      nacisniecie Enter wyswietla kolejna strone\n" +
"   t            powrot do pierwszej strony\n" +
"   b lub p      powrot do poprzedniej strony\n" +
"   n            kolejna strona (jak przy nacisnieciu Enter)\n" +
"   r            ponowne wyswietlenie obecnej strony\n" +
"   <numer>      przejscie do linii <numer>\n" +
"   a            wyswietlenie calej reszty tekstu, o ile nie jest zbyt dluga\n" +
"   h lub ?      niniejsza pomoc\n" +
"   !<komenda>   wykonanie <komendy> bez przerywania przegladania\n" +
"   q lub x      zaprzestanie przegladania tekstu\n\n");
	MORE_PROMPT;
	MORE_INPUT;
	return;

    case "t":
    case "top":
	lineno = first;
	break;

    case "r":
    case "redisplay":
	break;

    case "b":
    case "back":
    case "p":
    case "previous":
	lineno -= pagesize;
	break;

    case "a":
    case "all":
	if (stringp(filename))
	{
	    if (strlen(error = catch(cat(filename, lineno + 1))))
	    {
		write("Cannot print remainder: " + error);
		MORE_PROMPT;
	    	MORE_INPUT;
	    	return;
	    }
	    write("EOF\n");
	}
	else
	{
	    if (strlen(error = catch(write(implode(lines[lineno..], "\n")))))
	    {
	    	write("Cannot print remainder: " + error);
	    	MORE_PROMPT;
	    	MORE_INPUT;
	    	return;
	    }
	}
	MORE_DONE;
	return;

    default:
	MORE_PROMPT;
	MORE_INPUT;
	return;
    }	

    if (lineno < (pagesize + first))
    {
	lineno = pagesize + first;
    }

    if (stringp(filename))
    {
	/* Cat will return the number of lines that are actually read.
	 * If you read beyond the EOF, the reading will stop. This means
	 * that if the last chunk is exactly one page, you still have to
	 * read the EOF sign.
	 */
	if (cat(filename, (lineno - pagesize + 1), pagesize) < pagesize)
	{
	    write("EOF\n");
	    MORE_DONE;
	    return;
	}
    }
    else
    {
	for (index = (lineno - pagesize);
	    ((index < lineno) && (index < numlines)); index++)
	{
	    write(lines[index] + "\n");
	}

	if (lineno >= numlines)
	{
	    MORE_DONE;
	    return;
	}
    }

    MORE_PROMPT;
    MORE_INPUT;
    return;
}

/*
 * Function name: more
 * Description  : Call this function in a player like explained in the
 *                header of this file in order to let him/her read a text
 *                with more.
 * Arguments    : string arg    - either the filename of the text to read or
 *                                the text to read itself, concatenated with
 *                                newlines (\n).
 *                int start     - the first line to read, if equal to 0 zero
 *                                it means that <arg> contains the text to
 *                                print.
 *                function func - the function to call when the player is
 *                                done reading the text.
 * Returns      : int 1         - always, not really necessary, but nice for
 *                                confirmation of the function call.
 */
public nomask varargs int
more(string arg, int start, function func)
{
    string *lines;
    string filename;

    if (start)
    {
    	filename = arg;
	lines = 0;
	start--;
    }
    else
    {
    	filename = 0;
	lines = explode(arg, "\n");
    }

    input_to_more("", lines, filename, func, start, start);
    return 1;
}
