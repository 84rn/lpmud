#include "login.h"

inherit "/std/object.c";

#include <pl.h>
#include <const.h>
#include <living_desc.h>
#include <ss_types.h>
#include <macros.h>
#include <stdproperties.h>
#include <options.h>

#pragma strict_types

public void akcja();
public void transformacja();

object gracz;
mixed cechy, atr;
int faza = 0, faza_zmieniona = 0;
int plec = -1;
int wzrost, waga;
int alarm_id;
string rasa;

public void
create_object()
{
    add_name("/wybieracz/", 0);
    add_prop(OBJ_M_NO_DROP, "Ten tekst nie powinien ci sie nigdy pokazac. " +
        "Zglos blad");
    set_no_show();
}

public void
init()
{
    ::init();
    
    gracz = this_player();
    rasa = gracz->query_race_name(); 
    
    if (member_array(rasa, RACES) == -1)
    {
        faza = -5;
        alarm_id = set_alarm(5.0, 0.0, &akcja());
    }
    else
    {
        faza = 0;
        alarm_id = set_alarm(15.0, 0.0, &akcja());
    }
    
    add_action("komendy", "", 1);
}

public void
akcja()
{
    float czas;
    
    if (file_name(environment(gracz)) != PATH + "cechy")
        return ;
        
    faza_zmieniona = 0;

    switch (faza++)
    {
        case -5: gracz->catch_msg("Postac podnosi glowe i patrzy na ciebie " +
            "ze zdziwieniem.\n"); czas = 3.0; break;
        case -4: gracz->catch_msg("Postac mowi: Alez ty jeszcze nie " +
            "przynalezysz do zadnej rasy!\n"); czas = 3.0; break;
        case -3: gracz->catch_msg("Postac mowi: Przyjdz tu ponownie, " +
            "gdy juz podejmiesz decyzje.\n"); czas = 2.0; break;
        case -2: gracz->catch_msg("Postac skupia sie spowrotem na swym " +
            "kawalku gliny.\n"); return;
            
        case -1: gracz->catch_msg("Postac mowi: Dobrze, a zatem zacznijmy " +
            "jeszcze raz.\n"); czas = 3.0; faza = 2; break;
            
        case 0: gracz->catch_msg("Postac podnosi glowe znad swego kawalka " +
            "gliny i usmiecha sie do ciebie.\n"); czas = 4.0; break;
        case 1: gracz->catch_msg("Postac mowi: Milo ze wreszcie jestes. " +
            "Jestem gotow, a ty?\n"); czas = 4.0; break;
        case 2: gracz->catch_msg("Postac mowi: Wpierw chcialbym sie " +
            "dowiedziec, czy zamierzasz wedrowac po swiecie jako " +
            "mezczyzna, czy jako kobieta?\n"); return;

        case 3:
            atr = ({ HEIGHTDESC((plec == G_FEMALE ? "a" : "i")),
                       WIDTHDESC((plec == G_FEMALE ? "a" : "y")) });
            gracz->catch_msg("Postac mowi: Wolisz byc " +
                implode(atr[0][0..-2], ", ") + ", czy " + 
                atr[0][sizeof(atr[0]) - 1] + "?\n");
            return;

        case 4: gracz->catch_msg("Postac mowi: " + 
                capitalize(implode(atr[1][0..-2], ", ")) + ", czy " +
                atr[1][sizeof(atr[1]) - 1] + "?\n");
            return;

        case 5: gracz->catch_msg("Postac ugniata gline na ksztalt malej " +
            "figurki, nadajac jej podstawowe cechy wymienione przez " +
            "ciebie.\n"); cechy = allocate(2); czas = 4.0; break;
        case 6: gracz->catch_msg("Postac mowi: Obejrzyj me szkice i " +
            "wybierz dwie cechy, ktore ci najbardziej odpowiadaja.\n"); 
            czas = 5.0; break;
        case 7: /*gracz->catch_msg("Postac mowi: Aha, tylko dopilnuj, " +
            "zeby wybrana przez ciebie para cech pochodzila z dwoch roznych " +
            "kategorii.\n"); */ return;

        case 8: gracz->catch_msg("Postac usmiecha sie szeroko i " +
            "wprawnymi ruchami dloni dodaje figurce wybrana przez " +
            "ciebie ceche. Widzisz ze masz do czynienia z nie lada " +
            "artysta.\n"); czas = 5.0; break;
        case 9: gracz->catch_msg("Postac mowi: Jaki jest twoj drugi " +
            "wybor?\n"); return;

        case 10: gracz->catch_msg("Po chwili gliniana figurka jest juz w " +
            "pelni uksztaltowana. Artysta jest wyraznie zadowolony ze " +
            "swego dziela.\n"); czas = 6.0; break;
        case 11: gracz->catch_msg("Postac mowi: Jesli wyglad figurki " +
            "odpowiada twym oczekiwaniom, dotknij jej.\n"); return;
           
        case 12: gracz->catch_msg("Nagle czujesz ze swiat zaczyna sie " +
            "kurczyc wokol ciebie. Masz wrazenie, ze figurka wsysa cie cal" +
            (plec == G_MALE ? "ego" : "a") + ".\n"); czas = 4.5; break;
        case 13: gracz->catch_msg("Powoli zachodzi jakas dziwna " +
            "metamorfoza. Zaczynasz widziec swiat z nowej perspektywy, " +
            "z miejsca, gdzie jeszcze przed chwila stala owa gliniana " +
            "figurka.\n"); czas = 6.0; break;
        case 14: gracz->catch_msg("Mgla wokol ciebie gestnieje, " +
            "zaslaniajac powoli widok przedziwnej galerii.\n"); 
            czas = 2.0; break;
        case 15: gracz->catch_msg("\n\n"); 
                 gracz->move_living("M", PATH + "sala.c", 1, 0); 
                 return;

        case 16: gracz->catch_msg("Postac usmiecha sie dumnie na twoj " +
            "widok.\n"); return;
    }
    alarm_id = set_alarm(czas, 0.0, &akcja());
}

