/*
 * /std/living/description.c
 *
 * This is a subpart of /std/living.c
 *
 * All description relevant routines are defined here.
 *
 * NOTE
 * There is some calls of the type: this_object()->function()
 * in a number of places. The reason for this is to allow those
 * functions to be shadowed as internal function calls are not
 * possible to shadow.
 */

#include <composite.h>
#include <filter_funs.h>
#include <formulas.h>
#include <language.h>
#include <macros.h>
#include <state_desc.h>
#include <stdproperties.h>
#include <wa_types.h>

#ifdef DAY_AND_NIGHT
#include <mudtime.h>
#endif

private static int appearance_offset;
private static int is_linkdead;

/*
 * Function name: set_linkdead
 * Description  : Set the linkdeath-status of the player.
 * Arguments    : int i - true if the player is linkdead.
 */
public void
set_linkdead(int i)
{
    if (i)
    {
	is_linkdead = time();
    }
    else
    {
	is_linkdead = 0;
    }
}

/*
 * Nazwa funkcji:   ustaw_imie
 * Opis		  : Dzieki tej funkcji mozna zdefiniowac imie kazdemu
 *		    livingowi.
 * Argumenty	  : *imie - Tablica z odmiana imienia.
 *		    rodzaj - Rodzaj gramatyczny imienia.
 * Funkcja zwraca : int - 1 jesli imie zostalo nadane, 0 w przeciwnym wypadku.
 */
public int
ustaw_imie(string *imie, int rodzaj = PL_MESKI_NOS_ZYW)
{
    int x = 6;

    if (sizeof(imie) != 6)
        return 0;
    
    while (--x >= 0)
        ::set_name(imie[x], x, rodzaj);
        
    return 1;
}

/*
 * Function name: query_linkdead
 * Description  : Return the linkdeath-status of the player.
 * Returns      : int - if false, the player is not linkdead. Else it
 *                      returns the time the player linkdied.
 */
public int
query_linkdead()
{
    return is_linkdead;
}

/*
 * Function name: query_humanoid
 * Description  : Tells whether we are humanoid or not. By default, all
 *                livings are marked as humanoid and then in /std/creature
 *                we unmark them as such by masking this function.
 * Returns      : int - 1.
 */
public int
query_humanoid()
{
    return 1;
}

/*
 * Function name: query_met
 * Description  : Tells if we know a certain living's name. As NPC's always
 *                know everyone, this function is masked in the player object
 *                for true met/nonmet behaviour for players (if defined).
 * Arguments    : mixed name - the name or objectpointer to the living we
 *                             are supposed to have met.
 * Returns      : int 1 - always as NPC's always know everyone.
 */
public int
query_met(mixed name, int dummy = 0)
{
    return 1;
}

/*
 * Function name:   notmet_me
 * Description:     Finds out if obj is considered to have met me. Players
 *                  must have been introduced to me. All others don't have to
 *                  be introduced to know me.
 * Arguments:       obj: Object in question, has it met me?
 * Returns:         True if object has not met me.
 */
public int
notmet_me(object obj)
{
#ifdef MET_ACTIVE
    if (obj && query_ip_number(obj))
    {
	return !obj->query_met(this_object());
    }
#else
    return !this_player()->query_met(this_object());
#endif MET_ACTIVE
}

/*
 * Function name:   query_real_name
 * Description:     Returns the lowercase name of this living.
 *                  E.g.: "fatty".
 * Argumenty:	    przyp - Przypadek. Mozna podac, nie trzeba.
 * Returns:         The name
 */
public varargs string
query_real_name(int przyp)
{
    return lower_case(::query_name(0, przyp));
}

/*
 * Function name:   query_name
 * Description:     Returns the capitalized name of this living.
 *                  E.g.: "Fatty".
 * Returns:         The name
 */
public varargs string
query_name(int przyp)
{
    return capitalize(::query_nazwa(przyp));
}

/*
 * Function name:   query_met_name
 * Description:     Returns the name of this living, or "ghost of name" if
 *                  this living is not quite so living. E.g.: "Fatty".
 * Returns:         The name
 */
