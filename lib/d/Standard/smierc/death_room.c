inherit "/std/room";

#include <macros.h>
#include <std.h>
#include "smierci.h"

#pragma strict_types

mapping smierci = ([]);

public void akcja(object kto, int faza, string rasa, int sekwencja);
public int  komenda(string str);

public void
create_room()
{
    set_long("Pokoj Smierci.\n");
    
    setuid();
    seteuid(getuid());
    
    restore_object(SMIERCI_SAVE);
}

public void
init()
{
     add_action(&komenda(), "", 1);
     
     set_alarm(12.0, 0.0, &akcja(this_player(), 0));
}

public void
enter_inv(object ob, object from)
{
    ::enter_inv(ob, from);

    if (!living(ob))
    {
        ob->move(from);
        return ;
    }
    
    if ((!ob->query_ghost()) &&
        (SECURITY->query_wiz_rank(ob->query_real_name()) < WIZ_ARCH))
    {
        ob->catch_msg("Ty zyjesz! Smierc nie chce przyjac cie w swoje " +
           "objecia.\n");
        ob->move_living("X", from, 1, 1);
        return ;
    }
    
    return;
}

public void
akcja(object kto, int faza, string rasa = 0, int sekwencja = 0)
{
    mixed pole;
    int delay, x, len, begin, mid;
    string szukany, str1, str2;
    
    if (!objectp(kto))
        return ;
        
    if (!kto->query_ghost())
        return ;
        
    if (environment(kto) != this_object())
        return ;
        
    if (faza == 0)
    {
        rasa = MASTER_OB(kto);
        if (!sscanf(rasa, "/d/Standard/race/%s_std", rasa))
        {
            // To jednak nie gracz... 
            kto->remove_object();
        }
        
        if (!smierci[rasa])
        {
	    /* Nie mamy sekwencji smierci na dla tej rasy. Mowi sie
	     * trudno - ta postac szybciej wroci do swiata smiertelnych.
	     */
            kto->reincarnate();
            return ;
        }
        sekwencja = (kto->query_death_count() % sizeof(smierci[rasa]));
    }
    else
    {
        if (faza >= sizeof(smierci[rasa][sekwencja]))
        {
            kto->reincarnate();
            return ;
        }
    }
    
    pole = smierci[rasa][sekwencja][faza];
    
    delay = atoi(pole);
    if (delay)
    {
        set_alarm(itof(atoi(pole)), 0.0, &akcja(kto, ++faza, rasa, sekwencja));
        return ;
    }
    
    x = -1;
    len = strlen(pole);
    szukany = "[";
    
    while(++x < len)
    {
        if (pole[x..x] == szukany)
        {
            if (!begin)
            {
                begin = x;
                szukany = "]";
            }
            else
            {
                if (mid)
                {
                    str1 = pole[begin+1..mid-1];
                    str2 = pole[mid+1..x-1];
                    
                    mid = kto->koncowka(0, 1);
                    
                    pole = pole[0..begin-1] + (mid ? str2 : str1) + 
                        pole[x+1..];
                    len = strlen(pole);
                    x -= (3 + strlen((mid ? str1 : str2)));
                }
                
                szukany = "[";
                begin = 0; mid = 0;
            }
            
        }
        else if (pole[x..x] == "/")
            mid = x;
    }

    
    
    kto->catch_msg(pole + "\n\n");
    
    akcja(kto, ++faza, rasa, sekwencja);
}

public int
komenda(string str)
{
    if (this_player()->query_wiz_level())
        return 0;
 
    switch(query_verb())
    {
        case "save":
        case "home":
        case "goto":
        case "nagraj":
        case "quit":
        case "zakoncz":
        case "zglos":
        case "bug":
        case "praise":
        case "idea":	return 0;
        case "?": this_object()->pomoc(str); return 1;
	default: write("Wisisz w pustce, nicosci. Tutaj czas nie ma " +
		       "miejsca, jak mogl" +
		       this_player()->koncowka("bys", "abys") +
		       " wiec cos uczynic?\n"); return 1;
    }
    
    return 0;
}