public int 
komendy(string str)
{
    mixed *przym;
    int x;
    string tekst, tekst2, verb;
    
    if (file_name(environment(this_player())) != PATH + "cechy")
        return 0;

    verb = query_verb();
    
    if (verb == "odpowiedz" || verb == "powiedz" || verb[0] == ''')
    {
        if (verb[0] == ''')
            if (strlen(verb) == 1)
                return 0;
            else
                str = verb[1..] + (str ? (" " + str) : "");
            
        if (faza == 3)
        {
            if (str == "mezczyzna")
                plec = G_MALE;
            else if (str == "kobieta")
                plec = G_FEMALE;
            else
            {
                write("Postac mowi: Hmm, nie za bardzo ciebie rozumiem. " +
                   "Odpowiedz, ze 'kobieta', albo 'mezczyzna'.\n");
                return 1;
            }
            
            switch(random(5))
            {
                case 0: write("Postac mowi: Ha, ma sie rozumiec...\n" +
                          "Postac usmiecha sie wesolo do ciebie.\n"); break;
                case 1: write("Postac mowi: " + capitalize(str) + ", " +
                          "oczywista sprawa!\n"); break;
                case 2: write("Postac mowi: Umm! " + capitalize(str) + 
                          ", interesujace..\n"); break;
                case 3: write("Postac mowi: " + capitalize(str) + "?!\n" +
                          "Postac spoglada na ciebie podejrzliwie.\n"); break;
                case 4: write("Postac spoglada na ciebie.\n" +
                          "Postac mowi: " + capitalize(str) + 
                          ", no moglem sie tego domyslec!\n"); break;
            }
            
            alarm_id = set_alarm(5.0, 0.0, &akcja());
            
            return 1;
        }
        else if (faza == 4) // wybor wzrostu
        {
            if ((wzrost = member_array(str, atr[0])) == -1)
            {
                write("Postac mowi: Odpowiedz, ze " + 
                    implode(atr[0][0..-2], ", ") + ", albo ze " + 
                    atr[0][sizeof(atr[0]) - 1] + ".\n");
                return 1;
            }
            
            write("Postac mowi: ");
            switch(random(5))
            {
                case 0: write("Znakomicie!\n"); break;
                case 1: write("Dobrze.\n"); break;
                case 2: write("Zrobi sie.\n"); break;
                case 3: write("Naturalnie.\n"); break;
                case 4: write("O kurcze!\n"); break;
            }
            
            alarm_id = set_alarm(3.0, 0.0, &akcja());
            
            return 1;
            
        }
        
        else if (faza == 5) // wybor otylosci
        {
            if ((waga = member_array(str, atr[1])) == -1)
            {
                write("Postac mowi: Odpowiedz, ze " + 
                    implode(atr[1][0..-2], ", ") + ", albo ze " + 
                    atr[1][sizeof(atr[1]) - 1] + ".\n");
                return 1;
            }
            
            write("Postac mowi: ");
            switch(random(5))
            {
                case 0: write("Znakomicie!\n"); break;
                case 1: write("Dobrze.\n"); break;
                case 2: write("Zrobi sie.\n"); break;
                case 3: write("Naturalnie.\n"); break;
                case 4: write("O kurcze!\n"); break;
            }
            
            alarm_id = set_alarm(3.0, 0.0, &akcja());
            
            return 1;
        }
    }
    else if (verb == "ob" || verb == "obejrzyj")
    {
        if (str == "szkice")
        {
            write("Jest ich cale mnostwo. Kazdy szkic przedstawia " +
                "postac wyrozniajaca sie jedna cecha. Latwiej " +
                "bedzie ci zapoznac sie z nimi, jesli bedziesz analizowac " +
                "je kategoriami, miast ogladac wszystkie naraz.\n");
            return 1;
        }
        else if (strlen(str) && str[0..8] == "kategorie")
        {
            if (!rasa)
            {
                write("Jeszcze za wczesnie na interesowanie sie szkicami. "+
                    "Wpierw wybierz rase w pomieszczeniu na polnoc stad.\n");
                    
                return 1;
            }
            return call_other(PATH + "check_attrib.c", "list", 
                (strlen(str) > 10 ? str[10..] : "kategorie"), rasa, plec);
        }
        else if (str == "figurke" || str == "rzezbe")
        {
            if (faza < 6)
            {
                write("Jeszcze nie powstala zadna figurka. Dostarcz postaci " +
                    "wystarczajacych informacji, by mogla ja ulepic.\n");
                return 1;
            }
            
            if (faza > 15)
            {
                write("Szukasz jej wzrokiem, ale nigdzie jej nie ma.\n");
                return 1;
            }

            tekst = "Gliniana figurka przedstawia ";
            tekst2 = "Bez watpienia potrzebuje jeszcze wykonczenia.\n";
            
            if (sizeof(cechy[0]))
            {
                tekst += oblicz_przym(cechy[0][0], cechy[0][1], PL_BIE,
                    ODMIANA_RASY_RODZAJ[rasa][plec], 0) + " ";
                tekst2 = "Czujesz, ze czegos jej jeszcze brakuje.\n";
            }
            
            if (sizeof(cechy[1]))
            {
                tekst += oblicz_przym(cechy[1][0], cechy[1][1], PL_BIE,
                    ODMIANA_RASY_RODZAJ[rasa][plec], 0) + " ";
                tekst2 = "Jest juz w pelni gotowa.\n";
            }
            
            if (ODMIANA_RASY_OSOBNO[rasa])
                tekst += LD_HUM_GENDER_MAP[plec][PL_BIE] + " ";
            
            tekst += ODMIANA_RASY[rasa][plec][PL_BIE] + ".\n";
            
            tekst += "Jest on" + (plec == G_FEMALE ? "a": "") + " ";
            tekst += atr[0][wzrost] + " i " + atr[1][waga] + " jak na " +
                ODMIANA_RASY[rasa][plec][PL_BIE] + ".\n\n";
                
            write(tekst + tekst2 + "\n");
            
            return 1;
            
            
        }
        notify_fail("Co chcesz obejrzec? Szkice, kategorie, czy moze " +
            "figurke?\n");
        return 0;
    }  
 
    else if (verb == "wybierz")
    {
        if (faza == 7 || faza == 8 || faza == 10)
        {
            przym = call_other(PATH + "check_attrib.c", "select", str, rasa, plec);
            
            if (!przym)
                return 1;
              
            if (faza != 10) /* pierwsza cecha */
            {
                cechy[0] = przym;
            }
            else
            {
                if (cechy[0][0] == przym[0])
                {
                    write("Postac mowi: Druga cecha musi sie roznic " + 
                        "od pierwszej.\n");
                    return 1;
                }
                cechy[1] = przym;
            }
                
            alarm_id = set_alarm(1.0, 0.0, &akcja());
            
            return 1;
        }
        return 0;
    }
    else if (verb == "zrezygnuj")
    {
        if (faza < 4)
        {
            write("Jeszcze nie masz z czego rezygnowac.\n");
            return 1;
        }
        
        if (faza > 5)
            write("Postac jest troche zawiedziona, jednak zgodnie z twoja " +
                "wola niszczy powstala rzezbe, ugniatajac ja spowrotem " +
                "do postaci zwyklej kulki z gliny.\n");
           
        faza = -1;

        remove_alarm(alarm_id);

        cechy = atr = wzrost = waga = 0;
        plec = -1;
        
        alarm_id = set_alarm(2.0, 0.0, &akcja());
        
        return 1;
    }
    else
    if (verb == "dotknij")
    {
        if (str != "figurki" && str != "rzezby")
        {
            write("Dotknij czego? Figurki?\n");
            return 1;
        }
        
        if (faza != 12)
        {
            if (faza < 6)
            {
                write("Pozwol postaci ja najpierw wyrzezbic.\n");
                return 1;
            }
            
            write("Chcesz dotknac glinianej figurki, lecz postac " +
                "naglym ruchem reki broni ci don dostepu. W oczach " +
                "artysty zapalaja sie dwa wsciekle ogniki.\n" +
                "Postac mowi: Glupcze! Dotkniecie figurki, kiedy nie jest " +
                "ona jeszcze w pelni uksztaltowana grozi niewyobrazalnymi " +
                "konsekwencjami!\n");
            return 1;
        }
        
        write("Dotykasz niepewnie glinianej figurki i...\n");
        
        /* Musi byc wywolana tutaj, bo set_option wymaga
         * this_player() == this_interactive()
         */
        transformacja(); 
        
        alarm_id = set_alarm(1.5, 0.0, &akcja());
        
        return 1;
    }
    
    switch(query_verb())
    {
        case "sp":
        case "spojrz":
        case "l":
        case "look":
        case "exa":
        case "examine":
        case "save":
        case "nagraj":
        case "quit":
        case "zakoncz":
        case "zglos":
        case "bug":
        case "praise":
        case "idea":
        case "polnoc":
        case "polnocny-zachod":
        case "zachod":
        case "poludniowy-zachod":
        case "poludnie":
        case "poludniowy-wschod":
        case "wschod":
        case "polnocny-wschod": return 0;
        case "?": this_object()->pomoc(str); return 1;
        default: write("To nie jest mozliwe w tym miejscu. Wpisz '?', zeby " +
            "uzyskac pomoc.\n"); return 1;
    }

    
    return 0;
}

