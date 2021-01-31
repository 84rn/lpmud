/* NPC reagujacy na emoty i przedstawiajacy sie.
 * Napisal Rasputin
 */

inherit "/std/monster"; /* jest to osobnik humanoidalny */

#include <ss_types.h>
#include <macros.h>
#include <stdproperties.h>

#define TO    this_object()
#define MIECZ "/doc/examples/bronie/miecz.c"
#define BUTY  "/doc/examples/zbroje/buty.c"
#define ZBRO "/doc/examples/zbroje/kolczuga.c"
#define SPOD "/doc/examples/zbroje/spodnie.c"

void
create_monster()
{
/* Ta funkcja odmieni nam krotka nazwe - z niej beda pobierane dane, 
   gdy zajdzie potrzeba wyswietlenia krotkiego opisu dowodczyni. 
   ALE, ta funkcja nie dodaje nazw, o ktore gracz moglby 'zahaczyc' (czyli
   takich, ktore moglby uzyc w komendzie). Do tego celu bedziemy musieli
   uzyc funkcji dodaj_nazwy() i dodaj_przym().
*/
    ustaw_shorty(({ "dowodczyni strazy", "dowodczyni strazy",
       		    "dowodczyni strazy", "dowodczynie strazy",
       		    "dowodczynia strazy", "dowodczyni strazy"}),
                 ({ "dowodczynie strazy", "dowodczyn strazy",
                    "dowodczyniom strazy", "dowodczynie strazy", 
                    "dowodczyniami strazy", "dowodczyniach strazy" }),
                    PL_ZENSKI);

/* 
 * Dodajemy nazwe 'dowodczyni'. Dane z tej tablicy nie beda 
 * uzywane do wyswietlenia na ekranie. Beda tylko sluzyly do 
 * porownywania z tym, co gracz wpisze, np. w 'zaatakuj dowodczynie'.
 */
    dodaj_nazwy(({ "dowodczyni", "dowodczyni", "dowodczyni", "dowodczynie", 
    		   "dowodczynia", "dowodczyni" }), ({ "dowodczynie", 
    		   "dowodczyn", "dowodczyniom", "dowodczynie", "dowodczyniami",
                   "dowodczyniach" }), PL_ZENSKI);

/*
 * Ustawiamy dlugi opis dowodczyni - taki, ktory sie nam ukarze
 * po wpisaniu 'obejrzyj dowodczynie'.
 */                
    set_long("Jest to dowodczyni strazy miejskiej, kobieta niezwyklej urody. "+
             "Dlugie, czarne wlosy spiete z tylu opadaja jej na plecy. Rownie "+
             "czarne oczy swieca jakims niezwyklym blaskiem. Jej bujne ksztalty "+
             "skryte sa pod twarda kolczuga, a skorzane, obcisle spodnie "+
             "opasuja jej kragle biodra i podkreslaja niezwykla zgrabnosc "+
             "jej dlugich nog. Wysokie, podkute buty dodaja jej subtelnego "+
             "uroku. Jej twarz promienieje radoscia i dziewczeca niewinnoscia. "+
             "Rece ma smukle, a dlonie niezwykle gladkie, jakby stworzone "+
             "do pieszczot, a nie do dzierzenia miecza. Az trudno uwierzyc, ze "+  
             "tak urocza istota trudni sie wojaczka.\n");

/*
 * Ktos mily juz odmienil za nas odmiane rasy elfki. My po prostu korzystamy
 * z tego. Makra do odmiany sa zdefiniowane w /sys/pl.h
 */
    ustaw_odmiane_rasy(PL_ELFKA);

/*
 * Ustalamy plec dowodczyni.. jak sama nazwa wskazuje bedzie to kobieta :-)
 */    
    set_gender(G_FEMALE);
 
/* 
 * Nikt nigdy nie pozna imienia dowodczyni - wszyscy beda ja znali, jako
 * 'dowodczyni strazy'.
 */
    add_prop(LIVE_I_NEVERKNOWN, 1);
 
 /* 
  * 'Rozmiary' dowodczyni :-)
  */
    add_prop(CONT_I_WEIGHT, 65000); // ... 65 kilo...
    add_prop(CONT_I_HEIGHT, 175);   // ... i 175 cm wzrostu..

    set_skill(SS_DEFENCE, 35 + random(5));
    set_skill(SS_WEP_SWORD, 25 + random(10));
    set_skill(SS_UNARM_COMBAT, 15 + random(5));
    set_skill(SS_PARRY, 15 + random(10));

    set_stats(({ 25, 25, 22, 30, 30, 40}));

    set_chat_time(10);
    add_chat("Witaj, Przybyszu!");
    add_chat("Skad przybywasz?");
    add_chat("Ladna, pogoda, nieprawdaz?");

    set_cchat_time(5);
    add_cchat("A masz!");
    add_cchat("Dlaczego mnie bijesz?");
    add_cchat("Jestem stworzona do innych rzeczy!");

    set_act_time(10);
    add_act(({"mrugnij", "westchnij gleboko", "usmiechnij sie ."}));
    add_act("emote puszcza do ciebie oczko.");
    add_act("emote usmiecha sie do ciebie zalotnie.");

/*
 * Uzbrajamy nasze malenstwo. Swiat jest taki brutalny...
 */

    add_weapon(MIECZ);
    add_armour(ZBRO);
    add_armour(BUTY);
    add_armour(SPOD);
    /* Funkcje add_weapon() i add_armour() mamy dzieki temu, ze
     * /std/monster.c dziedziczy /std/act/add_things.c.
     */
}