public string
query_met_name(int przyp = 0)
{
    if (this_object()->query_ghost())
    {
        switch(przyp)
        {
            case 0: return "duch " + query_name(1);
            case 1: return "ducha " + query_name(1);
            case 2: return "duchowi " + query_name(1);
            case 3: return "ducha " + query_name(1);
            case 4: return "duchem " + query_name(1);
            case 5: return "duchu " + query_name(1);
            default: return "duch " + query_name(1);
        }
    }
    return query_name(przyp);
}


/*
 * Nazwa funkcji : query_nonmet_name
 * Opis          : Zwraca opis tego livinga w liczbie pojedynczej, 
 *		   w formie przeznaczonej dla osob, ktore go nie znaja, 
 *		   np. "niebieskooka zgrabna elfka". Jesli living nie ma 
 *		   ustawionych shortow, zostana one utworzone na podstawie 
 *		   dwoch pierwszych przymiotnikow oraz rasy.
 * Argumenty     : int przyp - przypadek, w jakim ma zostac podany opis.
 * Funkcja zwraca: string - opis livinga przeznaczony dla osob nie znajacych
 *			    go, w liczbie pojedynczej.
 */
public varargs string
query_nonmet_name(int przyp)
{
    string *adj, str;
    string gender;

    /* Espcially true for NPC's. If a short has been set, use it. */
    if (strlen(query_short(przyp)))
    {
	return ::short(przyp);
    }

    if (sizeof((adj = this_object()->query_przym(1, przyp, 
            this_object()->query_rodzaj_rasy() + 1 ))) > 0)
    {
//	str = implode(adj[..1], " ") + " "; // Bylo ograniczenie na dwa przymniotniki
	str = implode(adj, " ") + " ";
    }
    else str = "";
    
    if (query_ghost())
    {
	switch(przyp)
	{
	    case PL_MIA: str += "duch"; break;
	    case PL_DOP: str += "ducha"; break;
	    case PL_CEL: str += "duchowi"; break;
	    case PL_BIE: str += "ducha"; break;
	    case PL_NAR: str += "duchem"; break;
	    case PL_MIE: str += "duchu"; break;
	}
        
        if (!this_object()->query_rasa())
            return str;
        
	str += " ";
	przyp = PL_DOP;
    }

    if (this_object()->query_osobno())
    {
        switch(this_object()->query_gender())
        {
            case G_MALE:
                 switch(przyp)
                 {
                     case PL_MIA: str += "mezczyzna"; break;
                     case PL_DOP: str += "mezczyzny"; break;
                     case PL_CEL: str += "mezczyznie"; break;
                     case PL_BIE: str += "mezczyzne"; break;
                     case PL_NAR: str += "mezczyzna"; break;
                     case PL_MIE: str += "mezczyznie"; break;
                 }
                 break;
            case G_FEMALE:
                 switch(przyp)
                 {
                     case PL_MIA: str += "kobieta"; break;
                     case PL_DOP: str += "kobiety"; break;
                     case PL_CEL: str += "kobiecie"; break;
                     case PL_BIE: str += "kobiete"; break;
                     case PL_NAR: str += "kobieta"; break;
                     case PL_MIE: str += "kobiecie"; break;
                 }
                 break;
            case G_NEUTER:
                 switch(przyp)
                 {
                     case PL_MIA: str += "obojniak"; break;
                     case PL_DOP: str += "obojniaka"; break;
                     case PL_CEL: str += "obojniakowi"; break;
                     case PL_BIE: str += "obojniaka"; break;
                     case PL_NAR: str += "obojniakiem"; break;
                     case PL_MIE: str += "obojniaku"; break;
                 }
                 break;
        }
    }
    else
    {
	str += this_object()->query_rasa(przyp);
    }
    
    if (query_wiz_level())
        switch(przyp)
        {
            case PL_MIA: return str + " czarodziej";
            case PL_DOP: return str + " czarodzieja";
            case PL_CEL: return str + " czarodziejowi";
            case PL_BIE: return str + " czarodzieja";
            case PL_NAR: return str + " czarodziejem";
            case PL_MIE: return str + " czarodzieju";
        }

    return str;
}