public void
transformacja()
{
    int x, *tab, *wproc, *hproc, real_wzrost, real_waga, stat;
    mapping skille;
    
    gracz->set_gender(plec);
    gracz->ustaw_odmiane_rasy(ODMIANA_RASY[rasa][plec],
        ODMIANA_PRASY[rasa][plec], ODMIANA_RASY_RODZAJ[rasa][plec],
        ODMIANA_RASY_OSOBNO[rasa]);
        
    tab = RACESTAT[rasa];
    
    for (x = SS_STR; x <= SS_DIS; x++)
        gracz->set_base_stat(x, tab[x]);
        
    gracz->set_learn_pref(0);
    
    gracz->stats_to_acc_exp();
    
    for (x = 0; x < 6; x++)
    {
        stat = random(6);
        gracz->set_acc_exp(stat, (gracz->query_acc_exp(stat) + 150));
    }
    gracz->acc_exp_to_stats();
    gracz->stats_to_acc_exp();
    
    tab = RACEATTR[rasa][plec];
    hproc = HEIGHTPROC;
    wproc = WEIGHTPROC;
    
    real_wzrost = tab[0] * (hproc[wzrost] + random(hproc[wzrost + 1] -
        hproc[wzrost])) / 100;
    
    real_waga = real_wzrost * tab[2] * (wproc[waga] + 
        random(wproc[waga + 1] - wproc[waga])) / 100;
    
    gracz->add_prop(CONT_I_HEIGHT, real_wzrost);
    gracz->add_prop(CONT_I_WEIGHT, real_waga);
    gracz->add_prop(CONT_I_VOLUME, real_waga);
    
    gracz->dodaj_przym(cechy[0][0], cechy[0][1]);
    gracz->dodaj_przym(cechy[1][0], cechy[1][1]);
    
    gracz->set_hp(gracz->query_max_hp());
    gracz->set_ghost(0);
    
    gracz->set_option(OPT_ECHO, 1);
    gracz->set_whimpy(40);
    
    return ; 
}

