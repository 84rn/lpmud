inherit "/std/room";

#include <macros.h>

#pragma strict_types

#define SMIERCI "/d/Standard/room/death/"

mapping smierci = ([]);

public void akcja(object kto, int faza, string rasa, int sekwencja);
public int  komenda(string str);
public void zaladuj_smierci();

public void
create_room()
{
    set_long("Pokoj Smierci.\n");
    
    setuid();
    seteuid(getuid());
    
    zaladuj_smierci();
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
    if (!living(ob))
    {
        ob->move(from);
        return ;
    }
    
    if (!ob->query_ghost())
    {
        ob->catch_msg("Ty zyjesz! Smierc nie chce przyjac cie w swoje " +
           "objecia.\n");
        ob->move_living("X", from, 1, 1);
        return ;
    }
    
    return ;
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
            kto->catch_msg("Masz nieistniejaca rase. Skontaktuj sie z " +
                "czlonkiem administracji Arkadii.\n");
            kto->remove_object();
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
        default: write("Nie mozesz tego tutaj uczynic.\n"); return 1;
    }
    
    return 0;
}

public void
zaladuj_smierci()
{
    string *pliki, plik, tekst;
    int x, y, size, size2;
    mixed *zawartosc;
    
    pliki = get_dir(SMIERCI);
    
    x = -1; size = sizeof(pliki);
    while (++x < size)
    {
        if (sscanf(pliki[x], "%s_%d.txt", plik, y) == 2)
        {
            zawartosc = ({});
            
            tekst = read_file(SMIERCI + pliki[x]);
            
            zawartosc = explode(tekst, "\n\n");
            
            y = -1; size2 = sizeof(zawartosc);
            while (++y < size2)
            {
                if (!intp(zawartosc[y]))
                    zawartosc[y] = implode(explode(zawartosc[y], "\n"), " ");
            }
            
            if (smierci[plik])
                smierci[plik] += ({ zawartosc });
            else
                smierci[plik] = ({ zawartosc });
        }
    }
}
