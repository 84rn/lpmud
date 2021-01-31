/*
 * /std/book.c
 *
 * A general book with many pages. You have to open the book and turn it to
 * the right page in order to read it.
 *
 */
#pragma strict_types

inherit "/std/scroll";

#include <stdproperties.h>
#include <macros.h>
#include <language.h>
#include <cmdparse.h>
#include <pl.h>

/*
 * Prototype
 */
varargs void	read_book_at_page(int page, string verb);
int read_scroll(string str);

int 	book_is_closed;
int 	what_page;
int 	maxm_page;
string 	gPage;

/*
 * Function name: create_book
 * Description:   creates a default book
 *                change it to make your own book
 */
public void
create_book()
{
    set_long("Pusta ksiazka o bialych kartach.\n");
}

/*
 * Function name: create_scroll
 * Description:   creates the general object
 * Arguments:	  
 */
nomask void
create_scroll() 
{
    ustaw_nazwe( ({"ksiazka", "ksiazki", "ksiazce", "ksiazke", "ksiazka",
        "ksiazce" }), ({ "ksiazki", "ksiazek", "ksiazkom", "ksiazki",
        "ksiazkami","ksiazkach" }), PL_ZENSKI);

    book_is_closed = 1;
    what_page = 1;
    add_prop(OBJ_I_WEIGHT, 700);
    add_prop(OBJ_I_VOLUME, 400);
    add_prop(OBJ_I_VALUE, 200);
    create_book();
}

/*
 * Function name: init
 * Description:   initialise the commands 
 * Arguments:	  
 */
void
init()
{
    ::init();

    add_action(read_scroll, "zamknij");
    add_action(read_scroll, "otworz");
    add_action(read_scroll, "przewroc");
}

/*
 * Function name: open_me
 * Description:   opens the book (at page one!)
 */
void
open_me()
{
  int appr_num;

  if (gPage == "")
  {  
      if (!book_is_closed)
      {
          write(capitalize(short(this_player(), PL_MIA)) + " nie jest " +
              "zamknie" + koncowka("ty","ta","te") + ".\n");
	  return;                               
      }

      what_page = 1;
  }
  else
  {
/* jak juz bedzie LANG_ORDS, to bedzie:
    appr_num = LANG_ORDS(gPage);

poki co:  */
    appr_num = atoi(gPage);
    if (appr_num > 0 && appr_num < maxm_page + 1)
    {
        what_page = appr_num;
    }
    else
    {
        write("Na ktorej stronie chcesz otworzyc " + 
            short(this_player(), PL_BIE)+ "?\n");
        return;
    }

  }

  write("Otwierasz " + short(this_player(),PL_BIE) + " na stronie " +
      LANG_SORD(what_page, PL_MIE, PL_ZENSKI) + ".\n");
      
  say(QCIMIE(this_player(), PL_MIA) + " otwiera " + 
      QSHORT(this_object(), PL_BIE) + ".\n");

  book_is_closed = 0;
}

/*
 * Function name: close_me
 * Description:   closes the book again
 */
void
close_me()
{
    if (book_is_closed)
    {
        write(capitalize(short(this_player(), PL_MIA)) + " jest juz zamknie" +
            koncowka("ty","ta","te") + ".\n");
        return;
    }

    write("Zamykasz " + short(this_player(), PL_BIE) + ".\n");
    say(QCIMIE(this_player(), PL_MIA) + " zamyka " + 
        QSHORT(this_object(), PL_BIE) + ".\n");

    what_page = 1;
    book_is_closed = 1;
}

/*
 * Function name: turn_me
 * Description:   turn the book to the next page
 */