/*
 * Nazwa funkcji : query_nonmet_pname
 * Opis          : Zwraca opis tego livinga w liczbie mnogiej, 
 *		   w formie przeznaczonej dla osob, ktore go nie znaja, 
 *		   np. "niebieskookie zgrabne elfki". Jesli living nie ma 
 *		   ustawionych shortow, zostana one utworzone na podstawie 
 *		   dwoch pierwszych przymiotnikow oraz rasy. U livingow nie 
 *		   humanoidalnych nie pojawi sie plec. U humanoidalnych moze 
 *		   to zostac osiagniete poprzez dodanie propa 
 *		   LIVE_I_NO_GENDER_DESC.
 * Argumenty     : int przyp - przypadek, w jakim ma zostac podany opis.
 * Funkcja zwraca: string - opis livinga przeznaczony dla osob nie znajacych
 *			    go, w liczbie mnogiej.
 */
public varargs string
query_nonmet_pname(int przyp)
{
    string *adj, str;
    string gender;

    /* Espcially true for NPC's. If a short has been set, use it. */
    if (strlen(query_plural_short(przyp)))
    {
	return ::plural_short(przyp);
    }

    if (sizeof((adj = this_object()->query_pprzym(1, przyp, 
            this_object()->query_rodzaj_rasy() + 1 ))) > 0)
    {
	str = implode(adj[..1], " ") + " ";
    }
    else str = "";
    
    if (query_ghost())
    {
	switch(przyp)
	{
	    case PL_MIA: str += "duchy"; break;
	    case PL_DOP: str += "duchow"; break;
	    case PL_CEL: str += "duchom"; break;
	    case PL_BIE: str += "duchy"; break;
	    case PL_NAR: str += "duchami"; break;
	    case PL_MIE: str += "duchach"; break;
	}
        
        if (!this_object()->query_prasa())
            return str;
        
	str += " ";
	przyp = PL_DOP;
    }

    if (this_object()->query_osobno())
    {
        switch(this_object()->query_gender())
        {
            case G_MALE:
                 switch(przyp)
                 {
                     case PL_MIA: str += "mezczyzni"; break;
                     case PL_DOP: str += "mezczyzn"; break;
                     case PL_CEL: str += "mezczyznom"; break;
                     case PL_BIE: str += "mezczyzn"; break;
                     case PL_NAR: str += "mezczyznami"; break;
                     case PL_MIE: str += "mezczyznach"; break;
                 }
                 break;
            case G_FEMALE:
                 switch(przyp)
                 {
                     case PL_MIA: str += "kobiety"; break;
                     case PL_DOP: str += "kobiet"; break;
                     case PL_CEL: str += "kobietom"; break;
                     case PL_BIE: str += "kobiety"; break;
                     case PL_NAR: str += "kobietami"; break;
                     case PL_MIE: str += "kobietach"; break;
                 }
                 break;
            case G_NEUTER:
                 switch(przyp)
                 {
                     case PL_MIA: str += "obojniaki"; break;
                     case PL_DOP: str += "obojniakow"; break;
                     case PL_CEL: str += "obojniakom"; break;
                     case PL_BIE: str += "obojniakow"; break;
                     case PL_NAR: str += "obojniakami"; break;
                     case PL_MIE: str += "obojniakach"; break;
                 }
                 break;
        }
    }
    else
    {
	str += this_object()->query_prasa(przyp);
    }

    if (query_wiz_level())
        switch(przyp)
        {
            case PL_MIA: return str + " czarodzieje";
            case PL_DOP: return str + " czarodziejow";
            case PL_CEL: return str + " czarodziejom";
            case PL_BIE: return str + " czarodziejow";
            case PL_NAR: return str + " czarodziejami";
            case PL_MIE: return str + " czarodziejach";
        }
        
    return str;
}

/*
 * Function name:   query_Met_name
 * Description:     Returns the capitalized name of this living, prepended
 *                  with "Ghost of" if the living is not that living at all.
 *                  E.g.: "Ghost of Fatty".
 * Returns:         The capitalized name of the living when met.
 */
public varargs string
query_Met_name(int przyp)
{
    return capitalize(query_met_name(przyp));
}

/*
 * Nazwa funkcji : query_imie
 * Opis          : Zwraca opis osobnika widzianego przez kogos innego
 *		   (domyslnie previous_object(-1)). Funkcja sprawdza
 *		   czy osoba, dla ktorej sprawdzamy zna this_object(),
 *		   oraz czy go widzi.
 * Argumenty     : mixed pobj - Obiekt, ktory ma 'widziec' this_object().
 *			Domyslnie previous_object(-1);
 *		   mixed przyp - Przypadek gramatyczny, w ktorym
 *			obiekt ma nas widziec.
 * Funkcja zwraca: Krotki opis badz imie this_player(). (Patrz wyzej)
 */
