/*
 *  Plik zawierajacy funkcje umozliwiajace picie wody w lokacjach i innych
 *  obiektach stacjonarnych.
 *
 *  /lib/drink_water.c
 *
 *  dzielo Silvathraeca 1997
 *
 *  Instrukcja obslugi:
 *  - W init() nalezy wywolac init_drink_water().
 *  - W create_xxx() nalezy wywolac set_drink_places() z lista akceptowanych
 *    argumentow komend 'napij sie [wody]' jako argumentem.
 *  - Warto tez zamaskowac drink_effect() i fill_effect(), aby uzyskac
 *    pasujace do kontekstu komunikaty towarzyszace piciu wody i napelnianiu
 *    nia pojemnikow. Mozna tam tez umiescic ewentualne niestandardowe efekty.
 */

#pragma no_clone
#pragma save_binary
#pragma strict_types

#include <cmdparse.h>
#include <composite.h>
#include <macros.h>
#include <std.h>
#include <stdproperties.h>

private static string *drink_places = ({});

/* Prototypy */
int drink_water(string str);
int fill_barrel(string str);

/*
 * Nazwa funkcji : init_drink_water
 * Opis          : Dodaje w obiekcie komendy sluzace do picia wody i
 *                 napelniania nia pojemnikow. Funkcja powinna byc
 *                 wywolywana w init().
 */
public void
init_drink_water()
{
    add_action(drink_water, "napij");
    add_action(fill_barrel, "napelnij");
}

/*
 * Nazwa funkcji : set_drink_places
 * Opis          : Ustawia miejsca, z ktorych mozna pic wode (czyli
 *                 akceptowane argumenty komendy 'napij sie [wody]')
 *                 i gdzie mozna napelniac pojemniki.
 * Argumenty     : places: String lub tablica stringow.
 */
public void
set_drink_places(mixed places)
{
    drink_places = stringp(places) ? ({places}) : places;
}

/*
 * Nazwa funkcji : query_drink_places
 * Opis          : Zwraca tablice stringow ustawiona przez set_drink_places.
 */
public string *
query_drink_places()
{
    return secure_var(drink_places);
}

/*
 * Nazwa funkcji : drink_effect
 * Opis          : Funkcja ta sluzy do wypisywania komunikatow towarzyszacych
 *                 napiciu sie wody przez gracza. Na ogol funkcja ta powinna
 *                 byc zamaskowana, aby uzyskac pasujacy do kontekstu opis.
 *                 Jesli napicie sie powoduje dodatkowo cos interesujacego,
 *                 mozna to rowniez tu umiescic.
 * Argumenty     : skad: Argument komendy 'napij sie [wody]'.
 */
public void
drink_effect(string skad)
{
    write("Pijesz lyk wody " + skad + ".\n");
    saybb(QCIMIE(this_player(), PL_MIA) + " pije lyk wody " + skad + ".\n");
}

/*
 * Nazwa funkcji : attempt_drink
 * Opis          : Wywolywana, gdy gracz usiluje napic sie wody.
 * Argumenty     : skad: Argument komendy 'napij sie [wody]'.
 */
public void
attempt_drink(string skad)
{
    object tp = this_player();

    if (tp->drink_soft(tp->query_prop(LIVE_I_MAX_DRINK) / 15))
        drink_effect(skad);
    else
        write("Wypil" + tp->koncowka("es", "as") + " juz tak "
            + "duzo, ze nie jestes w stanie wmusic w siebie wiecej.\n");
}

/*
 * Nazwa funkcji : drink_water
 * Opis          : Wywolywana, gdy gracz uzyje komendy 'napij sie'.
 * Argumenty     : str: Argument uzytej przez gracza komendy.
 */
public int
drink_water(string str)
{
    string skad;

    if (environment(this_player()) != this_object() &&
        !CAN_SEE(this_player(), this_object()))
        return 0;

    if (!CAN_SEE_IN_ROOM(this_player()))
    {
        string prop = environment(this_player())->query_prop(ROOM_S_DARK_MSG);

        notify_fail((prop ? prop : "Jest zbyt ciemno")
                  + ", by moc dostrzec cokolwiek.\n");
        return 0;
    }

    if (str &&
        (sscanf(str, "sie wody %s", skad) || sscanf(str, "sie %s", skad)) &&
        member_array(skad, query_drink_places()) != -1)
    {
        attempt_drink(skad);
        return 1;
    }
    notify_fail("Napij sie [wody] skad ?\n");
    return 0;
}