void
turn_me()
{
    int appr_num;

    gPage = previous_object()->query_gPage();

    if (book_is_closed) 
    {
	write(capitalize(short(this_player(),PL_MIA)) + " jest zamknie"+
	    koncowka("ty", "ta", "te") + ".\n");
	return;
    }
    
    if (gPage == "dalej" || gPage == "wprzod" || gPage == "nastepna" || 
        gPage == "naprzod" || gPage == "") 
    {
        if (maxm_page < what_page + 1) 
        {
            write("To juz ostatnia strona " +
			short(this_player(), PL_DOP) + ".\n");    
            return;
	}
        what_page += 1;
        if (maxm_page == what_page)
            write("Otwierasz " + short(this_player(), PL_BIE) +
			" na ostatniej stronie.\n");
    }
    else if (gPage == "wstecz" || gPage == "poprzednia")
    {
        if (what_page == 1)
    	{
            write("To juz jest pierwsza strona t" + koncowka("ego ","ej ") + 
                short(this_player(), PL_DOP) +".\n");
            return;
        }
        what_page -= 1;
    }
    else if (gPage != "/konkretna/")
    {
	write("Chcesz przewrocic strone 'naprzod' czy 'wstecz'?\n");
	return;
    }

    write("Otwierasz " + short(this_player(), PL_BIE) +
          " na stronie " + LANG_SORD(what_page, PL_MIE, PL_ZENSKI) + 
          ".\n");
          
    return ;
}

/*
 * Function name: set_max_page
 * Description:   sets the number of pages of the book
 * Arguments:	  how_many - how many pages ?
 */
void
set_max_pages(int how_many) 
{ 
    maxm_page = how_many; 
}

int query_max_pages()
{
    return maxm_page;
}

/*
 * Function name: read_scroll
 * Description:   We need some special stuff for the turn page command
 *		  The turn page has the following syntax
 *		    turn page - turn forward one page
 *		    turn page forward/[backward, back] - turn one page
 *			in the appropriate direction
 *		    turn book to/at page <num> - turn to page <num>
 *			where <num> is a string like one, two, eight,
 *			not an integer like 7.
 */
static int
read_scroll(string str)
{
    string where, what;
    int strona;

    if (!str)
	return ::read_scroll(what);

    if (str == "strone" || str == "kartke")
    {
	gPage = "naprzod";
	what = "ksiazke";
    }
    else if (query_verb() == "przewroc")
    {
        if (parse_command(str, ({}), "'strone' / 'kartke' %w", where))
        {            
	    gPage = where;
	    what = "ksiazke";
        }
        else if ((parse_command(str, ({}), "'na' [strone] %d", strona)) ||
                 (parse_command(str, ({}), "'strone' 'na' %d", strona)))
        {
            what = "ksiazke";
            what_page = strona;
            gPage = "/konkretna/";
        }
        else
        {
	    gPage = "";
	    what = str;
        }
    }
    else if ((query_verb() == "otworz") &&
             (parse_command(str, ({}), "%s 'na' [stronie] %w", what, where)))
    {
        gPage = where;
    }
    else
    {
	gPage = "";
	what = str;
    }

    return ::read_scroll(what);
}

/*
 * Function name: read_it
 * Description:   If player wanted to do anything to this book we end up here.
 * Arguments:	  verb - The verb the player had used
 */
void
read_it(string verb)
{
    switch (verb)
    {
    case "przeczytaj":
	if (book_is_closed)
	{
	    write(capitalize(short(this_player(), PL_MIA)) + " jest zamknie" +
                koncowka("ty", "ta", "te") + ".\n");
	    return;
	}
	say(QCIMIE(this_player(), PL_MIA) + " czyta " + 
	    QSHORT(this_object(), PL_BIE) + ".\n");
	read_book_at_page(what_page, verb); 
	break;
    case "przewroc":
	turn_me(); break;
    case "otworz":
	open_me(); break;
    case "zamknij":
	close_me(); break;
    }
}

/*
 * Function name: read_book_at_page
 * Description:   should be redefined in your book. is called from read_me
 * Arguments:	  which - read the book at which page
 *		  verb  - If the player wanted to read it, or mread it.
 *			  To mread something, you can look how the scoll.c does
 *			  it in read_it() if needed.
 */
varargs void
read_book_at_page(int which, string verb)
{ 
    write("Ta ksiazka jest zupelnie pusta, pelna bialych kart.\n");
}

void
leave_env(object from, object to)
{
    book_is_closed = 1;
    ::leave_env(from, to);
}

/*
 * Function name: query_gPage
 * Description:   Ask what page info the player gave.
 * Returns:	  The same string the player gave
 */
string
query_gPage()
{
    return gPage;
}
