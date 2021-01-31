/*
 * /cmd/live/things.c
 *
 * General commands for manipulating things.
 * X oznacza komende wycofana
 *
 * - appraise	X
 * - count	X
 * - drop	X
 * - examine	X
 * - exa	X
 * - get	X
 * - give	X
 * - hide	X
 * - i		- zamienione na inwentarz
 * - inventory	X
 * - keep	X
 * - l		X
 * - look	X
 * - peek	X
 * - pick	X
 * - put	X
 * - reveal	X
 * - search	X
 * - sneak	X
 * - take	X
 * - track	X
 * - unkeep	X
 */

#pragma no_clone
#pragma no_inherit
#pragma save_binary
#pragma strict_types

inherit "/cmd/std/command_driver";

#include <composite.h>
#include <cmdparse.h>
#include <filter_funs.h>
#include <formulas.h>
#include <language.h>
#include <macros.h>
#include <ss_types.h>
#include <std.h>
#include <stdproperties.h>
#include <subloc.h>

#define ENV (environment(this_player()))
#define PREV_LIGHT CAN_SEE_IN_ROOM(this_player()) 

/*
 * Prototypes
 */
int visibly_hold(object ob);
int manip_set_dest(string prep, object *carr);
varargs int visible(object ob, object cobj);

/*
 * Global variables
 */
static int silent;	   /* silent flag if person did 'get/drop all' */
static object gDest;	     /* destination use for put and give */
static object *gContainers;  /* array of containers to try */
static object *gFrom;	     /* array of objects where player did get things */
static object gHolder;
static string gItem;	     /* string to hold pseudoitem from look command */
static string gBezokol = ""; /* bezokolicznik */

static mapping sneak_dirs = ([
				"n"  : "polnoc",
				"ne" : "polnocny-wschod",
				"e"  : "wschod",
				"se" : "poludniowy-wschod",
				"s"  : "poludnie",
				"sw" : "poludniowy-zachod",
				"w"  : "zachod",
				"nw" : "polnocny-zachod",
				"u"  : "gore",
				"d"  : "dol",
			     ]);


void
create()
{
    seteuid(getuid(this_object())); 
}

/* **************************************************************************
 * Return a proper name of the soul in order to get a nice printout.
 */
string
get_soul_id()
{
    return "things";
}

/* **************************************************************************
 * This is a command soul.
 */
int
query_cmd_soul()
{
    return 1;
}

/* **************************************************************************
 * The list of verbs and functions. Please add new in alfabetical order.
 */
mapping
query_cmdlist()
{
    return ([
	     "appraise":"appraise",

	     "count":"count",
	     "czas":"czas",

	     "daj":"daj",
	     "drop":"drop",

	     "examine":"examine",
	     "exa":"examine",

	     "get":"get",
	     "give":"give",

	     "hide":"hide",

	     "i":"inwentarz",
	     "inwentarz":"inwentarz",

	     "keep":"keep", // poki co po angielsku

	     "l":"look",
	     "look":"look",
	     
	     "obejrzyj":"obejrzyj",
	     "ob":"obejrzyj",
	     "ocen":"ocen",
	     "odbezpiecz":"zabezpiecz",
	     "odloz":"odloz",

	     "peek":"peek",
	     "pick":"get",
	     "podejrzyj":"podejrzyj",
	     "policz":"policz",
	     "poloz":"odloz",
	     "przemknij":"przemknij",
	     "przeszukaj":"przeszukaj",
	     "put":"put",
	     
	     "reveal":"reveal",

	     "schowaj":"ukryj",
	     "search":"search",
	     "sneak":"sneak",
	     "sp":"spojrz",
	     "spojrz":"spojrz",
	     "szukaj":"szukaj",

	     "take":"get",
	     "track":"track",
	     "trop":"trop",

	     "ujawnij":"ujawnij",
	     "ukryj":"ukryj",
	     "unkeep":"keep",
	     
	     "wez":"wez",
	     "wloz":"wloz",
	     
	     "zabezpiecz":"zabezpiecz",
	   ]);
}

/*
 * Function name: using_soul
 * Description:   Called once by the living object using this soul. Adds
 *		  sublocations responsible for extra descriptions of the
 *		  living object.
 */
public void 
using_soul(object live)
{
/*
    live->add_subloc(SUBLOC_MISCEXTRADESC, file_name(this_object()));
    live->add_textgiver(file_name(this_object()));
*/
}

/* **************************************************************************
 * Here follows some support functions. 
 * **************************************************************************/

int
kobieta()
{
    return (this_player()->query_rodzaj() == PL_ZENSKI);
}

/*
 * We fail to do something because it is dark
 */
int
light_fail(string str = "dostrzec cokolwiek")
{
    string prop = environment(this_player())->query_prop(ROOM_S_DARK_MSG);

    notify_fail((prop ? prop : "Jest zbyt ciemno") + ", by moc " + str
	      + ".\n");
    return 0;
}

/*
 * Function name: move_err_short
 * Description:   Translate move error code to message and prints it.
 * Arguments:	  ierr: move error code
 *		  ob:   object
 * Outputs:	  Message string.
 *
 * Error message examples:
	"The bag is too heavy."
	"The bird can not be dropped."
	"The pearl can not be removed."
	"The ankheg refuses."
	"The chest is full."
	"The river can not be taken."
 *
 * Ref: 	  see /std/object.c for the move function 
 */
varargs void 
move_err_short(int ierr, object ob, object dest)
{
    mixed str, str2;
    string shortdesc;
    int rodzaj;

    if (silent || ierr <= 0 || ierr >= 11 || (ierr == 7) || !objectp(ob) ||
	ob->query_no_show())
	return;

    str = "   ";
    rodzaj = ob->query_rodzaj();

    switch (ierr)
    {
    case 1:
	if (ob->query_prop(HEAP_I_IS) && ob->num_heap() > 1)
	    str += capitalize(ob->query_pname(0, PL_MIA)) + " sa zbyt ciez" + 
	    ob->koncowka(0, 0, 0, "cy", "kie") + ".\n";
	else
	    str += capitalize(ob->short(PL_MIA)) + 
		(ob->query_tylko_mn() ? " sa" : " jest") + " zbyt ciez" + 
		ob->koncowka("ki", "ka", "kie", "cy", "kie") + ".\n";
	break;
    case 2:
	str2 = ob->query_prop(OBJ_M_NO_DROP);
	if (!stringp(str2))
	    str += "Nie mozesz porzucic " + ob->short(PL_DOP) + ".\n";
	else
	    str = str2;
	break;
    case 3:
	if (query_verb() == "daj")
	{
	    str2 = ob->query_prop(OBJ_M_NO_GIVE);
	    if (!stringp(str2))
		str += "Nie mozesz oddac " + ob->short(PL_DOP) + ".\n";
	    else
		str = str2;
	    break;
	}
	if (dest)
	    str2 = environment(ob)->query_prop(CONT_M_NO_REM);
	if (!stringp(str2))
	    str += "Nie mozesz wyjac " + ob->short(PL_DOP) + ".\n";
	else
	    str = str2;
	break;
    case 4:
	str2 = ob->query_prop(OBJ_M_NO_INS);
	if (!stringp(str2))
	    str += capitalize(ob->short(PL_MIA)) + " nie chce wejsc.\n";
	else
	    str = str2;
	break;
    case 5:
	if (dest)
	    str2 = dest->query_prop(CONT_M_NO_INS);
	if (!dest->query_prop(CONT_I_IN))
	    str += "Nie mozesz tam wlozyc " + ob->short(PL_DOP) + ".\n";
	else
	    str = str2;
	break;
    case 6:
	str2 = ob->query_prop(OBJ_M_NO_GET);
	if (!stringp(str2))
	    str += "Nie mozesz wziac " + ob->short(PL_DOP) + ".\n";
	else
	    str = str2;
	break;
    case 8:
	str += capitalize(ob->short(PL_MIA)) + " " + 
	    (ob->query_tylko_mn() ? "sa" : "jest") + " zbyt duz" +
	    ob->koncowka("y", "a", "e", "i", "e");
	    
	str += ".\n";
	break;
    case 9:
	str += "Nie mozesz wyjac " + ob->short(PL_DOP) + " z zamknietego " +
	    "pojemnika.\n";
	break;
    case 10:
	str += "Nie jestes w stanie wlozyc " + ob->short(PL_DOP) + 
	    " do zamknietego pojemnika.\n";
    	break;
    }

    write(str);
}
   