public void
powrot_tekst()
{
    gracz->catch_msg("Postac mowi: Dobrze, ze juz jestes spowrotem. " +
        "Kontynuujmy!\n");
    
    if ((faza == 3 || faza == 4 || faza == 5 || faza == 8 || faza == 10)
        && !faza_zmieniona)
    {
        faza--; // fazy, przed ktorymi sa fazy gdzie sie czeka na gracza
        faza_zmieniona = 1;
    }

    alarm_id = set_alarm(3.0, 0.0, &akcja());
    
    return;
}

public void
wyszedl()
{
    if (faza == 12)
    {
        gracz->catch_msg("... I wysz" + (plec == G_MALE ? "edles" : "las") +
            "... \n");
    }
    else if (faza == 13)
        gracz->catch_msg("Wychodzac przerwal" + 
            (plec == G_MALE ? "es" : "as") + " ten dziwny i magiczny akt.\n");
            
    remove_alarm(alarm_id);
}

public void
wrocil()
{
    string nowa_rasa;
    
    nowa_rasa = this_player()->query_race_name();
    
    if (nowa_rasa != rasa)
    {
        if (faza <= -1)
        {
             setuid(); seteuid(getuid());
             clone_object(MASTER)->move(environment(this_object()));
             remove_object();
             return ;
        }
    }
    if (faza >= 0 && faza <= 1)
        faza = 0;
    else if (faza == 2) ;
    else if (faza >= 3 && faza < 12)
    {
        alarm_id = set_alarm(2.0, 0.0, &powrot_tekst());
        return;
    }
    else if (faza < 0)
        faza = -5;
    else if (faza == 12)
        return;
    else if (faza == 13)
    {
        faza = 12; 
        return;
    }
    else
        faza = 16;
        
    alarm_id = set_alarm(4.0, 0.0, &akcja());
}