public string
query_imie(mixed pobj = 0, mixed przyp = 0)
{
    string pre, aft;
    
    pre = ""; aft = "";
    
    if (!objectp(pobj))
    {
        przyp = pobj;
        pobj = previous_object(-1);
    }
    if (!intp(przyp))
    {
        if (stringp(przyp))
            przyp = atoi(przyp);
        else
            przyp = 0;
    }
    
    if (!CAN_SEE(pobj, this_object()) || !CAN_SEE_IN_ROOM(pobj))
        switch(przyp)
        {
            case PL_MIA: return "ktos";
            case PL_DOP: return "kogos";
            case PL_CEL: return "komus";
            case PL_BIE: return "kogos";
            case PL_NAR: return "kims";
            case PL_MIE: return "kims";
        }

    if (query_prop(OBJ_I_INVIS))
    {
	pre = "(";
	aft = ")";
    }
    else if (query_prop(OBJ_I_HIDE))
    {
	pre = "[";
	aft = "]";
    }

#ifdef MET_ACTIVE
    if (notmet_me(pobj))
	return pre + this_object()->query_nonmet_name(przyp) + aft;
    else
#endif
	return pre + this_object()->query_met_name(przyp) + aft;
}

public string
query_Imie(mixed pobj = 0, int przyp = 0)
{
    return capitalize(query_imie(pobj, przyp));
}

/*
 * Function name:   query_art_name
 * Description:	    Gives the name with a prefix article when the object
 *                  has not met this living.
 *                  E.g.:
 *                        when met:   "Fatty",
 *                        when unmet: "a big fat gnome wizard".
 * Arguments:       pobj: The object that wants to know the name.
 *                  If pobj is undefined, it is assumed that the call has
 *                  been done through a protected vbfc
 * Returns:         Name prefixed with article.
 */
public varargs string
query_art_name(mixed pobj, int przyp)
{
    return query_imie(pobj, przyp);
}

/*
 * Function name:   query_Art_name
 * Description:	    Gives the name with a capitalized prefix article when the
 *                  calling object has not met this living.
 *                  E.g.:
 *                        when met:   "Fatty",
 *                        when unmet: "A big fat gnome wizard".
 * Arguments:       pobj: The object that wants to know the name.
 *                  If pobj is undefined, it is assumed that the call has
 *                  been done through a protected vbfc
 * Returns:         Name prefixed with capitalized article.
 */
public varargs string
query_Art_name(mixed pobj, int przyp)
{
    return capitalize(query_art_name(pobj, przyp));
}

/*
 * Function name:   query_the_name
 * Description:	    Gives the name preceded by "the" when the object
 *                  has not met this living.
 *                  E.g.:
 *                        when met:   "Fatty",
 *                        when unmet: "the big fat gnome wizard".
 * Arguments:       pobj: The object that wants to know the name.
 *                  If pobj is undefined, it is assumed that the call has
 *                  been done through a protected vbfc
 * Returns:         Name prefixed with "the".
 */
public varargs string
query_the_name(mixed pobj, int przyp)
{
    return query_art_name(pobj, przyp);
}

/*
 * Function name:   query_The_name
 * Description:	    Gives the name preceded by "The" when the calling object
 *                  has not met this living.
 *                  E.g.:
 *                        when met:   "Fatty",
 *                        when unmet: "The big fat gnome wizard".
 * Argument:        pobj: The object that wants to know the name.
 *                  If pobj is undefined, it is assumed that the call has
 *                  been done through a protected vbfc
 * Returns:         Name prefixed with "The".
 */
public varargs string
query_The_name(object pobj)
{
    return capitalize(query_the_name(pobj));
}

/*
 * Nazwa funkcji : query_wolacz
 * Opis          : Znajduje wolacz liczby pojedynczej imienia istoty.
 * Funkcja zwraca: string - szukany wolacz.
 */
public string
query_wolacz()
{
    return LANG_FILE->wolacz(this_object()->query_name(PL_MIA),
                             this_object()->query_name(PL_MIE));
}