/*
 * Function name: manip_drop_access
 * Description:   Test if player carries an object
 * Arguments:	  ob: object
 * Returns:       1: caried object
 *		  0: otherwise
 *
 */
int
manip_drop_access(object ob)
{
    if (!objectp(ob))
	return 0;
    if (environment(ob) == this_player())
	return 1;
    return 0;
}

/*
 * Function name: manip_give_access
 * Description  : This function is called to see whether the player can
 *		actually give this object. To be able to give an object
 *		to another player, you must first have it yourself.
 * Arguments    : object ob - the object to give.
 * Returns      : int 1/0 - true if the player has the object in his
 *			  inventory.
 */
int
manip_give_access(object ob)
{
    if (!objectp(ob))
    {
	return 0;
    }

    return (environment(ob) == this_player());
}

/*
 * Function name: manip_relocate_to
 * Description:   Test if an object can be moved to another one and do it.
 * Arguments:	  item_o: object to be moved
 *		  to: object to move to
 * Returns:       1: object moved
 *		  0: otherwise
 * Outputs:	  move error messages
 * Notify_fail:   ""  (no notify fail because of output of error messages)
 *
 */
varargs int
manip_relocate_to(object item_o, object to)
{
    object dest;	/* receiver */
    int ierr;
    string destmsg;
    
    if (!objectp(item_o))
	return 0;
    if (!to)
    {
	dest = environment(this_player());
	destmsg = "";
    }
    else
    {
	dest = to;
	destmsg = (gBezokol == "dac" ? " " + QSHORT(dest, PL_CEL)
				     : " do " + QSHORT(dest, PL_DOP));
    }
    
    if (item_o == dest)
	return 0;		/* not into itself */
    if (item_o == this_player())
	return 0;		/* not him self */

    if (item_o->query_prop(HEAP_I_IS) && query_verb() == "ukryj")
	item_o->set_no_merge(1);

    if (!(ierr = item_o->move(dest)))
	return 1;

    if (!silent && !item_o->query_no_show())
    {
	saybb(QCIMIE(this_player(), PL_MIA) + " bezskutecznie probuje " +
		gBezokol + " " + (stringp(item_o->short(PL_BIE)) ? 
		QSHORT(item_o, PL_BIE) : 
		item_o->query_name(PL_BIE)) + destmsg + ".\n",
		({ this_player(), dest }) );
		
	if (living(dest))
	    dest->catch_msg(QCIMIE(this_player(), PL_MIA) + " bezskutecznie " +
		"probuje ci " + gBezokol + " " + (stringp(item_o->short(PL_BIE)) ? 
		QSHORT(item_o, PL_BIE) : item_o->query_name(PL_BIE)) + 
		".\n");
    }
		

    move_err_short(ierr, item_o, dest);
    notify_fail("");
    return 0;
}

int
manip_put_dest(object item) 
{ 
    return manip_relocate_to(item, gDest); 
}

int
manip_set_dest(string prep, object *carr)
{ 
    string vb;

    if (!carr || sizeof(carr) == 0)
    {
	notify_fail(capitalize(gBezokol) + " " + (prep == "w" ?
		    "w czym" : prep + " czego") + "?\n");
	return 0;
    }
    if (sizeof(carr) > 1)
    {
	notify_fail("Zdecyduj sie, gdzie chcesz to " + gBezokol + ".\n");
	return 0;
    }
    gDest = carr[0];

    if (living(gDest))
    {
	notify_fail(gDest->query_Imie(this_player(), PL_MIA) + 
	    " nie zgodzi sie na to.\n");
	return 0;
    }

/*
    if (parse_command(prep, ({0}), "'w' / 'do' / 'wewnatrz'"))
 */

    if (prep == "do")
    {
	notify_fail("Co chcesz " + gBezokol + " " + prep + " " +
/*
		    gDest->short((prep == "w" ? PL_BIE : PL_DOP)) + "?\n");
 */
		    gDest->short(PL_DOP) + "?\n");
	return 1;
    }
    else
    {
	notify_fail("Nie rozumiem, co to znaczy wlozyc " + prep +".\n");
	return 0;
    }
}

/*
 * Function name: manip_relocate_from
 * Description:   tries to transfer object to player
 * Arguments:	  item_o: object
 * Returns:       1: transfered the object
 *		  0: some obstruction
 * Outputs:       Messages on failed transfer.
 * Notify_fail    Is set to 0 to hide access failure.
 * Ex:	       get item(s), get item(s) from container(s)
 *
 */
int
manip_relocate_from(object item_o)
{
    int ierr;
    object env;
    string tmp;
    
    if (!objectp(item_o))
	return 0;
    if (item_o == this_player())
	return 0;
    env = environment(item_o);
    if (env == this_player())
	return 0;

    if (env->query_prop(CONT_I_HIDDEN) || env->query_prop(CONT_I_CLOSED))
    {
	tmp = " " + gBezokol + " " + QSHORT(item_o, PL_BIE) + " z ";
	if (living(env))
	{
	    tell_object(env, this_player()->query_imie(env, PL_MIA) +
		" bezskutecznie probuje" + tmp + " cie.\n");
	}
	saybb(QCIMIE(this_player(), PL_MIA) + " bezskutecznie probuje" + tmp + 
	    QSHORT(env, PL_DOP) + ".\n", ({ env, this_player() }) );
	this_player()->catch_msg("Nie udaje ci sie " + tmp + 
	    QSHORT(env, PL_DOP) + ".\n");
	if (env->query_prop(CONT_I_CLOSED))
	{
	    tmp = capitalize(env->short(PL_MIA)) + (env->query_tylko_mn()
	      ? "sa" : "jest") + " zamknie" + env->koncowka("ty", "ta", "te",
	      "ci", "te");
	      
	    write(tmp + ".\n");
	}
	notify_fail("");
	return 0;
    }

    if (item_o->query_prop(HEAP_I_IS))
	item_o->set_no_merge(1);
    
    if ((ierr = item_o->move(this_player())) == 0)
    {
	gFrom = gFrom + ({env});
	return 1;
    }

    if (!silent && !item_o->query_no_show())
    {
	if (living(item_o))
	    item_o->catch_msg(this_player()->query_Imie(item_o, PL_MIA) +
		" bezskutecznie probuje " + gBezokol + " ciebie.\n");
	saybb(QCIMIE(this_player(), PL_MIA) + " bezskutecznie probuje " +
	    gBezokol + " " +
	    (stringp(item_o->short(PL_BIE)) ? QSHORT(item_o, PL_BIE) :
	    item_o->query_name(PL_BIE)) + ".\n", ({ item_o, this_player() }));
    }
    move_err_short(ierr, item_o, this_player());
    notify_fail("");    /* to hide the failed access one  */
    return 0;
}

/*
 * Function name: manip_put_whom
 * Description  : We only allow the object to be moved if this_player()
 *		  has it in his inventory. For additional information on
 *		  the move process and the return values, see the documents
 *		  on manip_relocate_to(item, gDest).
 * Arguments    : object item - the object to move.
 * Returns      : int 1/0 - it will fail if 'item' is not in this_player(),
 *			    else see manip_relocate_to().
 */
int
manip_put_whom(object item) 
{
    if (environment(item) != this_player())
    {
	return 0;
    }

    return manip_relocate_to(item, gDest);
}

int
manip_set_whom(object *carr) /* Argument prep usuniety */
{ 
    mixed tmp;
    int i;

    if (!carr || sizeof(carr) == 0)
	return 0;

    if (sizeof(carr) > 1)
    {
	notify_fail("Zdecyduj sie, komu chcesz to dac?\n");
	return 0;
    }

    gDest = carr[0];
    if (gDest->query_npc() &&
	(tmp = gDest->query_prop(NPC_M_NO_ACCEPT_GIVE)))
    {
	if (stringp(tmp))
    	notify_fail(gDest->query_Imie(this_player(), PL_MIA) + tmp);
	return 0;
    }

    notify_fail("Co chcesz " + gBezokol + " " +
	gDest->query_imie(this_player(), PL_CEL) + "?\n");
	
    return 1;
}

