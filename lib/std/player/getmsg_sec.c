/*
 * /std/player/getmsg_sec.c
 *
 * This is a subpart of player_sec.c
 *
 * All incoming messages to the player should come through here.
 * All non combat interaction with other players are also here.
 */

#include <config.h>
#include <flags.h>
#include <formulas.h>
#include <language.h>
#include <ss_types.h>
#include <stdproperties.h>

static mapping introduced_name = ([]);	/* People who introduced themselves */
public int     wiz_unmet;         /* If wizards want to be unmet */

void add_introduced(string imie_mia, string imie_bie);
mixed query_remembered(string imie = 0, int tylko_mianownik = 0);
mixed query_introduced(string imie = 0, int tylko_mianownik = 0);

/************************************************************************
 *
 * Introduction and met routines
 */

/*
 * Function name:   set_wiz_unmet
 * Description:     Marks if the wizard wants to see all as met or unmet
 * Arguments:       flag: 1 if see as unmet, 0 if see as met
 * Returns:         The current state    
 */
public int
set_wiz_unmet(int flag)
{
    wiz_unmet = flag;
    return wiz_unmet;
}

/*
 * Function name:   query_met
 * Description:     Tells if we know a certain living's name.
 * Arguments:       name: Name of living or objectp of living
 * Returns:         Jesli znamy, zwraca tablice:
 *		    ({ imie_w_mianowniku, imie_w_bierniku }), w przeciwnym
 *		    wypadku 0.
 */
/*
 * Nazwa funkcji : query_met
 * Opis          : Zwraca, czy znana jest nam postac o podanym imieniu.
 * Argumenty     : name - wskaznik na obiekt gracza, lub jego imie
 *			  w mianowniku lub bierniku.
 *		   dopuszczalny_biernik - jesli rowny 0, podanie imienia
 *			  gracza w bierniku nie prowadzi do rozpoznania.
 * Funkcja zwraca: 
 */
public mixed
query_met(mixed name, int dopuszczalny_biernik = 0)
{
    string str, *ret;
    mapping rem;

#ifndef MET_ACTIVE
    return 1;
#else
    if (objectp(name))
	str = (string) name->query_real_name();
    else if (stringp(name))
    {
       	str = name;
	name = find_living(name);
    }
    else
	return 0;

    if (name && name->query_prop(LIVE_I_NEVERKNOWN))
	return 0;

    if (name && name->query_prop(LIVE_I_ALWAYSKNOWN))
    {
        add_introduced(name->query_real_name(PL_MIA), 
            name->query_real_name(PL_BIE));
	return ({ name->query_real_name(PL_MIA), 
	          name->query_real_name(PL_BIE) });
    }

    /* Wizards know everyone */
    if (query_wiz_level())
	if (wiz_unmet == 0 || (wiz_unmet == 2 && name && !(name->query_npc())))
    	    return 1; /* Czarodzieje nie moga zapamietywac. */
	else
	    return 0; /* Unless they have said they don't want to. */

    if (str == query_real_name(PL_MIA)) /* I always know myself */
	return ({ query_real_name(PL_MIA), query_real_name(PL_BIE) });

//    rem = query_remember_name();
	
//    if (introduced_name[str] || (mappingp(rem) && rem[str]))

    if ((ret = query_introduced(str, !dopuszczalny_biernik)) ||
	(ret = query_remembered(str, !dopuszczalny_biernik)))
	return ret;
     
    /* Default case */
    return 0;
#endif MET_ACTIVE
}

/*
 * Function name:   add_introduced
 * Description:     Add the name of a living who has introduced herself to us
 * Argumenty:	    imie_mia: Imie przedstawionej osoby w mianowniku
 *		    imie_bie: Imie przedstawionej osoby w bierniku
 */
public void
add_introduced(string imie_mia, string imie_bie)
{
    introduced_name[imie_mia] = imie_bie;
}

/*
 * Nazwa funkcji:   query_introduced
 * Opis:	    Jesli nie podany argument zwraca mapping z ludzmi, ktorzy
 *		    sie przedstawili. Jesli podane jest konkretne imie, 
 *		    zwraca tablice z mianownikiem i biernikiem tego imienia.
 * Argument:	    imie - jesli podany, funkcja zwraca, czy przedstawila
 *			   nam sie dana osoba. Imie moze byc w mianowniku
 *			   lub bierniku.
 *		    tylko_mianownik - jesli != 0, funkcja zwroci 1 tylko gdy
 *			   podane imie bedzie zgodne z zapamietanym
 *			   mianownikiem imienia.
 * Funkcja zwraca:  Mapping z przedstawionymi ludzmi, albo tablica w formie:
 *		    ({ imie_w_mianowniku, imie_w_bierniku }),
 *		    gdy osoba o podanym imieniu figuruje w mappingu 
 *		    przedstawionych osob.
 */