/*
 * Function name: query_exp_title
 * Description  : Returns "wizard" if this living is a wizard, or else
 *                tries to calculate a title from the stats
 * Returns      : string - the title.
 */
public string
query_exp_title()
{
    int a, s;

    if (query_wiz_level())
    {
	return "czarodziej";
    }

    s = sizeof(SD_AV_TITLES);
    a = ((this_object()->query_average_stat() * s) / 100);
    a = ((a >= s) ? (s - 1) : a);

    return SD_AV_TITLES[a];
}

/*
 * Function:    query_presentation
 * Description: Gives a presentation of the living in one line. Including
 *              Name, Race, Guild titles, Alignment and Experience level
 *              This should only be displayed to met players.
 *              E.g.: "Fatty the donut-fan, wizard, male gnome (eating)"
 * Returns:     The presentation string
 */
public string
query_presentation()
{
    string a, b, c;
    
    a = query_title(); 
//    b = this_object()->query_exp_title(); 
#ifndef NO_ALIGN_TITLE
    c = this_object()->query_al_title();
#endif

    return query_name() +
	(strlen(a) ? (" " + a + ",") : ",") +
	/* (strlen(b) ? (" " + b + ",") : "") + */ " " +
	this_object()->query_rasa(PL_MIA)
#ifndef NO_ALIGN_TITLE
	+ (strlen(c) ? (" (" + c + ")") : "")
#endif
	; /* Oke, it is ugly to have a semi-colon on a separate line. */
}

/*
 * Function name:   short
 * Description:     Returns the short-description of this living, for the
 *                  object given. Handles invisibility and met/nonmet.
 * Returns:         The short string.
 */
public varargs string
short(mixed for_obj, mixed przyp)
{
    string desc;
    string extra;

    if (!objectp(for_obj))
    {
        if (intp(for_obj))
            przyp = for_obj;
        else if (stringp(for_obj))
            przyp = atoi(for_obj);
        
        for_obj = this_player();
    }
    else
        if (stringp(przyp))
            przyp = atoi(przyp);
            
     if (przyp || !strlen(extra = query_prop(LIVE_S_EXTRA_SHORT)))
         extra = "";
        
    /*
     * If a specific short is set with set_short, use that description
     */
    if (strlen(query_short(przyp)))
	return ::short(for_obj, przyp) + extra;

#ifdef STATUE_WHEN_LINKDEAD
    if (is_linkdead)
    {
        switch(przyp)
        {
            case PL_MIA:
            case PL_NAR: desc = "statua "; break;
            case PL_DOP:
            case PL_CEL:
            case PL_MIE: desc = "statui "; break;
            case PL_BIE: desc = "statue ";
        }
        przyp = PL_DOP;
    }
    else
#endif
    desc = "";

#ifdef MET_ACTIVE
    if (notmet_me(for_obj))
        desc += this_object()->query_nonmet_name(przyp);
    else
#endif
    desc += this_object()->query_met_name(przyp);

    return desc + extra;
}

public varargs string
plural_short(mixed for_obj, int przyp)
{
    string desc;

    if (!objectp(for_obj))
    {
        if (intp(for_obj))
            przyp = for_obj;
        else if (stringp(for_obj))
            przyp = atoi(for_obj);
        
        for_obj = this_player();
    }
    else
        if (stringp(przyp))
            przyp = atoi(przyp);

    if (strlen(query_plural_short(przyp)))
	return ::plural_short(for_obj, przyp);

#ifdef STATUE_WHEN_LINKDEAD
    if (is_linkdead)
    {
        switch(przyp)
        {
            case PL_MIA: desc = "statuy "; break;
            case PL_DOP:
            case PL_CEL:
            case PL_BIE:
            case PL_NAR: 
            case PL_MIE: desc = "statui ";
        }
        przyp = PL_DOP;
    }
    else
#endif
    desc = "";

#ifdef MET_ACTIVE
    if (notmet_me(for_obj))
        desc += this_object()->query_nonmet_pname(przyp);
    else
#endif
    desc += this_object()->query_met_name(przyp);

/*    
if (strlen(extra = query_prop(LIVE_S_EXTRA_SHORT)))
    {
	return (desc + extra);
    }
*/

    return desc;
}