/*
 * Function name: in_gContainers
 * Description:   test if object is in one of a set of containers
 * Arguments:	  ob: object
 * Returns:       1: is in the conatiner
 *		  0: not in the container
 * globals	  gContainers: the array of containers
 *
 */
int
in_gContainers(object ob) 
{
    mixed res;

    if (!objectp(ob))
	return 0;
    if (environment(ob) != gContainers[0] &&
		environment(ob) != gContainers[0]->query_room())
	return 0;

    if ((gContainers[0]->query_prop(CONT_I_CLOSED) &&
	    !gContainers[0]->query_prop(CONT_I_TRANSP)) ||
	    gContainers[0]->query_prop(CONT_I_HIDDEN))
	return 0;

    return 1;
}

/*
 * Here are some functions with the looks command.
 */
void
show_contents(object cobj)
{
    object *obarr, linked;
    string str;

    if (linked = (object)cobj->query_room())
	obarr = all_inventory(linked);
    else
    	obarr = all_inventory(cobj);
    obarr = filter(obarr, &visible(, cobj));
    if (sizeof(obarr) > 0)
    {
	str = COMPOSITE_DEAD(obarr, PL_BIE);
	write(capitalize(cobj->short(PL_MIA)) + " zawiera" +
	    (cobj->query_tylko_mn() ? "ja " : " ") + 
	    str + ".\n");
    }
    else
    {
	str = "  " + capitalize(cobj->short()) + " " +
	    (cobj->query_tylko_mn() ? "sa" : "jest") + " pus" +
	    cobj->koncowka("ty", "ta", "te", "ci", "te");
	
	write(str + ".\n");
    }
}

void
look_living_exec(mixed plr)
{
    write(plr->long());
}

/*
 * Function name: show_exec
 * Description:   Shows an item depending on its position, normally the long
 *		  description, but short description for items carried or
 *		  inside other items.
 * Arguments:	  object ob
 *
 */
void 
show_exec(object ob)
{
    object env;
    string str;
    
    env = environment(ob); str = 0;

    if (env == this_player() || env == environment(this_player()))
	write(ob->long());
    
    /* objects inside transparent objects */
    while (env != this_player() && env != environment(this_player()))
    {
	if (!strlen(str))
	    str = "Widzisz " + ob->short(this_player(), PL_BIE);
	if (living(env))
	{
	    str += " niesion";
	    switch(ob->query_rodzaj())
	    {
		case PL_MESKI_OS:
		case PL_MESKI_NOS_ZYW:
		case PL_MESKI_NOS_NZYW: str += "ego"; break;
		case PL_ZENSKI: str += "a"; break;
		default: str += "e"; break;
	    }
	     str += " przez " + env->short(this_player(), PL_BIE);
	}
	else
	    str += " wewnatrz " + env->short(this_player(), PL_DOP);
	env = environment(env);
    }

    if (str) 
    {
	str += ".\n";
	write(str);
    }
}

/*
 * Function name: item_access
 * Description:   test if an object contains (pseudo) item description gItem
 * Arguments:	  object ob
 * Returns:       1: found gItem
 *		  0: failed to find gItem
 * Globals:       string gItem
 *
 */
int 
item_access(object ob)
{
    if (!objectp(ob))
	return 0;
	
    return (int) ob->item_id(gItem);
}

int
inside_visible(object cobj)
{
    object env;

    if (!objectp(cobj) || cobj->query_no_show())
	return 0;

    /* Properties stop us from seing the inside
     */
    if (!cobj->query_prop(CONT_I_IN) || cobj->query_prop(CONT_I_HIDDEN) ||
		(cobj->query_prop(CONT_I_CLOSED) &&
	 	!cobj->query_prop(CONT_I_TRANSP) &&
	 	!cobj->query_prop(CONT_I_ATTACH)))
	return 0;	

    env = environment(cobj);
    if (env == this_player() || env == environment(this_player()) ||
		visibly_hold(cobj))
	return 1;

    while (env && (!env->query_prop(CONT_I_CLOSED) ||
	    env->query_prop(CONT_I_TRANSP)) && !env->query_no_show())
    {
	if (visibly_hold(env))
	    return 1;
	env = environment(env);
	if (env == this_player() || env == environment(this_player()))
	    return 1;
    }
    return 0;
}

varargs int
visible(object ob, object cobj)
{
    object env;

    if (!objectp(ob))
	return 0;

    if (cobj && (env = (object)cobj->query_room()) &&
		(cobj->query_prop(CONT_I_TRANSP) ||
		!cobj->query_prop(CONT_I_CLOSED)))
    {
	return ((env->query_prop(OBJ_I_LIGHT) >
		-(this_player()->query_prop(LIVE_I_SEE_DARK))) && 
		CAN_SEE(this_player(), ob));
    }
	
    env = environment(ob);
    if (env == this_player() || env == environment(this_player()))
	return CAN_SEE(this_player(), ob);

    while (objectp(env) && !living(env) && (env->query_prop(CONT_I_TRANSP) ||
		!env->query_prop(CONT_I_CLOSED)))
    {
	env = environment(env);
	if (env == this_player() || env == environment(this_player()))
	    return CAN_SEE(this_player(), ob);
    }
    return 0;
}

/* 
 * Is 
 */
int
visibly_hold(object ob)
{
    object env;
    if (!objectp(ob))
	return 0;
 
    env = environment(ob);
    while (objectp(env))
    {
	if (env == gHolder)
	    return 1;

	if (env->query_prop(CONT_I_HIDDEN) ||
	    (!env->query_prop(CONT_I_TRANSP) &&
	     !env->query_prop(CONT_I_ATTACH) &&
	     env->query_prop(CONT_I_CLOSED)))
	    return 0;
	else
	    env = environment(env);
    }
    return 0;
}

/* 
 * Look ended here.
 */

/* **************************************************************************
 * Here follows the actual functions. Please add new functions in the 
 * same order as in the function name list.
 * **************************************************************************/

/*
 * appraise - Appraise something
 */
int
appraise(string str)
{
    notify_fail("Komenda 'appraise' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'ocen'.\n");

    return 0;
}

/*
 * Function name: count
 * Description:   Let the players count stuff other than coins (heaps)
 * Arguments:	  str - the string describing what players want to count
 */
int
count(string str)
{
    notify_fail("Komenda 'count' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'policz'.\n");

    return 0;
}

/*
 * czas - Zwraca aktualny czas w otoczeniu gracza (zaleznie od domeny).
 */
int
czas(string str)
{
    string czas = environment(this_player())->check_time();

    write(czas ? czas : "To nie jest rodzaj miejsca, w ktorym czas bylby "
	+ "czyms istotnym...\n");
    return 1;
}

/* 
 * daj - Daj cos komus
 */
int
daj(string str)
{
    object *a;
    object *item1, *item2;
    string str2;

    if (!PREV_LIGHT)
	return light_fail("komus cos dac");

    notify_fail(capitalize(query_verb()) + " co komu?\n");

    if (!strlen(str))
    {
	return 0;
    }
    
    silent = 0;

    if (!parse_command(str, environment(this_player()), 
	"%i:" + PL_BIE + " %l:" + PL_CEL, item1, item2))
       return 0;
    
    gBezokol = "dac";

    item2 = CMDPARSE_STD->normal_access(item2, 0, this_object());
    if (!item2)
	return 0;

    item1 = CMDPARSE_STD->normal_access(item1, "manip_give_access",
	this_object());
    if (!item1)
	return 0;

    if (!manip_set_whom(item2))
	return 0;
	
    item1 = filter(item1, "manip_put_whom", this_object());
    
    if (sizeof(item1) > 0)
    {
	this_player()->set_obiekty_zaimkow(item1, item2);
    
	write("Dajesz " + COMPOSITE_DEAD(item1, PL_BIE) + " " +
	    gDest->query_imie(this_player(), PL_CEL) + ".\n");
	gDest->catch_msg(this_player()->query_Imie(gDest, PL_MIA) +
	    " daje ci " + QCOMPDEAD(PL_BIE) + ".\n");
    	saybb(QCIMIE(this_player(), PL_MIA) + " daje " + QCOMPDEAD(PL_BIE)
    	   + " " + QIMIE(gDest, PL_CEL) + ".\n", ({ gDest, this_player()}) );
    	return 1;
    }

    return 0;
}

