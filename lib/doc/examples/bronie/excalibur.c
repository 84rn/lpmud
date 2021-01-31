/*
 *  Przyklad broni z polska odmiana.
 *
 *  /d/Wiz/silvathraec/examples/excalibur.c
 *
 *  dzielo Silvathraeca 1997
 */

/* To nalezy umiescic w kazdym pliku broni... */

inherit "/std/weapon";

#include <formulas.h>
#include <macros.h>
#include <ss_types.h>
#include <stdproperties.h>
#include <wa_types.h>

void
create_weapon()
{
    /* Tutaj ustawiamy nazwy naszego obiektu... Mozemy sie do niego
       odwolywac, podajac ktoras z nich, lub piszac 'bron' (Odpowiedzialny
       za ta druga mozliwosc jest inherit "/std/weapon.c".) */

    dodaj_nazwy(({"excalibur", "excalibura", "excaliburowi", "excalibura",
                  "excaliburem", "excaliburze"}),
                ({"excalibury", "excaliburow", "excaliburom", "excalibury",
                  "excaliburami", "excaliburach"}), PL_MESKI_NOS_ZYW);
    dodaj_nazwy(({"miecz", "miecza", "mieczowi", "miecz", "mieczem",
                  "mieczu"}),
                ({"miecze", "mieczy", "mieczom", "miecze", "mieczami",
                  "mieczach"}), PL_MESKI_NZYW);

    /* Tu ustalamy, jaka bedzie krotka nazwa naszego obiektu. Zwrocmy uwage,
       ze przyjete jest, aby gracze wydajac komendy uzywali wylacznie malych
       liter (z wyjatkiem oczywiscie tego co mowia). Dlatego wlasnie nie
       moglismy pozwolic, zeby mudlib sam ustalil shorta korzystajac z
       ustaw_nazwe. */

    ustaw_shorty(({"Excalibur", "Excalibura", "Excaliburowi", "Excalibura",
                  "Excaliburem", "Excaliburze"}),
                ({"Excalibury", "Excaliburow", "Excaliburom", "Excalibury",
                  "Excaliburami", "Excaliburach"}), PL_MESKI_NOS_ZYW);

    /* Tak bedzie wygladal nasz miecz, jesli go obejrzymy. */

    set_long("Wystarczy jedno spojrzenie i nie masz zadnych watpliwosci... "
           + "oto Excalibur, legendarny miecz krola Artura! Zadnych "
           + "watpliwosci... zadnych watpliwosci... HMMM... Co robi tu ten "
           + "idiotyczny napis?!\n");

    /* Skoro juz umiescilismy w opisie to co umiescilismy, badzmy
       konsekwentni... :) */

    add_item("napis", "Istotnie, jest tu napis.\n");

    add_cmd_item("napis", "przeczytaj",
                 "Manufaktura Wiesenthal i syn, Winchester (d. Camelot)\n"
               + "Istnieje od 1879 roku.\n");

    /* A to juz standardowe dodatki... */

    set_hit(8);     /* Wspolczynnik okreslajacy latwosc operowania bronia. */
    set_pen(16);    /* Wspolczynnik okreslajacy zadawane uszkodzenia. */

    set_wt(W_SWORD);                /* Typ broni - miecz. */
    set_dt(W_IMPALE | W_SLASH);     /* Bron klujaco-tnaca. */

    set_hands(W_BOTH);              /* Dwureczna. */

    add_prop(OBJ_I_WEIGHT, 6000);   /* 6 kg, 0.67 l */
    add_prop(OBJ_I_VOLUME, 670);    /* Patrz tabela na koncu man WSTEP. */

    /* Nie ustawiajac wartosci broni, pozwalamy by zostala ustawiona
       standardowa, wyliczona w zaleznosci od klasy broni. */
}