/*
 * Function name: vbfc_short
 * Description:   Gives short as seen by previous object
 * Returns:	  string holding short()
 */
public varargs string
vbfc_short(int przyp)
{
    object for_obj;

    for_obj = previous_object(-1);
    if (!this_object()->check_seen(for_obj))
        switch(przyp)
        {
            case PL_MIA: return "ktos";
            case PL_DOP: return "kogos";
            case PL_CEL: return "komus";
            case PL_BIE: return "kogos";
            case PL_NAR: return "kims";
            case PL_MIE: return "kims";
        }

    return this_object()->short(for_obj, przyp);
}

/*
 * Function name:   long
 * Description:     Returns the long-description of this living, and shows also
 *                  the inventory. Handles invisibility and met/nonmet. Note
 *                  that the function does not do any writes! It only returns
 *                  the correct string.
 * Arguments:       string for_obj: return an item-description
 *                  object for_obj: return the living-description
 * Returns:         The description string
 */
public varargs string
long(mixed for_obj)
{
    string          cap_pronoun, res;
    object 	    eob;
    int		    a, s;

    if (stringp(for_obj))   /* Items */
	return ::long(for_obj);

    if (!objectp(for_obj))
	for_obj = this_player();

    if (for_obj == this_object())
    {
        res = "Jestes " + this_object()->query_nonmet_name(PL_NAR) +
            ", znan" + koncowka("ym", "a") + " jako:\n" +
            this_object()->query_presentation() + ".\n";
    }
    else
    {		
	if (!strlen(res = query_long()))
	{
	    if (!notmet_me(for_obj))
	    {
		res = "Jest " + this_object()->query_nonmet_name(PL_NAR) +
		    ", znan" + koncowka("ym", "a") + " jako:\n" +
		    this_object()->query_presentation() + ".\n";
	    }
	    else if (!(this_object()->short(for_obj)))
		return "";
	    else
		res = "Jest to " + this_object()->short(for_obj, PL_MIA) + 
		    ".\n";

	    if (this_object()->query_ghost())
	    {
		res = "Jest to ";
#ifdef MET_ACTIVE
		if (notmet_me(for_obj))
		    return res + this_object()->query_nonmet_name(PL_MIA) +
		        ".\n";
		else
#endif
		    return res + this_object()->query_name() + ".\n";
	    }
	}
	else
	{
	    res = check_call(res);
	}
    }
    
    if (/*this_object()->query_humanoid() */
	!this_object()->query_npc() && !this_object()->query_wiz_level() &&
	!this_object()->notmet_me(for_obj))
    {
        if (for_obj == this_object())
            res += "Wygladasz na ";
        else
            res += "Wyglada na ";

        s = sizeof(SD_AVG_TITLES);
        a = ((this_object()->query_average_stat() * s) / 100);
        a = ((a >= s) ? (s - 1) : a);

        res += SD_AVG_TITLES[a] + ".\n";
    }

    return res + this_object()->show_sublocs(for_obj);
}

private string
z_ze(string str)
{
    int ix;

    switch(lower_case(str[0]))
    {
	case 's':
	case 'S':
	    if (str[1] == 'z')
		ix = 2;
	    else
		ix = 1;
	    break;
	case 'z':
	case 'Z':
	    ix = 1;
	    break;
	default: return "z " + str;
    }
    
    if (member_array(str[ix], ({ 'a', 'e', 'i', 'o', 'u', 'y' }) ) == -1)
	return "ze " + str;
    else
	return "z " + str;
}

/*
 * Function name: describe_combat
 * Description  : This function describes the combat that is going on in
 *                the room this_object() is in. It is an internal function
 *                and should only be called internally by do_glance.
 * Arguments    : object *livings - the livings in the room.
 */