int
drop(string str)
{
    notify_fail("Komenda 'drop' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'poloz'.\n");

    return 0;
}

/*
 * policz - pozwala graczom policzyc cos innego niz monety
 */
int
policz(string str)
{
    object *ob;
    int nr;
    int rodzaj;

    if (!stringp(str))
    {
	notify_fail("Co chcesz policzyc?\n");
	return 0;
    }

    gBezokol = "policzyc";

    ob = FIND_STR_IN_OBJECT(str, this_player(), PL_BIE);
    if (!sizeof(ob))
	ob = FIND_STR_IN_OBJECT(str, environment(this_player()), PL_BIE);

    if (sizeof(ob))
    {
 	/* Heaps (coins) have their own routines for countind */
	if (ob[0]->query_prop(HEAP_I_IS))
	    return 0;

	nr = sizeof(ob);

	write ("Doliczyl" + (kobieta() ? "as" : "es") + " sie " +
	    LANG_SNUM(nr, PL_DOP, PL_ZENSKI) + " " +
	    (nr > 1 ? "sztuk" : "sztuki") + ".\n");
	
//            LANG_SNUM(nr, PL_DOP, ob[0]->query_rodzaj()) + " " + 
//           (nr == 1 ? ob[0]->short(PL_DOP) : 
//            ob[0]->plural_short(LANG_PRZYP(nr, PL_DOP, 
//            ob[0]->query_rodzaj()))) + ".\n");
	return 1;
    }

    notify_fail("Nie widzisz tu niczego takiego.\n");
    return 0;
}


/*
 * get - get something
 */
/*
 * Function name: get, pick, take
 * Description:   get items from environment or containers
 * Arguments:	  str: tail of command string
 * Returns:       1: did get something
 *		  0: 
 * Globals:	  gFrom: sources of items
 *		  gContainers: containers 
 * Notify_fail:   Get what?
 *		  Get from what?
 *
 * Ex:		  get item(s), get item(s) from container(s)
 *
*/
int
get(string str)
{
    notify_fail("Komenda 'get' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'wez'.\n");

    return 0;
}

/* 
 * give - Give something to someone
 */
/*
 * Function name: give
 * Description:   tries to move object to another living
 * Arguments:	  str: tail of command string
 * Returns:       1: manage to give something
 *		  0: failed
 * Ex:		  give item(s) to player
 */
int
give(string str)
{
    notify_fail("Komenda 'give' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'daj'.\n");

    return 0;
}

/*
 * hide - Hide something.
 */
int
hide(string str)
{
    notify_fail("Komenda 'hide' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'ukryj'.\n");

    return 0;
}

int
filter_inv(object ob)
{
    return !(ob->query_no_show() || ob->query_no_show_composite());
}

/*
 * inventory - List things in my inventory
 */
int
inwentarz(string str)
{
    object tp, *obarr;
    int id;

    if (stringp(str))
    {
	if (!(this_player()->query_wiz_level()))
	{
	    notify_fail("Czyj inwentarz?\n");
	    return 0;
	}

	str = capitalize(lower_case(str));
	
	id = member_array(str, users()->query_met_name(PL_DOP));
	
	if (id != -1)
	{
	    tp = users()[id];
	}	
	else
	{
	   if (!objectp(tp = find_player(str)))
	   {
	       if (!objectp(tp = find_living(str)))
	       {
		   notify_fail("Nie ma nikogo takiego.\n");
		   return 0;
	       }
	   }
	}
    }
    else
	tp = this_player();

    if (PREV_LIGHT <= 0)
    {
	return light_fail("dostrzec cokolwiek");
    }

    tp->add_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS, 1);
    id = set_alarm(0.5, 0.0,
	      &tp->remove_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS));
    write(tp->show_sublocs(this_player()));
    tp->remove_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS);
    remove_alarm(id);

    obarr = (object*)tp->subinventory(0);
    obarr = filter(obarr, &filter_inv());
    if (sizeof(obarr) > 0)
    {
	str = COMPOSITE_DEAD(obarr, PL_BIE);
	write((tp == this_player() ? "Masz" : 
	    tp->query_name(PL_MIA) + " ma") + " przy sobie " + 
	    str + "." + "\n");
    }
    else
    {
	write("  " + (tp == this_player() ? "Nie masz" : 
	   tp->query_name(PL_MIA) + " nie ma") +
	      " nic przy sobie.\n");
    }
    return 1;
}

/*
 * keep - set the OBJ_M_NO_SELL property in an object.
 * unkeep - remove the OBJ_M_NO_SELL property from an object.
 */
int
keep(string str)
{
    notify_fail("Komendy 'keep' i 'unkeep' zostaly wycofane. Zamiast " +
	"nich mozesz uzyc 'zabezpiecz' i 'odbezpiecz'.\n");

    return 0;
}

/*
 * look - Look at something
 */
/*
 * Function name: look
 * Description:   glances around, examines objects, or looks at pseudo items
 * Arguments:	  str: tail of look command
 * Returns:       1: found something to look at
 *		  0: failed to find object
 * Notify_fail:   several
 * 
 * Globals:       gItem:  
 *		  gHolder:
 * Ex:	    look, look at orcs, look inside chest
 *
   Documentation of look
   
   The look at (in, inside) command has the following features.

   look	
   	Shows long description of the room and short description of
   	all visible items in the environment (see do_glance() ).

   look at 'multiple objects'
     	Show the long description of several objects, carried or in the
	environment and the short description of object inside transparent
	objects.
	
   look at 'multiple living'
     	Show the long description of several living in the environment.

   look at 'single living'
   	Show the long description of one living in the environment and
	the objects carried by this being.

   look in 'containers'
     	Show the long description of the containers and short descriptions 
	their contents.

   look at 'single object'
     	Shows the long description of the object and if the object is open
	and a container, shows short descriptions of it contents.
   
   look at 'object' carried by 'living'
   	Shows the short description of the object and if the object is
	an open or transparent container shows short descriptions of
	it contents.
   
 */
int 
look(string str)
{
    notify_fail("Komenda 'look' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'spojrz', lub prosciej - 'sp'.\n");

    return 0;
}


/*
 * examine - Examine something
 */
/*
 * Function name: examine
 * Description:   pseudonym for  look at, look in, etc
 * Arguments:	  string str: tail of examine command or exa command
 * Returns:       1: found something to look at
 *		  0: failed to found object
 * Ex:	    examine("knife")
 *
*/
int
examine(string str)
{
#if 0
    if (!stringp(str))
    {
	notify_fail("Examine what?\n");
	return 0;
    }

    return look("prp_examine " + str);
#endif

    notify_fail("Komenda 'examine' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'obejrzyj', lub prosciej - 'ob'.\n");

    return 0;

}

/*
 * peek - Peek into someone's inventory, part of someone's inventory.
 */
int
peek(string str)
{
    notify_fail("Komenda 'peek' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'podejrzyj'.\n");

    return 0;
}

/*
 * przemknij - przemknij gdzies
 */