public mixed
query_introduced(string imie = 0, int tylko_mianownik = 0)
{
    int indeks;
    string *tab, biernik;
    
    if (!imie)
        return introduced_name;
    
    if (biernik = introduced_name[imie])
        return ({ imie, biernik });
    else if (tylko_mianownik)
	return 0;
        
    if ((indeks = member_array(imie, (tab = m_values(introduced_name)))) != -1)
        return ({ m_indices(introduced_name)[indeks], tab[indeks] });
        
    return 0;
}

/*
 * Function name:   add_remembered
 * Description:     Adds a living to those whom we want to remember.
 *                  The living must exist in our list of those we have been
 *                  introduced to.
 * Argumenty:	    imie_mia: Imie zapamietywanej osoby w mianowniku 
 *			      lub bierniku.
 * Returns:         -1 if at limit for remember, 0 if not introduced, 
 *                  1 if remember ok, 2 if already known
 */
public varargs int
add_remembered(string imie)
{
    mapping tmp;
    int max;
    string *ret;

    if (query_remembered(imie))
	return 2; /* Already known */
	
    tmp = query_remember_name();
    
/*
    if (!query_met(imie) &&
		(!mappingp(introduced_name) || !introduced_name[str]))
*/
     if (!(ret = query_met(imie, 1)))
	return 0;
    
    max = F_MAX_REMEMBERED(query_stat(SS_INT), query_stat(SS_WIS));

    if(m_sizeof(tmp) >= max)
	return -1;

    introduced_name = m_delete(introduced_name, ret[0]);

    tmp[ret[0]] = ret[1];
	set_remember_name(tmp);
    
    return 1; /* Remember ok */
}

/*
 * Nazwa funkcji:   query_remembered
 * Opis:	    Jesli nie podany argument zwraca mapping z ludzmi, ktorych
 *		    pamietamy. Jesli podane jest konkretne imie, 
 *		    zwraca tablice z mianownikiem i biernikiem tego imienia.
 * Argument:	    imie - jesli podany, funkcja zwraca, czy pamietamy
 *			   dana osobe. Imie moze byc w mianowniku
 *			   lub bierniku.
 *		    tylko_mianownik - jesli != 0, funkcja zwroci 1 tylko gdy
 *			   podane imie bedzie zgodne z zapamietanym
 *			   mianownikiem imienia.
 * Funkcja zwraca:  Mapping z zapamietanymi ludzmi, albo tablica w formie:
 *		    ({ imie_w_mianowniku, imie_w_bierniku }),
 *		    gdy osoba o podanym imieniu figuruje w mappingu 
 *		    zapamietanych osob.
 */
public mixed
query_remembered(string imie = 0, int tylko_mianownik = 0)
{
    int indeks;
    string *tab, biernik;
    mapping zapamietani;

    zapamietani = query_remember_name();

    if (!imie)
        return zapamietani;
    
    if (biernik = zapamietani[imie])
        return ({ imie, biernik });
    else if (tylko_mianownik)
	return 0;
        
    if ((indeks = member_array(imie, (tab = m_values(zapamietani)))) != -1)
        return ({ m_indices(zapamietani)[indeks], tab[indeks] });
        
    return 0;
}

/*
 * Function name:   remove_remembered
 * Description:     Removes a remembered or introduced person from our list.
 * Argumenty:       imie: Imie osoby, ktorej imie zapominamy (w mianowniku
 *		    lub bierniku). 
 * Funkcja zwraca:  0 jesli nie byl przedsatwiony ani zapamietany, 
 *		    tablice w formie ({ imie_w_mian, imie_w_bierniku })
 *		    w przeciwnym wypadku.
 */
public mixed
remove_remembered(string imie)
{
    string  *pos, *pos2;

    imie = lower_case(imie);
    
    if (mappingp(introduced_name))
    {
        if (pos = query_introduced(imie))
	introduced_name = m_delete(introduced_name, pos[0]);
    }
    
    if (pos2 = query_remembered(imie))
        set_remember_name(m_delete(query_remember_name(), pos2[0]));
    
    if (pos2)
        pos = pos2;
    else 
    if(!pos)
        return 0;
        
    return ({ pos[0], pos[1] });
}

/*
 * Function name: catch_msg
 * Description:   This function is called for every normal message sent
 *                to this player.
 * Arguments:     msg:       Message to tell the player
 *                from_player: The object that generated the message
 *			     This is only valid if the message is on the
 *			     form ({ "met message", "unmet message" })
 */
public void 
catch_msg(mixed str, object from_player)
{
    /* A mortal cannot be busy. */
    if (query_wiz_level() &&
	(this_object()->query_prop(WIZARD_I_BUSY_LEVEL) & BUSY_F))
    {
	return;
    }

    ::catch_msg(str, from_player);
}

/*
 * Function name: catch_tell
 * Description  : All text printed to this living via either write() or
 *                tell_object() will end up here. Here we do the actual
 *                printing to the player in the form of a write_socket()
 *                that will send the message to the host.
 * Arguments    : string msg - the message to print.
 */
public void
catch_tell(string msg)
{
    write_socket(msg);
}