static void
describe_combat(object *livings)
{
    int     index;
    int     size;
    string  text = "";
    object  victim;
    mapping fights = ([ ]);
    mixed tmp_alv;

    /* Sanity check. No need to print anything if there aren't enough
     * people to actually fight. Note that if there is only one living, it
     * is possible that we fight that living.
     */
    if ((size = sizeof(livings)) < 1)
    {
	return;
    }

    /* First compile a mapping of all combats going on in the room.
     * The format is the victim as index and an array of all those
     * beating on the poor soul as value. Add this_object() to the
     * list since it isn't in there yet.
     */
    livings += ({ this_object() });
    size++;
    index = -1;
    while(++index < size)
    {
	/* Only if the living is actually fighting. */
	if (objectp(victim = livings[index]->query_attack()))
	{
	    if (pointerp(fights[victim]))
	    {
		fights[victim] += ({ livings[index] });
	    }
	    else
	    {
		fights[victim] = ({ livings[index] });
	    }
	}
    }

    /* No combat going on. */
    if (!m_sizeof(fights))
    {
	return;
    }
    
    /* First we describe the combat of the player him/herself. This will
     * be a nice compound message. Start with 'outgoing' combat.
     */
    if (objectp(victim = this_object()->query_attack()))
    {
	fights[victim] -= ({ this_object() });

	/* Victim is fighting back. */
	if (victim->query_attack() == this_object())
	{
	    text = "walczysz " + z_ze(victim->query_imie(this_object(), 
	        PL_NAR));
	    
	    if (fights[this_object()])
		fights[this_object()] -= ({ victim });
	}
	else
	{
	    text = "koncentrujesz sie na walce " + 
	        z_ze(victim->query_imie(this_object(), PL_NAR));
	}

	/* Other people helping us attacking the same target. */
	if (sizeof(fights[victim]))
	{
	    text = "Wraz " + z_ze(FO_COMPOSITE_LIVE(fights[victim],
	        this_object(), PL_NAR)) + ", " + text;
	}
	else
	    text = capitalize(text);

	fights = m_delete(fights, victim);

	/* Other people hitting on me. */
	if (index = sizeof(fights[this_object()]))
	{
	    text += ", wspart" + victim->koncowka("ym", "a") + " przez " +
	        FO_COMPOSITE_LIVE(fights[this_object()], this_object(),
	        PL_BIE);
	}

	text += ".\n";
    }
    /* If we aren't fighting, someone or something may be fighting us. */
    else if (index = sizeof(fights[this_object()]))
    {
        text = capitalize(FO_COMPOSITE_LIVE(fights[this_object()],
            this_object(), PL_MIA) + " koncentruj" +
            ((index == 1) ? "e" : "a") + " sie na walce z toba.\n");
    }

    /* Now generate messages about the other combat going on. This will
     * not be as sophisticated as the personal combat, but it will try to
     * to circumvent printing two lines of 'a fights b' and 'b fights a'
     * since I think that is a silly way of putting things.
     */
    fights = m_delete(fights, this_object());
    livings = m_indices(fights);
    size = sizeof(livings);
    index = -1;
    while(++index < size)
    {
	if (!fights[livings[index]])
	    continue;
              
	/* Victim is fighting (one of his) attackers. */
	if (objectp(victim = livings[index]->query_attack()) &&
	    (member_array(victim, fights[livings[index]]) >= 0))
	{
	    fights[livings[index]] -= ({ victim });
	    
	    /* Walka z victim zostala opisana juz w poprzednim ustepie,
	     * teraz tylko wyszczegolniamy kogo victim atakuje */
	    if (!fights[victim])
	    {
		text += victim->query_Imie(this_object(), PL_MIA) +
		    " koncentruje sie na walce " + 
		    z_ze(livings[index]->query_imie(this_object(), PL_NAR)) + 
		    ".\n";
	    }
	    else
	    {
	        fights[victim] -= ({ livings[index] });
	    
		/* Start with the the name of one of the fighters. */
		text += livings[index]->query_Imie(this_object(), PL_MIA);
		
		/* Then the people helping the first combatant. */
		if (sizeof(fights[victim]))
		{
		    text += ", wraz " + z_ze(FO_COMPOSITE_LIVE(fights[victim], 
			this_object(), PL_NAR));
		}
		
		/* Then the second living in the fight. */
		text += " walczy " + z_ze(victim->query_imie(this_object(),
		    PL_NAR));
		
		/* And the helpers on the other side. */
		if (sizeof(fights[livings[index]]))
		{
		    text += ", wspart" + victim->koncowka("ym", "a")
			+ " przez " + FO_COMPOSITE_LIVE(fights[livings[index]],
			this_object(), PL_BIE);
		}
		
		text += ".\n";
	    }
	}
	else
	{
	    text += capitalize(FO_COMPOSITE_LIVE(fights[livings[index]],
		this_object(), PL_MIA)) + " koncentruj" + 
		((sizeof(fights[livings[index]]) == 1) ? "e" : "a") +
		" sie na walce " + 
		z_ze(livings[index]->query_imie(this_object(), PL_NAR)) + 
		".\n";
	}

	fights = m_delete(fights, livings[index]);
	fights = m_delete(fights, victim);
    }

    write(text);
}
 