int
przemknij(string str)
{
    int hiding, *dirs, i, val, bval;
    string str2;

    if (!stringp(str))
    {
	notify_fail("Gdzie chcesz sie przemknac?\n");
	return 0;
    }

    if (str[0..3] == "sie ")
	str = str[4..];
	
    if (str[0..2] == "na ")
    {
	i = 1;
	str = str[3..];
    }
    else i = 0;
    
    gBezokol = "przemknac";

/*
 * W /secure/master.c pamietane sa kierunki w zlej formie (gora/gore)
 
    str2 = SECURITY->modify_command(str, environment(this_player()));
*/

    str2 = sneak_dirs[str];
    if (strlen(str2))
	str = "na " + str2;
    else if (i)
	str = "na " + str;

    dirs = environment(this_player())->query_exit_full_cmds();
    if ((i = member_array(str, dirs)) < 0)
    {
	notify_fail("Gdzie chcesz sie przemknac?\n");
	return 0;
    }

    if (this_player()->query_prop(OBJ_I_LIGHT) &&  
	(this_player()->query_prop(OBJ_I_LIGHT) >=
	environment(this_player())->query_prop(OBJ_I_LIGHT)))
    {
	notify_fail("Nie potrafisz przemknac sie niezauwazenie " +
	    "posiadajac przy sobie zrodlo swiatla.\n");
	return 0;
    }

    if (objectp(this_player()->query_attack()))
    {
	notify_fail("Nie mozesz tak po prostu wymknac sie z walki.\n");
	return 0;
    }

    hiding = environment(this_player())->query_prop(ROOM_I_HIDE);
    bval = this_player()->query_skill(SS_SNEAK) * 2 + this_player()->query_skill(SS_HIDE) / 3;
    bval = (bval - hiding) / 2;

    if (hiding < 0 || bval <= 0)
    {
	notify_fail("Nie jestes w stanie wymknac sie stad niezauwazenie.\n");
	return 0;
    }	

    val = bval + random(bval);
    this_player()->add_prop(OBJ_I_HIDE, val);

    this_player()->add_prop(LIVE_I_SNEAK, 1);
    this_player()->command(environment(this_player())->query_exit_cmds()[i]);

    hiding = environment(this_player())->query_prop(ROOM_I_HIDE);
    bval = this_player()->query_skill(SS_HIDE);
    bval = (bval - hiding) / 2;

    if (hiding < 0 || bval <= 0)
    {
	write("Jest tu zbyt ciezko sie schowac, wiec jestes widoczn" +
	    (kobieta() ? "a" : "y") + " z powrotem.\n");
	this_player()->reveal_me(0);
	return 1;
    }	

    if (this_player()->query_prop(OBJ_I_LIGHT) &&  
	(this_player()->query_prop(OBJ_I_LIGHT) > 
	environment(this_player())->query_prop(OBJ_I_LIGHT)))
    {
	write("Posiadajac zrodlo swiata nie jestes w stanie ukryc sie " +
	    "skutecznie.\n");
	this_player()->reveal_me(1);
	return 1;
    }

    val = bval + random(bval);
    this_player()->add_prop(OBJ_I_HIDE, val);
    return 1;
}


/*
 * Przeszukaj - Przeszukaj cos
 */
int
przeszukaj(string str)
{
    object *ob, obj;
    int time;
    string item, rest;

    notify_fail("Przeszukaj co?\n");
    
    if (!stringp(str))
	return 0;

    if (this_player()->query_attack())
    {
	write("Ale przeciez jestes w srodku walki!\n");
	return 1;
    }

    if (!CAN_SEE_IN_ROOM(this_player()))
	return light_fail("dostrzec cokolwiek");
    
    gBezokol = "przeszukac";

    if (!sizeof(ob = FIND_STR_IN_OBJECT(str, this_player(), PL_BIE)) &&
	!sizeof(ob = FIND_STR_IN_ARR(str, 
	    (all_inventory(environment(this_player())) - ({ this_player() })),
	    PL_BIE)))
    {
	 if (environment(this_player())->item_id(str))
	     ob = ({environment(this_player())});
    }
    else
	ob = FILTER_CAN_SEE(ob, this_player());

    if (!sizeof(ob))
	return 0;

    obj = ob[0];

    if (obj == environment(this_player()))
    {
	write("Zaczynasz przeszukiwac " + str + ".\n");
	saybb(QCIMIE(this_player(), PL_MIA) + " zaczyna przeszukiwac " +
	    str + ".\n");
    } 
    else 
    {
	if (living(obj))
	{
	    write("Zaczynasz przeszukiwac " +
		obj->query_imie(this_player(), PL_BIE) + ".\n");
	    tell_object(obj, this_player()->query_Imie(obj, PL_MIA) +
		" zaczyna przeszukiwac CIEBIE!\n");
	    saybb(QCIMIE(this_player(), PL_MIA) + " zaczyna przeszukiwac " + 
		QIMIE(obj, PL_BIE) + 
		".\n", ({ obj, this_player() }));
   	} 
   	else 
   	{
	    write("Zaczynasz przeszukiwac " + COMPOSITE_DEAD(obj, PL_BIE) + 
		".\n");
	    saybb(QCIMIE(this_player(), PL_MIA) + " zaczyna przeszukiwac " + 
		QSHORT(obj, PL_BIE) + ".\n");
	}
    }

    obj->search_object(str, 1);
    return 1;
}


/*
 * put - Put something
 */
/*
 * Function name: put, drop
 * Description:   put items in environment or cointainers
 * Arguments:	  str: tail of command string
 * Returns:       1: did get something
 *		  0: failedd to get anything
 * Notify_fail:   * "Put what?"
 *		  * 
 * Ex:	       drop item(s), put item(s) in container
 *
 */
int
put(string str)
{
    notify_fail("Komenda 'put' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'wloz'.\n");

    return 0;
}

/*
 * ocen - Ocen jakas rzecz
 */
int
ocen(string str)
{
    object *ob;
    int i;

    if (PREV_LIGHT <= 0)
	return light_fail("oceniac");

    notify_fail("Umm, co chcesz ocenic?\n");
    if (!stringp(str))
	return 0;

    gBezokol = "ocenic";

    if (str == "siebie")
	ob = ({this_player()});
    else if (!sizeof(ob = FIND_STR_IN_OBJECT(str, this_player(), PL_BIE)) &&
	     !sizeof(ob = FIND_STR_IN_OBJECT(str, environment(this_player()),
	                                     PL_BIE)))
	return 0;

    for (i = 0; i < sizeof(ob); i++)
    {
	if (living(ob[i]))
	    this_player()->catch_msg("Ogladasz dokladnie " +
			ob[i]->query_imie(this_player(), PL_BIE) + 
			".\n");
	else
	    this_player()->catch_msg("Oceniasz starannie " +
			ob[i]->short(this_player(), PL_BIE) + ".\n");
	ob[i]->appraise_object();
    }

    return 1;
}

/*
 *
 */
int
odloz(string str)
{
    object *itema;
    
    gBezokol = (query_verb() == "odloz" ? "odlozyc" : "polozyc");
    notify_fail(capitalize(query_verb()) + " co?\n");
    
    if (!stringp(str))
	return 0;

    /* This is done to avoid all those stupid messages 
       when you try 'drop all' 
     */
    if (str == "wszystko")
	silent = 1;
    else
	silent = 0;
    
    if (parse_command(str, environment(this_player()), "%i:" + PL_BIE, itema)) 
    {
	itema = CMDPARSE_STD->normal_access(itema, "manip_drop_access",
		this_object(), 1);

	if (sizeof(itema) == 0) 
	{
	    if (silent)
		notify_fail("Nic nie odlozyl" + 
		    this_player()->koncowka("es", "as") + ".\n");
	    return 0;
	}
	itema = filter(itema, manip_relocate_to);

	if (sizeof(itema) > 0)
	{
	    this_player()->set_obiekty_zaimkow(itema);
	    write("Odkladasz " + COMPOSITE_DEAD(itema, PL_BIE) + ".\n");
	    say(QCIMIE(this_player(), PL_MIA) + " odklada " + 
		QCOMPDEAD(PL_BIE) + ".\n");
	    return 1;
	}
    }
    

    if (!PREV_LIGHT)
	return light_fail("dostrzec cokolwiek");

    return 0;
}

/*
 * obejrzyj - podaje opis jakiegos przedmiotu
 */
