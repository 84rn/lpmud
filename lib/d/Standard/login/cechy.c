#include "login.h"

inherit "/std/room.c";

#pragma strict_types

#include <std.h>
#include <stdproperties.h>

public void
create_room()
{
    set_short("Tworzenie postaci - wybor cech i plci");
    set_long("Znajdujesz sie w pomieszczeniu przypominajacym olbrzymia " + 
        "galerie. Nie ma tu scian, jedynie sufit w ksztalcie kopuly, " +
        "obnizajacy sie az do poziomu podlogi. Na calej jej powierzchni " +
        "spostrzegasz szkice roznych postaci. Kazdy z nich wyroznia sie jakas " +
        "specyficzna cecha, taka jak dlugi nos czy krzywe nogi. Jest ich " +
        "mnostwo, wrecz zatrzesienie. Zauwazasz, ze zostaly one pogrupowane " +
        "w rozne kategorie tematyczne. Jedna czesc poswiecona jest " +
        "na przyklad samym oczom, inna zas wlosom. Na srodku pomieszczenia " + 
        "widzisz dziwna postac, siedzaca w kucki, skulona nad niewielkim " +
        "kawalkiem gliny.\n");
        
    add_item( ({ "postac", "rzezbiarza", "artyste", "dziwna postac" }), 
        "Dziwny stwor, siedzacy w kucki po srodku galerii. Caly " +
        "pokryty jest guzowata, brazowoszara, gola skora, tak, ze " +
        "nie ma na nim nawet najmniejszego wloska. Jego twarz " +
        "wyraza skupienie. Zrecznymi palcami bawi sie swoim kawalkiem " +
        "gliny. Nie trzeba go dlugo obserwowac, zeby sie zorientowac, " +
        "ze dobrze zna sie na tym.\n");
        
    add_prop(ROOM_I_INSIDE, 1);
    add_prop(ROOM_I_LIGHT, 1);

        
    add_exit(PATH + "sala.c", "polnoc", 0, 0);
}

public void
enter_inv(object ob, object from)
{
    object ustawiacz;
    
    ::enter_inv(ob, from);
    
    if (ustawiacz = present("/wybieracz/", ob))
    {
        ustawiacz->wrocil();
    }
    else
    if (this_player()->query_ghost()) 
        clone_object(PATH + "set_cechy.c")->move(this_player(), 1);
}

public void
leave_inv(object ob, object gdzie)
{
    object ustawiacz;
    
    ::leave_inv(ob, gdzie);
    
    if (ustawiacz = present("/wybieracz/", ob))
        ustawiacz->wyszedl();
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
#if 0
public void
init()
{
    ::init();
    
    add_action("komendy", "", 1);
}
#endif

public int
komendy(string str)
{
    if (SECURITY->query_wiz_rank(this_player()->query_real_name()) >= WIZ_ARCH)
        return 0;
        
    switch(query_verb())
    {
        case "sp":
        case "spojrz":
        case "ob":
        case "obejrzyj":
        case "l":
        case "look":
        case "save":
        case "nagraj":
        case "quit":
        case "zakoncz":
        case "zglos":
        case "bug":
        case "praise":
        case "idea":
        case "goto":
        case "Goto":
        case "polnoc":
        case "polnocny-zachod":
        case "zachod":
        case "poludniowy-zachod":
        case "poludnie":
        case "poludniowy-wschod":
        case "wschod":
        case "polnocny-wschod":
        case "wybierz":
        case "odpowiedz":
        case "home": return 0;
        case "?": this_object()->pomoc(str); return 1;
        default: write("To nie jest mozliwe w tym miejscu. Wpisz '?', zeby " +
            "uzyskac pomoc.\n"); return 1;
    }

}