/*
 * Function name: do_glance
 * Description  : This is the routine describing rooms to livings. It will
 *                print the long or short description of the room, the
 *                possible exits, all visible non-living and living objects
 *                in the room (but not this_object() itself) and then it
 *                will print possible attacks going on. Note that this
 *                function performs a write(), so this_player() will get the
 *                descriptions printed to him/her.
 * Arguments    : int brief - if true, write the short-description,
 *                            else write the long-description.
 * Returns      : int 1 - always.
 */
public int
do_glance(int brief)
{
    object env;
    object *ob_list;
    object *lv;
    object *dd;
    string item;

    /* Don't waste the long description on NPC's. */
    if (!interactive(this_object()))
    {
	return 0;
    }

    /* Wizard gets to see the filename of the room we enter and a flag if
     * there is WIZINFO in the room.
     */
    env = environment();
    if (query_wiz_level())
    {
	if (stringp(env->query_prop(OBJ_S_WIZINFO)))
	{
	    write("Wizinfo ");
	}

	write(file_name(env) + "\n");
    }

    /* It is dark. */
    if (!CAN_SEE_IN_ROOM(this_object()))
    {
 	if (!stringp(item = env->query_prop(ROOM_S_DARK_LONG)))
 	    write(LD_DARK_LONG);
 	else
 	    write(item);
	return 1;
    }

    /* Describe the room and its contents. */
#ifdef DAY_AND_NIGHT
    if (!env->query_prop(ROOM_I_INSIDE) && 
	((HOUR > 21) ||
	 (HOUR < 5)) &&
	((env->query_prop(OBJ_I_LIGHT) +
	 query_prop(LIVE_I_SEE_DARK)) < 2))
    {
	write(LD_IS_NIGHT(env));
    }
    else
#endif
    {
	if (brief)
  	{
	    write(capitalize(env->short()) + ".\n");

	    if (!env->query_noshow_obvious())
	    {
		write(env->exits_description());
	    }
	}
	else
	{
	    write(env->long());
	}
    }

    ob_list = all_inventory(env) - ({ this_object() });
    lv = FILTER_LIVE(ob_list);
    dd = FILTER_SHOWN(ob_list - lv);

    item = COMPOSITE_FILE->desc_dead(dd, 0, 1);
    if (stringp(item))
    {
	write(capitalize(item) + ".\n");
    }
    item = COMPOSITE_FILE->desc_live(lv, PL_MIA, 1);
    if (stringp(item))
    {
	write(capitalize(item) + ".\n");
    }

    /* Give a nice description of the combat that is going on. */
    describe_combat(lv);

    return 1;
}

/*
 * Function name: appraise_object
 * Description:   This function is called when someone tries to appraise a
 *		  living object.
 * Arguments:     num - use this number instead of the skill.
 */
public void
appraise_object(int num)
{
    write("\n" + this_object()->long(this_player()) + "\n");
    write("Oceniasz, ze " + short(this_player(), PL_MIA) + " wazy " +
	appraise_weight(num) + ", zas " + koncowka("jego", "jej", "jego") + 
	" objetosc wynosi " + appraise_volume(num) + ".\n");
}

/*
 * Function name: query_align_text
 * Description:	  Returns the alignment text for this object.
 * Returns:	  The alignment text.
 */
string
query_align_text()
{
    int a, prc;
    string t, *names;

    a = this_object()->query_alignment();

    if (a < 0)
    {
	prc = (100 * (-a)) / (F_KILL_NEUTRAL_ALIGNMENT * 100);
	names = SD_EVIL_ALIGN;
    }
    else
    {
	prc = (100 * a) / (F_KILL_NEUTRAL_ALIGNMENT * 100);
	names = SD_GOOD_ALIGN;
    }

    a = (sizeof(names) * prc) / 100;

    if (a >= sizeof(names))
	a = sizeof(names) - 1;

    return names[a];
}