int
obejrzyj(string str)
{
    string	*tmp;
    int		i;
    object	*obarr, *obd, *obl;
	    
    if (!stringp(str))
    {
	notify_fail("Co chcesz obejrzec?\n");
	return 0;
    }

    if (PREV_LIGHT <= 0)
	return light_fail("dostrzec cokolwiek");

    gItem = lower_case(str);
    gBezokol = "obejrzec";

    str = lower_case(str);
    tmp = explode(str, " ");
    if (sizeof(tmp) > 1 && tmp[1][0] == '0')
	return 0;
	
    if (!parse_command(str, ENV, "%i:" + PL_BIE, obarr) ||
	!sizeof(obarr = NORMAL_ACCESS(obarr, "visible", this_object())))
    {
	/* No objects found */
	/* Test for pseudo item in the environment */
	if (environment(this_player())->item_id(str) &&
		CAN_SEE(this_player(), ENV))
	{
	    write(environment(this_player())->long(str));
	    return 1;
	}
	else
	{
	    obarr = deep_inventory(environment(this_player()));
	    obarr = filter(obarr, visible);
	    obarr = filter(obarr, item_access); 
	    if (sizeof(obarr) > 0) 
	    {
		if (obarr[0]->item_id(str))
		    write(obarr[0]->long(str));
		else 
		    for (i = 0; i < sizeof(obarr); i++)
			write(obarr[i]->long(str));
			
		return 1;
	    } 
	    else 
	    {
		if (str == "siebie" || str == "mnie")
		{
		    write(this_player()->long(this_player()));
		    return 1;
		}
		if ((str == "przeciwnika" || str == "wroga") && 
		    this_player()->query_attack())
		{
		    write(this_player()->query_attack()->long(this_player()));
		    return 1;
		}
		notify_fail("Nie zauwazasz niczego takiego.\n");
		return 0;
	    }
	}
    }

    if (sizeof(obarr) == 0)
    {
	notify_fail("Nie zauwazasz niczego takiego.\n");
	return 0;
    }

    obd = FILTER_DEAD(obarr);
    obl = FILTER_LIVE(obarr);
    if (sizeof(obd) == 0 && sizeof(obl) == 0)
    {
	notify_fail("Nie zauwazasz niczego takiego.\n");
	return 0;
    }

    /* if single container we show the contents */
    if (sizeof(obd) == 1 && inside_visible(obd[0]))
    {
	show_exec(obd[0]);
	if (!obd[0]->query_prop(CONT_I_DONT_SHOW_CONTENTS))
	    show_contents(obd[0]);
    } 
    else
	map(obd, show_exec);

    /* if a single living being we show carried items */
    if (sizeof(obl) == 1)
	look_living_exec(obl[0]);
    else
	map(obl, show_exec);
	
    this_player()->set_obiekty_zaimkow(obarr);

    return 1;
}

int 
peek_access(object ob)
{
    if (!living(ob) || ob->query_ghost() || ob == this_player())
	return 0;
    else
	return 1;
}

/*
 * podejrzyj - Podejrzyj czyjs ekwipunek, lub czesc ekwipunku.
 */
int
podejrzyj(string str)
{
    string vb;
    object *p, *inv;
    int id, i, pp_skill;

    vb = query_verb(); gBezokol = "podejrzec";

    notify_fail(capitalize(vb) + " czyj ekwipunek?\n");

    if (!stringp(str))
	return 0;

    p = CMDPARSE_ONE_ITEM(str, "peek_access", "peek_access");

    if (!sizeof(p))
    {
	return 0;
    }
    if (sizeof(p) > 1)
    {
	notify_fail("Mozesz podejrzec ekwipunek tylko jednej osobie naraz.");
	return 0;
    }

    pp_skill = this_player()->query_skill(SS_PICK_POCKET) / 2;
    if ((pp_skill + random(pp_skill) > p[0]->query_skill(SS_AWARENESS)) &&
	(!p[0]->query_wiz_level()))
    {
	inv = all_inventory(p[0]);

	p[0]->add_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS, 1);
	id = set_alarm(0.1, 0.0, &(p[0])->remove_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS));
	write(p[0]->show_sublocs(this_player()));
	p[0]->remove_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS);
	remove_alarm(id);

	inv = (object*)p[0]->subinventory(0);
	inv = FILTER_SHOWN(inv);
	if (sizeof(inv))
	    write(p[0]->query_Imie(this_player(), PL_MIA) +
		" ma przy sobie " +
		COMPOSITE_DEAD(inv, PL_BIE) + ".\n");
	else
	    write(p[0]->query_Imie(this_player(), PL_MIA) +
		" nie ma nic przy sobie.\n");
    }
    else
    {
	tell_object(p[0], "Przylapujesz " +
	    this_player()->query_Imie(p[0], PL_BIE) +
	    " na penetrowaniu twojego wlasnego ekwipunku! Co za brak "+
	    "kultury...\n");
	write("O kurcze! Zdaje sie, ze " + 
	    p[0]->query_Imie(this_player(), PL_MIA) + " przylapal" +
	    p[0]->koncowka("", "a") + " cie!!\n");
    }
    return 1;
}


/*
 * ukryj - Chowanie czegos.
 */
int
ukryj(string str)
{
    object *itema, *cont;
    string vb = query_verb();
    int hiding, i, val, bval, poorly, self;

    notify_fail(capitalize(vb) + " co?\n");

    gBezokol = "ukryc";

    if (!stringp(str))
	return 0;

    if (sscanf(str, "pobieznie %s", str))
	poorly = 1;
    else
	poorly = 0;
	
    if (str == "sie" || str == "siebie")
	self = 1;

    if (this_player()->query_prop(OBJ_I_LIGHT) &&  
	(this_player()->query_prop(OBJ_I_LIGHT) > 
	environment(this_player())->query_prop(OBJ_I_LIGHT)))
    {
	notify_fail("Swiecac tak nie jestes w stanie " +
	    (self ? "sie dobrze ukryc" : "ukryc czegokolwiek") + "!\n");
	return 0;
    }

    hiding = environment(this_player())->query_prop(ROOM_I_HIDE);
    bval = this_player()->query_skill(SS_HIDE);
    if (hiding < 0 || hiding > bval)
    {
	notify_fail("Nie jestes w stanie " + (self ? "sie tu dobrze schowac" :
	    "tu czegokolwiek schowac") + ".\n");
	return 0;
    }

    bval = (bval - hiding) / 2;
    val = bval + random(bval);

    if ((str == "sie" || str == "siebie") && !poorly)
    {
	cont = all_inventory(environment(this_player()));
	itema = FILTER_LIVE(cont);
	itema = FILTER_CAN_SEE(itema, this_player());
	if (sizeof(itema) > 1)
	{
	    notify_fail("Nie mozesz sie schowac kiedy inni patrza!\n");
	    return 0;
	}

	if (this_player()->query_attack())
	{
	    notify_fail("Podczas walki nie mozesz sie tak po prostu " +
		"schowac.\n");
	    return 0;
	}

	if (this_player()->query_prop(OBJ_I_HIDE))
	{
	    notify_fail("Nie potrafisz schowac sie jeszcze lepiej.\n");
	    return 0;
	}
	else
	{
	    saybb(QCIMIE(this_player(), PL_MIA) + " probuje ukryc sie " +
		"jak najlepiej.\n");
	    write("Chowasz sie najlepiej jak potrafisz.\n");
	    if (this_player()->query_prop(OBJ_I_INVIS))
		this_player()->add_prop(OBJ_I_HIDE, val / 2);
	    else
		this_player()->add_prop(OBJ_I_HIDE, val);
	}
	return 1;
    }

    silent = 0;
    if (str == "wszystko")
	silent = 1;

    if (parse_command(str, environment(this_player()), "%i:" + PL_BIE, itema)) 
    {
	itema = CMDPARSE_STD->normal_access(itema, "manip_drop_access",
		this_object(), 1);
	if (sizeof(itema) == 0) 
	{
	    if (silent)
		notify_fail("Nic nie schowal" + (kobieta() ? "as" : "es") + 
		    ".\n");
	    return 0;
	}
	itema = filter(itema, manip_relocate_to);
	if (poorly)
	{
	    bval = this_player()->query_skill(SS_AWARENESS) / 2;
	    val = val > bval * 2 ? bval + random(bval) : val;
	}

	if (sizeof(itema) > 0)
	{
	    this_player()->set_obiekty_zaimkow(itema);
	    write("Chowasz " + COMPOSITE_DEAD(itema, PL_BIE) + ".\n");
	    saybb(QCIMIE(this_player(), PL_MIA) + " chowa cos.\n");
	    itema->add_prop(OBJ_I_HIDE, val);
	    return 1;
	}
    }

    if (!PREV_LIGHT)
	return light_fail("dostrzec cokolwiek");

    if (silent)
    {
	notify_fail("Nic nie schowal" + (kobieta() ? "as" : "es") + ".\n");
	return 0;
    }

    if (!parse_command(str, environment(this_player()),
	"%i:" + PL_BIE + " 'w' %i:" + PL_MIE, itema, cont))
	return 0;

    cont = NORMAL_ACCESS(cont, 0, 0);
    if (!manip_set_dest("w", cont))
    {
	notify_fail("Ukryj w czym ?\n");
	return 0;
    }

    itema = NORMAL_ACCESS(itema, "manip_drop_access", this_object());
    if (sizeof(itema) == 0)
    {
	notify_fail(capitalize(vb) + " co?\n");
	return 0;
    }

    itema = filter(itema, manip_put_dest);
    if (sizeof(itema) > 0)
    {
	this_player()->set_obiekty_zaimkow(itema, cont);
	write("Chowasz " + COMPOSITE_DEAD(itema, PL_BIE) + " do " +
	    gDest->short(PL_DOP) + ".\n");
	saybb(QCIMIE(this_player(), PL_MIA) + " chowa cos do " +
	    QSHORT(gDest, PL_DOP) + ".\n");
	itema->add_prop(OBJ_I_HIDE, (this_player()->query_skill(SS_HIDE) / 2) +
	    random(this_player()->query_skill(SS_HIDE)));
	return 1;
    }

    return 0;
}