/*  Staramy sie bardzo unikac triggerow, poniewaz sa BARDZO wolne, 
   to wykorzystamy pewien trick na przedstawianie sie. add_introduced()
   jest wywolywane we wszystkich zyjacych obiektach w pokoju, w ktorym ktos 
   sie przedstawil. Wykorzystujemy to do odpowiedniego zareagowania na 
   to, ze ktos sie przedstawil.
*/
void
add_introduced(string imie)
{
    set_alarm(2.0, 0.0, "return_introduce", imie);
}

void
emote_hook(string emote, object wykonujacy, string przyslowek)
{
    switch (emote)
    {
        case "uklon":
            set_alarm(2.0, 0.0, "command", "uklon sie "
                                         + OB_NAME(wykonujacy));
            break;
        case "pocaluj":
        case "przytul":
            command("powiedz Co ty sobie wyobrazasz?!");
            command("kopnij " + OB_NAME(wykonujacy));
            break;
        case "kopnij":
        case "opluj":
            set_alarm(2.0, 0.0, "wkop_mu", wykonujacy);
            break;
        case "zasmiej":
            set_alarm(2.0, 0.0, "return_laugh", wykonujacy);
            break;
    }
}

void
wkop_mu(object kto)
{
    switch (random(3))
    {
        case 0:
            command("powiedz Jak smiales to zrobic, chamie?!");
            command("spoliczkuj " + OB_NAME(kto));
            command("kopnij " + OB_NAME(kto));
            break;
        case 1:
            command("powiedz Zrob to jeszcze raz a pozalujesz!");
            command("kopnij " + OB_NAME(kto));
            break;
        case 2:
            command("powiedz Dosc tego! Teraz dostaniesz za swoje!");
            command("zabij " + OB_NAME(kto));
    }
}


void
return_introduce(string imie)
{
    object osoba;
    
    osoba = present(imie, environment());

    if (osoba)
        command("powiedz Milo cie poznac, " + osoba->query_wolacz() + ".");
}


void
return_laugh(object kto) 
{
    switch(random(2))
    {
        case 0:
            command("powiedz Tak, tak, smiech to zdrowie.");
            break;
        case 1:
            command("powiedz Nie smiej sie tak glosno.");
            command("powiedz Zaklocasz porzadek publiczny.");
    }
}
