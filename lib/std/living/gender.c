/*
    /std/living/gender.c

    This is a subpart of living.c
    All gender processing routines are defined here.

    This file is included into living.c
*/

#include "/sys/macros.h"
#include "/sys/const.h"
#include <living_desc.h>
#include <pl.h>


static nomask void
gender_reset()
{
    /* Illegal default value to force a change. 
    */
    this_object()->set_gender(-1); 
}

/*
 * Function name:   query_gender_string
 * Description:     Gives back a string that contains the gender of a living
 * Returns:         The string
 */
public string
query_gender_string()
{
    return LD_GENDER_MAP[query_gender()];
}

public varargs string
query_plec_string(int przyp)
{
    return LD_HUM_GENDER_MAP[query_gender()][przyp];
}

/*
 * Nazwa funkcji : query_living_rodzaj
 * Opis          : Funkcja zwraca rodzaj gramatyczny obiektu, w zaleznosci
 *		   od jego plci. Tak wiec dla plci zenskiej zwroci
 *		   PL_ZENSKI, dla ludzi mezczyzn PL_MESKI_OS, zas
 *		   dla nie-ludzi mezczyzn PL_MESKI_NOS.
 * Funkcja zwraca: Jeden z rodzajow gramatycznych.
 */
public int
query_living_rodzaj()
{
    int gender = this_object()->query_gender();
    
    switch(gender)
    {
        case G_FEMALE: return PL_ZENSKI;
        case G_NEUTER: return PL_NIJAKI_OS;
        case G_MALE: if (this_object()->query_rasa() == "czlowiek")
        		 return PL_MESKI_OS;
        default:
            return PL_MESKI_NOS_ZYW;
    }
}    

/*
 * Function name:   query_pronoun
 * Description:     Returns the pronoun that goes with the gender of this
 *                  living.
 * Returns:         "he", "she" or "it", depending on gender.
 */
public string
query_pronoun()
{
     return LD_PRONOUN_MAP[query_gender()]; 
}

/*
 * Function name:   query_possessive
 * Description:     Returns the possessive that goes with the gender of this
 *                  living.
 * Returns:         "his", "her" or "its", depending on gender.
 */
public string
query_possessive()
{
    return LD_POSSESSIVE_MAP[query_gender()];
}

/*
 * Function name:   query_objective
 * Description:     Returns the objective that goes with the gender of this
 *                  living.
 * Returns:         "him", "her" or "it", depending on gender.
 */
public string
query_objective()
{
    return LD_OBJECTIVE_MAP[query_gender()];
}

/*
 * Nazwa funkcji : add_gender_names
 * Opis          : Dodaje obiektowi nazwy plci, typu mezczyzna/samiec,
 *		   w zaleznosci od plci, rasyi humanoidalnosci.
 */
static void
add_gender_names()
{
    if (this_object()->query_humanoid())
    {
        if (query_gender() == G_FEMALE)
        {
            dodaj_nazwy( ({ "kobieta", "kobiety", "kobiecie", "kobiete",
                "kobieta", "kobiecie" }), ({ "kobiety", "kobiet",
                "kobietom", "kobiety", "kobietami", "kobietach" }),
                PL_ZENSKI);
        }
        else
        {
            dodaj_nazwy( ({ "mezczyzna", "mezczyzny", "mezczyznie", 
                "mezczyzne", "mezczyzna", "mezczyznie" }), 
                ({ "mezczyzni", "mezczyzn", "mezczyznom", 
                "mezczyzn", "mezczyznami", "mezczyznach" }), 
                PL_MESKI_OS);
        }
    }
    else
    {
        if (query_rodzaj_rasy() == PL_ZENSKI)
            dodaj_nazwy( ({ "samica", "samicy", "samicy", "samice",
                "samica", "samicy" }), ({ "samice", "samic", 
                "samicom", "samice", "samicami", "samicach" }), PL_ZENSKI);
        else
        {
            dodaj_nazwy( ({ "samiec", "samca", "samcowi", "samca", 
                "samcem", "samcu" }), ({ "samce", "samcow", "samcom",
                "samce", "samcami", "samcach" }), PL_MESKI_NOS_ZYW);
        }
    }
}