/*
 * reveal - Reveal something hidden
 */
int
reveal(string str)
{
    notify_fail("Komenda 'reveal' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'ujawnij'.\n");

    return 0;
}

/*
 * search - Search something
 */
int
search(string str)
{
    notify_fail("Komenda 'search' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'szukaj' lub 'przeszukaj'.\n");

    return 0;
}

/*
 * sneak - sneak somewhere.
 */
int
sneak(string str)
{
    notify_fail("Komenda 'sneak' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'przemknij sie'.\n");
	
    return 0;
}

/*
 * szukaj - szuka czegos w pokoju
 */
int
szukaj(string str)
{
    gBezokol = "szukac";

    if (this_player()->query_attack())
    {
	write("Ale przeciez jestes w srodku walki!\n");
	return 1;
    }
    
    if (str)
    {
	write("Zaczynasz szukac " + str + ".\n");
	saybb(QCIMIE(this_player(), PL_MIA) + " zaczyna szukac " + str + ".\n");
    }
    else
    {
	write("Zaczynasz przeszukiwac najblizsza okolice.\n");
	saybb(QCIMIE(this_player(), PL_MIA) + " zaczyna przeszukiwac "
	  + "najblizsza okolice.\n");
    }

    environment(this_player())->search_object(str, 0);
    
    return 1;
}


int
spojrz(string str)
{
    gBezokol = "patrzec";
    
    if (!str)
    {
	this_player()->do_glance();
	
	return 1;
    }
    
    if (str[0..2] == "na ")
	return obejrzyj(str[3..]);
	
    notify_fail("Gdzie lub na co chcesz spojrzec?\n");
	
    return 0;
}

/*
 * Function name: track
 * Description:   look for tracks
 * Argument:      str - the string given to the command
 * Returns:       0 - failure
 */
int
track(string str)
{
    notify_fail("Komenda 'track' zostala wycofana. Zamiast " +
	"niej mozesz uzyc 'trop'.\n");
	
    return 0;
}

int
trop(string str)
{
    object  room = ENV;

    gBezokol == "tropic";

    if (this_player()->query_attack())
    {
	notify_fail("Ale jestes w srodku walki!\n");
	return 0;
    }
    
    if (!room->query_prop(ROOM_I_IS))
    {
	notify_fail("Nie mozesz szukac tu sladow!\n");
	return 0;
    }

    if (room->query_prop(ROOM_I_INSIDE))
    {
	notify_fail("Nie mozesz szukac sladow wewnatrz!\n");
	return 0;
    }

    if (this_player()->query_mana() < 2*F_TRACK_MANA_COST)
    {
	notify_fail("Jestes zbyt wyczepran" + (kobieta() ? "a" : "y") + 
	    " mentalnie.\n");
	return 0;
    }

    write("Klekasz szukajac sladow.\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " kleka, by obejrzec dokladnie " +
	"grunt.\n");

    this_player()->add_prop(LIVE_S_EXTRA_SHORT, ", kleczac" +
	this_player()->koncowka("y", "a") + " na ziemi");
    this_player()->add_mana(-F_TRACK_MANA_COST);

    room->track_room();
    return 1;
}


/*
 * ujawnij - Zdekonspiruj cos ukrytego ;-)
 */
int
ujawnij(string str)
{
    object *itema, *cont, linked, *obarr;
    string vb, prep, items;
    int i, size;
    
    if (!PREV_LIGHT) return light_fail("dostrzec cokolwiek");

    vb = query_verb();
    
    gBezokol = "ujawnic";
    
    notify_fail(capitalize(vb) + " co?\n");  /* access failure */

    if (!stringp(str))
	return 0;

    gFrom = ({});


    if (str == "mnie" || str == "sie" || str == "siebie")
    {
	if (this_player()->reveal_me(0))
	{
	    write("Wychodzisz z ukrycia.\n");
	    return 1;
	}
	else
	{
	    notify_fail("Przeciez nie jestes ukryt" + 
		(kobieta() ? "a" : "y") + ".\n");
	    return 0;
	}
    }

    silent = 0;
    if (str == "wszystko")
	silent = 1;

    if (parse_command(str, environment(this_player()), "%i:" + PL_BIE, itema))
    {
	itema = NORMAL_ACCESS(itema, 0, 0);
	itema = filter(itema, &->query_prop(OBJ_I_HIDE));
	if (sizeof(itema) == 0)
	{
	    if (silent)
		notify_fail("Nic nie ujawnil" + 
		    (kobieta() ? "as" : "es") + ".\n");
	    return 0;
	} 
	if (sizeof(itema))
	{
	    itema->remove_prop(OBJ_I_HIDE);
	    this_player()->set_obiekty_zaimkow(itema);

	    write("Ujawniasz " + COMPOSITE_DEAD(itema, PL_BIE) + ".\n");
	    obarr = FILTER_LIVE(itema);
	    say(QCIMIE(this_player(), PL_MIA) + " ujawnia " +
		QCOMPDEAD(PL_BIE) + ".\n", obarr + ({ this_player() }));
	    size = sizeof(obarr);
	    if (size == 1)
	        obarr[0]->catch_msg(this_player()->query_Imie(obarr[i],
		    PL_MIA) + " ujawnia cie.\n");
	    else if (size > 1)
	    {
		i = -1;
		while (++i < size)
		obarr[i]->catch_msg(this_player()->query_Imie(obarr[i],
		    PL_MIA) + " ujawnia ciebie oraz " +
		    FO_COMPOSITE_LIVE((obarr - ({ obarr[i] })), obarr[i],
		    PL_BIE) + ".\n");
	    }
	    itema->force_heap_merge();
	    
	    return 1;
	}
    }

    if (silent)
    {
	notify_fail("Nic nie ujawnil" + 
	    (kobieta() ? "as" : "es") + ".\n");
	return 0;
    }

    gFrom = ({});
    
/*
 * Konstrukcja nie dziala w oryginale... nie chce mi sie na razie u nas
 * poprawiac
 */
/*
    if (parse_command(str, environment(this_player()),
		      "%s 'w' %i:" + PL_MIE, items, cont))
    {
	gContainers = NORMAL_ACCESS(cont, 0, 0);
	gContainers = filter(gContainers, &->query_prop(OBJ_I_HIDE));
	gContainers = FILTER_DEAD(gContainers);
	if (sizeof(gContainers) == 0)
	{
	    notify_fail(capitalize(vb) + " w czym?\n");
	    return 0;
	}

	if (linked = gContainers[0]->query_room())
	    obarr = all_inventory(linked);
	else
	    obarr = deep_inventory(gContainers[0]);

	if (!parse_command(items, obarr, "%i:" + PL_BIE, itema))
	    return 0;

	itema = NORMAL_ACCESS(itema, "in_gContainers", this_object());
	if (sizeof(itema) == 0) 
	    return 0;

	itema->remove_prop(OBJ_I_HIDE);

	write("Ujawniasz " + vb + " " + COMPOSITE_DEAD(itema, PL_BIE) + 
	   " w " + gContainers[0]->short(PL_NAR) + ".\n");
	say(QCIMIE(this_player(), PL_MIA) + " ujawnia " + QCOMPDEAD(PL_BIE) +
	    " w " + QSHORT(gContainers[0], PL_NAR) + ".\n");
	return 1;
    }
*/
    if (environment(this_player())->item_id(str))
    {
	notify_fail("Nie mozesz tego ujawnic.\n");
	return 0;
    }

    return 0;
}


/*
 * wez - wez cos
 */