/*
 * Nazwa funkcji : fill_effect
 * Opis          : Funkcja ta sluzy do wypisywania komunikatow towarzyszacych
 *                 napelnianiu pojemnika woda przez gracza. Na ogol funkcja
 *                 ta powinna byc zamaskowana, aby uzyskac pasujacy do
 *                 kontekstu opis.
 * Argumenty     : beczulka: Napelniany pojemnik.
 *                 skad: Argument komendy 'napelnij <co> [woda]'.
 */
public void
fill_effect(object beczulka, string skad)
{
    if (beczulka->query_ilosc_plynu())
    {
        write("Dopelniasz " + beczulka->short(this_player(), PL_BIE)
            + " woda " + skad + ".\n");
        saybb(QCIMIE(this_player(), PL_MIA) + " dopelnia "
            + QSHORT(beczulka, PL_BIE) + " woda " + skad + ".\n");
    }
    else
    {
        write("Napelniasz " + beczulka->short(this_player(), PL_BIE)
            + " woda " + skad + ".\n");
        saybb(QCIMIE(this_player(), PL_MIA) + " napelnia "
            + QSHORT(beczulka, PL_BIE) + " woda " + skad + ".\n");
    }
}

/*
 * Nazwa funkcji : fill_barrel
 * Opis          : Wywolywana, gdy gracz uzyje komendy 'napelnij'.
 * Argumenty     : str: Argument uzytej przez gracza komendy.
 */
public int
fill_barrel(string str)
{
    object *oblist;
    string skad;

    if (environment(this_player()) != this_object() &&
        !CAN_SEE(this_player(), this_object()))
        return 0;

    if (!CAN_SEE_IN_ROOM(this_player()))
    {
        string prop = environment(this_player())->query_prop(ROOM_S_DARK_MSG);

        notify_fail((prop ? prop : "Jest zbyt ciemno")
                  + ", by moc dostrzec cokolwiek.\n");
        return 0;
    }

    if (str && parse_command(str, environment(this_player()),
        "%i:" + PL_BIE + " [woda] %s", oblist, skad) &&
        sizeof(oblist = NORMAL_ACCESS(oblist, 0, 0)))
    {
        object beczulka = oblist[0];

        this_player()->set_obiekty_zaimkow(oblist);
        if (sizeof(oblist) > 1)
        {
            notify_fail("Nie mozesz napelniac jednoczesnie woda "
                      + COMPOSITE_DEAD(oblist, PL_DOP) + ".\n");
            return 0;
        }
        if (function_exists("create_object", beczulka) != BECZULKA_OBJECT)
        {
            notify_fail("Nie mozesz napelnic woda " +
                        beczulka->short(this_player(), PL_DOP) + ".\n");
            return 0;
        }
        if (member_array(skad, query_drink_places()) == -1)
        {
            notify_fail("Napelnij " +
                        beczulka->short(this_player(), PL_BIE) +
                        " [woda] skad ?\n");
            return 0;
        }
        if (member_array(beczulka, all_inventory(this_player())) == -1)
        {
            notify_fail("Musisz trzymac " +
                        beczulka->short(this_player(), PL_BIE) +
                        ", jesli chcesz " +
                        beczulka->koncowka("go", "ja", "je") +
                        " napelnic woda.\n");
            return 0;
        }
        if (beczulka->query_ilosc_plynu() &&
            beczulka->query_nazwa_plynu_dop() != "wody")
        {
            notify_fail("Nie mozesz napelnic " +
                        beczulka->short(this_player(), PL_DOP) +
                        " woda dopoki nie wypijesz z niej " +
                        beczulka->query_nazwa_plynu_dop() + ".\n");
            return 0;
        }
        if (beczulka->query_ilosc_plynu() == beczulka->query_pojemnosc())
        {
            notify_fail(capitalize(beczulka->short(this_player(), PL_MIA)) +
                        " jest juz pel" +
                        beczulka->koncowka("en", "na", "ne") + " wody.\n");
            return 0;
        }
        fill_effect(beczulka, skad);
        beczulka->set_opis_plynu("zwyklej, czystej wody");
        beczulka->set_nazwa_plynu_dop("wody");
        beczulka->set_vol(0);
        beczulka->set_ilosc_plynu(beczulka->query_pojemnosc());
        return 1;
    }
    notify_fail("Napelnij co [woda] skad ?\n");
    return 0;
}