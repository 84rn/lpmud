/* An object for setting player attributes. Each player has 2 attributes
 * that are settable from a number of categories. A player may only have
 * one attribute from each category.
 * /Tintin 911002
 */
inherit "/std/object";

#include <pl.h>
#include <living_desc.h>
#include <const.h>
#include "attributes.h"
#include "login.h"

#define PATH "/d/Standard/login/"

public int is_equal(string a, string b);

/* Attrib contains one element for each category of attributes. Each element
 * conatins an array with 3 parts, category name, category type and an array
 * of category members.
 */
mapping attributes;
string *kategorie;
int size;

create_object()
{
    set_name( ({"selector"}) );
    setuid();
    seteuid(getuid(this_object()));
//    restore_object(PATH + "attributes");
    attributes = L_ATTRIBUTES;
    kategorie = m_indices(attributes);
    size = sizeof(kategorie);
}

int
list(string str, string rasa = "elf", int plec = 0)
{
    string *tmp = ({}), x;
    int i, j, typ;
    
    plec = (plec ? PL_ZENSKI : PL_MESKI_OS);
    typ = L_RACE2L_MAP[rasa] & (plec ? L_FEMALE : L_MALE);

    if (!str)
    {
        i = -1;
        while (++i < size)
	{
	    for (j = 0; j < sizeof(attributes[kategorie[i]]); j++)
	        if (typ & attributes[kategorie[i]][j][2])
	            tmp += ({ oblicz_przym(attributes[kategorie[i]][j][0], 
	                attributes[kategorie[i]][j][1], 0, plec, 0) });
	    if (sizeof(tmp))
                write("Kategoria: " + kategorie[i] + "\n" + 
                      implode(tmp, ", ") + "\n");
	}
	return 1;
    }

    str = lower_case(str);
    if (str == "kategorie")
    {
        i = -1;
	while (++i < size)
	    write(capitalize(kategorie[i]) + "  ");
	write("\n");
	return 1;
    }

    if (attributes[str])
    {
        i = sizeof(attributes[str]);
        while (--i >= 0)
            if (typ & attributes[str][i][2])
                tmp += ({ oblicz_przym(attributes[str][i][0], 
                    attributes[str][i][1], 0, plec, 0) });
                    
        x = (ODMIANA_RASY_OSOBNO[rasa] ? 
             LD_HUM_GENDER_MAP[(plec == PL_MESKI_OS) ? 0 : 1][PL_DOP] + " " 
             : "") + ODMIANA_RASY[rasa][(plec == PL_MESKI_OS) ? 0 : 1][PL_DOP];
                    
        if (sizeof(tmp))
            write("Cechami charakterystycznymi dla " + x + " w kategorii '" + 
                str + "' sa:\n" +
                implode(tmp, ", ") + ".\n");
        else
            write("W kategorii '" + str + "' nie widzisz " +
                "zadnych cech charakterystycznych dla " + x + ".\n");
                
    }
    return 1;
}

mixed
select(string str, string rasa, int plec = 0)
{
    int i, j, typ;
    string p1, p2;
    
    plec = (plec ? PL_ZENSKI : PL_MESKI_OS);

    if (!str)
    {
	write("Musisz wybrac jakis atrybut.\n");
	return 0;
    }

    typ = L_RACE2L_MAP[rasa] & (plec ? L_FEMALE : L_MALE);
    
    i = -1;
    while (++i < size)
    {
        j = sizeof(attributes[kategorie[i]]);
        while (--j >= 0)
        {
            if (attributes[kategorie[i]][j][2] & typ)
            {
                p1 = attributes[kategorie[i]][j][0];
                p2 = attributes[kategorie[i]][j][1];
                
                if (sizeof(filter( 
                      ({ oblicz_przym(p1, p2, PL_MIA, plec, 0),
                         oblicz_przym(p1, p2, PL_BIE, plec, 0)
                      }), &is_equal(, str))))
                    return ({ p1, p2 });
            }
        }
    }
    
    write("Nieznany atrybut.\n");
    return 0;
}

public int
is_equal(string a, string b)
{
    if (a == b)
    {
	return 1;
    }
    return 0;
}