int
wez(string str)
{
    object *itema, *cont, linked, *obarr;
    string vb, prep, items;
    int i;
    
    if (!PREV_LIGHT) return light_fail("brac cokolwiek");

    vb = query_verb();
    gBezokol = "wziac";
    
    notify_fail(capitalize(vb) + " co?\n");  /* access failure */

    if (!stringp(str))
	return 0;

    gFrom = ({});

    /* This is done to avoid all those stupid messages 
       when you try 'get all' 
    */
    silent = 0;
    if (str == "wszystko")
	silent = 1;

    if (parse_command(str, environment(this_player()), "%i:" + PL_BIE, itema))
    {
	itema = NORMAL_ACCESS(itema, 0, 0);
	itema = filter(itema, manip_relocate_from);
	if (sizeof(itema) == 0)
	{
	    if (silent)
		notify_fail("Nic nie wzi" + (kobieta() ? "elas" : "ales") +
		    ".\n");
	    return 0;
	} 
	if (sizeof(itema) > 0)
	{
	    this_player()->set_obiekty_zaimkow(itema);
	    itema->remove_prop(OBJ_I_HIDE);
	    write("Bierzesz " + COMPOSITE_DEAD(itema, PL_BIE) + ".\n");
	    say(QCIMIE(this_player(), PL_MIA) + " bierze " + 
		QCOMPDEAD(PL_BIE) + ".\n");

	    itema->force_heap_merge();

	    return 1;
	}
    }

    if (silent)
    {
	notify_fail("Nic nie wzi" + (kobieta() ? "elas" : "ales") +
	    ".\n");
	return 0;
    }

    gFrom = ({});

    if (parse_command(str, environment(this_player()),
		      "%s 'z' / 'ze' %i:" + PL_DOP, items, cont))
    {
	gContainers = NORMAL_ACCESS(cont, 0, 0);
	gContainers = FILTER_DEAD(gContainers);
	if (sizeof(gContainers) == 0)
	{
	    notify_fail(capitalize(vb) + " skad?\n");
	    return 0;
	}

	if (linked = gContainers[0]->query_room())
	    obarr = all_inventory(linked);
	else
	    obarr = deep_inventory(gContainers[0]);

	if (!parse_command(items, obarr, "%i:" + PL_BIE, itema))
	    return 0;

	itema = NORMAL_ACCESS(itema, "in_gContainers", this_object());
	if (sizeof(itema) == 0) 
	    return 0;

	itema = filter(itema, manip_relocate_from);
	if (sizeof(itema) == 0)
	    return 0;

	this_player()->set_obiekty_zaimkow(itema, gContainers);
	itema->remove_prop(OBJ_I_HIDE);
	write("Bierzesz " + COMPOSITE_DEAD(itema, PL_BIE) + " z " +
	    gContainers[0]->short(PL_DOP) + ".\n");
	say(QCIMIE(this_player(), PL_MIA) + " bierze " + QCOMPDEAD(PL_BIE) +
	    " z " + QSHORT(gContainers[0], PL_DOP) + ".\n");

	itema->force_heap_merge();

	return 1;
    }

    if (environment(this_player())->item_id(str))
    {
	notify_fail("Nie mozesz tego wziac.\n");
	return 0;
    }
 
    return 0;
}

/*
 * wloz - Wloz cos
 */
int
wloz(string str)
{
    object *itema;
    object *cont;
    string prep, vb;
    
    vb = query_verb(); 
    notify_fail(capitalize(vb) + " co do czego?\n");
    
    if (!stringp(str))
	return 0;

    /* This is done to avoid all those stupid messages 
       when you try 'drop all' 
     */
    silent = 0;
    
    if (!PREV_LIGHT)
	return light_fail("dostrzec cokolwiek.");
	
    gBezokol = "wlozyc";

    if (!parse_command(str, environment(this_player()), 
	"%i:" + PL_BIE + " %w %i:" + PL_DOP, itema, prep, cont)) 
	return 0;
	
    cont = NORMAL_ACCESS(cont, 0, 0);
    if (!manip_set_dest(prep, cont))
    {
	return 0;
    }

    itema = NORMAL_ACCESS(itema, "manip_drop_access", this_object());
    if (sizeof(itema) == 0)
    {
	notify_fail(capitalize(vb) + " co?\n");
	return 0;
    }

    itema = filter(itema, manip_put_dest);
    if (sizeof(itema) > 0)
    {
	this_player()->set_obiekty_zaimkow(itema, cont);
	write("Wkladasz " + COMPOSITE_DEAD(itema, PL_BIE) + " do " +
	    gDest->short(PL_DOP) + ".\n");
	say(QCIMIE(this_player(), PL_MIA) + " wklada " + QCOMPDEAD(PL_BIE) + 
		" do " + QSHORT(gDest, PL_DOP) + ".\n");
	return 1;
    }

/*
 * Blad powinien byc wypisany przez manip_relocate_to(), ktora wywola
 * move_err_short()
 *
 *    notify_fail(capitalize(vb) + " " + prep + " czego?\n");
 */
    notify_fail("");
    return 0;
}

/*
 * keep - set the OBJ_M_NO_SELL property in an object.
 * unkeep - remove the OBJ_M_NO_SELL property from an object.
 */
int
zabezpiecz(string str)
{
    object *objs;
    object *keep_objs;
    int    keep = (query_verb() == "zabezpiecz");
    int    list;

    gBezokol = (keep ? "zabezpieczyc" : "odbezpieczyc");

    if (!stringp(str))
    {
	notify_fail(capitalize(query_verb()) + " co?\n");
	return 0;
    }

    /* Player wants to list, remove the flag. */
    if (list = wildmatch("-l*", str))
    {
	str = extract(str, 3);
    }

    /* Playes wants to list, but didn't give any argument. Get all items
     * in his/her inventory.
     */
    if (list &&
	!strlen(str))
    {
	if (!sizeof(objs = FILTER_CAN_SEE(all_inventory(this_player()),
	    this_player())))
	{
	    notify_fail("Nie masz nic przy sobie.\n");
	    return 0;
	}
    }
    /* Or parse the argument to see which items to process. */
    else if (!parse_command(str, this_player(), "%i:" + PL_BIE, objs) ||
	!sizeof(objs = NORMAL_ACCESS(objs, 0, 0)))
    {
	notify_fail(capitalize(query_verb()) + (list ? " -l" : "") +
	    " co?\n");
	return 0;
    }
    
    this_player()->set_obiekty_zaimkow(objs);

    /* Filter all non-keepable objects. */
    keep_objs = filter(objs, &not() @ &->query_keepable());

    /* List the 'keep' status of the selected items. */
    if (list)
    {
	if (sizeof(keep_objs))
	{
	    write("Niezabezpieczalne --------------\n" +
		break_string(COMPOSITE_DEAD(keep_objs, 0), 70, 5) + "\n");
	    objs -= keep_objs;
	}

	/* Filter all kept objects. */
	keep_objs = filter(objs, &->query_keep());
	if (sizeof(keep_objs))
	{
	    write("Zabezpieczone ------------\n" +
	 	break_string(COMPOSITE_DEAD(keep_objs, 0), 70, 5) + "\n");
	    objs -= keep_objs;
	}

	/* The remainder is keepable, but not kept. */
	if (sizeof(objs))
	{
	    write("Niezabezpieczone --------\n" +
		break_string(COMPOSITE_DEAD(objs, 0), 70, 5) + "\n");
	}
	
	return 1;
    }

    /* None of the objects are keepable. */
    if (sizeof(keep_objs) == sizeof(objs))
    {
	notify_fail("Niezabezpieczalne: " +
	    COMPOSITE_DEAD(keep_objs, 0) + ".\n");
	return 0;
    }

    /* Now select the objects to (un)keep. First remove the non-keepable
     * objects.
     */
    objs -= keep_objs;
    if (keep)
    {
	keep_objs = filter(objs, &not() @ &->query_keep());
    }
    else
    {
	keep_objs = filter(objs, &->query_keep());
    }

    /* No objects to process. */
    if (!sizeof(keep_objs))
    {
	notify_fail((keep ? "Juz zabezpieczone: " : "niezabezpieczone: ") +
	    COMPOSITE_DEAD(objs, 0) + ".\n");
	return 0;
    }

    keep_objs->set_keep(keep);
    write((keep ? "Zabezpieczone: " : "Odbezpieczone: ") + 
	COMPOSITE_DEAD(keep_objs, 0) + ".\n");
    return 1;
}
