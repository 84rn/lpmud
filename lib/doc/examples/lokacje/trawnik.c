/*
 * Jest to przykladowa lokacja,
 * sporzadzona przez Technokrate.
 */
 
/* To powinno sie znalezc w kazdym pliku lokacji... */

inherit "/std/room";
#include <macros.h>
#include <stdproperties.h>

/* Prototypy */
int wyrwij_tabliczke();
int wyjdz();

void
create_room()
{
    set_short("Przykladowy trawnik");
    set_long("Jest to przykladowa lokacja, majaca pomoc ci w oswojeniu sie z "
           + "wymogami, jakie stawia ci Arkadia. Na trawniku trawa co prawda "
           + "nie rosnie (bo jest zima), ale za to, wbita w ziemie, stoi "
           + "sobie z lekka przekrzywiona tabliczka.\n");
   
    /* Tutaj dodajemy przedmioty, jakie gracz moze obejrzec. Zwrocmy uwage,
       ze nazwy sa podawane w bierniku - nie chcielibysmy, zeby trzeba (ani
       mozna) bylo uzywac polecenia <obejrzyj ziemia>. */
       
    add_item("ziemie",
             "Ziemia jest silnie przemarznieta. Jak to w zimie...\n");
    add_item("tabliczke",
             "Niezbyt starannie wykonana tabliczka, wbita w ziemie. Jest na "
           + "niej jakis napis.\n");
    
    /* Dobra lokacja charakteryzuje sie tym, ze wszystkie przedmioty
       wystepujace w opisach powinno dac sie obejrzec. Natomiast nie jest
       wlasciwe umieszczanie opisow przedmiotow, ktorych istnienia gracz nie
       musi domyslic sie, mimo uwaznego obejrzenia calej lokacji. */
       
    add_item("napis", "Zapewne daloby sie go przeczytac.\n");
    
    /* Dodajac komendy, musimy pamietac, aby spelnialy one pewne standardy.
       Powinninismy uzywac trybu rozkazujacego i, o ile to mozliwe, formy
       dokonanej. Wraz z parametrami, polecenie powinno byc sensownym i
       naturalnie zbudowanym zdaniem. */
       
    add_cmd_item(({"napis", "tabliczke"}), "przeczytaj",
                 "NIE DEPTAC TRAWY.\n\n"
               + "Pod spodem, malym drukiem, zawarto ostrzezenie o surowych "
               + "karach za wyrywanie podobnych tabliczek.\n");

    /* Jesli istnieje wiecej niz jedna mozliwosc zbudowania sensownego
       polecenia, warto umozliwic wykorzystanie kazdej... */
       
    add_cmd_item("tabliczke", ({"wyrwij", "wyrwij z ziemi", "zniszcz"}),
                 wyrwij_tabliczke);

    /* Na standardowe kierunki zostaly zalozone aliasy n,ne,... (oczywiste),
       oraz u,d (gora,dol). Ze wzgledow technicznych w kierunkach takich jak
       polnocny wschod wyrazy laczymy kreseczka. Naturalnie nie ma powodu,
       dla ktorego nie moglibysmy zdefiniowac innych wyjsc... Na przyklad
       gracz gubiacy sie w labiryncie podmiejskich kanalow (patrz umiejetnosc
       'wyczucie kierunku'), wchodzac na skrzyzowanie moglby widziec wyjscia:
       naprzod, lewo, prawo, zawroc... */
       
    add_exit("trawnik.c", "polnocny-wschod", wyjdz, 3);
    add_exit("trawnik.c", "poludniowy-wschod", wyjdz, 3);
    add_exit("trawnik.c", "poludniowy-zachod", wyjdz, 3);
    add_exit("trawnik.c", "polnocny-zachod", wyjdz, 3);
    add_exit("/d/Standard/start/church.c", "startroom", 0, 0, 1);
}

/* Byc moze taki rezultat dzialan gracza nie jest najinteligentniejszy, ale
   lokacja ta ma przeciez sluzyc celom edukacyjnym... :) */

int
wyrwij_tabliczke()
{
    saybb(QCIMIE(this_player(), PL_MIA) + " chwyta za tabliczke, "
        + "najwyrazniej z zamiarem wyrwania jej z podloza, gdy nagle jak "
        + "spod ziemi wyrasta pijany cieciu, wymachujacy z wrzaskiem przed "
        + this_player()->koncowka("jego", "jej") + " nosem zardzewialymi, "
        + "dwurecznymi nozycami ogrodowymi! Widzac efekt, jaki wywolal na "
        + QIMIE(this_player(), PL_MIE) + ", spluwa z satysfakcja, po czym "
        + "znika rownie tajemniczo jak sie pojawil...\n");
    write("Chwytasz z furia za tabliczke, usilujac wyrwac ja z podloza, gdy "
        + "nagle jak spod ziemi wyrasta pijany cieciu, wymachujacy przed "
        + "twoim nosem zardzewialymi, dwurecznymi nozycami ogrodowymi! "
        + "Wywrzaskuje ci prosto w twarz jakies bzdury, z ktorych rozumiesz "
        + "tylko to, ze twoj pomysl bardzo mu sie nie spodobal. Widzac "
        + "efekt, jaki na tobie wywolal, spluwa z satysfakcja, po czym "
        + "znika rownie tajemniczo jak sie pojawil...\n");
    return 1;
}

int
wyjdz()
{
    write("Potezna magia sprawia, ze wszystkie drogi sa tu zakrzywione. "
        + "Jesli utknales, zawsze mozesz sprobowac wyjscia <startroom>.\n");
    return 0;
}