public int
pomoc(string str)
{
    if (!this_player()->query_ghost())
    {
        write("Ty juz masz cielesna forme - nic tu po tobie.\n");
        return 1;
    }
    
    if (member_array(this_player()->query_race_name(), RACES) == -1)
    {
        write("Zanim bedziesz w stanie cokolwiek tu uczynic, musisz " +
            "wybrac sobie jakas rase. W tym celu udaj sie na polnoc, " +
            "do hali z posagami.\n");
    }
    
    write("Znajdujesz sie w miejscu, gdzie ustawia sie " +
      "plec oraz cechy wygladu. Postac przebywajaca tutaj ulepi " +
      "figurke na podobienstwo twojej przyszlej postaci. W tym celu " +
      "musisz jej podac kilka wskazowek, jak chcesz wygladac.\n" +
      "Dostepne sa nastepujace komendy:\n" + 
      " - 'odpowiedz' - odpowiedzi na pytania postaci o plec, wzrost, itp.\n" +
      " - 'obejrzyj figurke' - sprawdzenie dotychczasowych postepow.\n" +
      " - 'obejrzyj kategorie' - wyswietlenie listy wszystkich kategorii cech.\n"+
      " - 'obejrzyj kategorie <nazwa>' - wyswietlenie cech z danej kategorii.\n" +
      " - 'wybierz <nazwa cechy>' - wybranie jednej z cech.\n" +
      " - 'zrezygnuj' - zanulowanie dotychczasowej pracy i rozpoczecie od nowa.\n"+
      " - 'dotknij figurki' - potwierdzenie wczesniejszych wyborow.\n" +
      "\nPoza tym sluchaj uwaznie postaci, a wszystko bedzie jasne.\n");
        
    return 1;
}
