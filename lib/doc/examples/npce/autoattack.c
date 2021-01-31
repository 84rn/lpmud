/*
 * Nawiedzona Feministka.
 *
 * Przyklad obrazujacy uzycie funkcji z modulu '/std/act/attack.c'
 * (standardowo dziedziczonego przez '/std/monster.c')
 *
 * Feministka bedzie autoatakowala wszystkie humanoidalne istoty plci meskiej.
 *
 * Styczen'99.
 */

inherit "/std/monster.c";

#include <macros.h>
#include <stdproperties.h>
#include <money.h>
#include <ss_types.h>

int czy_atakowac();

void
create_monster()
{
    ustaw_odmiane_rasy(PL_KOBIETA);
         
    dodaj_przym("dumny", "dumni");
    dodaj_przym("nawiedzony", "nawiedzeni");
    
    set_gender(G_FEMALE);
    
    set_long("Owa szczupla kobieta jest sredniego wzrostu. Jej sympatyczna, " +
	"choc dumna i juz nie pierwszej mlodosci twarz maci dziwny grymas. " +
	"Krotko sciete, jasne wlosy stercza buntowniczo we wszystkie strony. "+
	"Najbardziej wymowne sa jej zmruzone oczy; przebija z nich " +
	"zlowrogi blysk - tak jak by caly swiat (albo przynajmniej jego " +
	"polowa) nadepnal jej na odcisk, a teraz zamierzala na nim calym " +
	"(albo raczej jego polowie) zemscic za wszystkie doznane krzywdy. " +
	"Ubrana jest w cwiekowany kaftan z czarnej skory oraz krotka, szara " +
	"plocienna spodnice.\n");
    
    set_act_time(10);
    add_act("emote rozglada sie wokol z bojowym grymasem wymalowanym na " +
        "twarzy.");
    add_act("emote wspomina niezliczone cierpienia, jakich doznala w swoim " +
        "zyciu.");
    add_act("emote przymyka oczy i z calej sily zwiera dlonie w piesci.");
    
    set_cchat_time(10);
    add_cchat("Ty gnido! Ile kobiet porzuciles juz w swym zyciu, he? " +
	"Odwdziecze ci sie w imieniu ich wszystkich z nawiazka!");
    add_cchat("Wy, zawszone psy, tylko powietrze marnujecie!");
    add_cchat("A masz! Popamietasz co to znaczy plec piekna!");
    add_cchat("Myslicie tylko o jednym!");
    add_cchat("Za grosz poczucia estetyki, gustu, delikatnosci... Rzygac mi " +
	"sie chce gdy o was mysle!");
    
    set_chat_time(10);
    add_chat("Czy nie widzialas zlotko tu gdzies ktoregos z tych psow, samcow?");
    add_chat("Zginal balwan w karczemnej bijatyce i zostawil mnie sama z " +
	"bachorem...");
    add_chat("Ja im wszystkim tego nie daruje!");
    add_chat("Niech tu tylko ktorys podejdzie - niech zazna twardej piesci " +
        "kobiecej sprawiedliwosci!");
    add_chat("Mistrz Silvathraec tez byla kobieta!");

    set_stats ( ({ 40, 55, 35, 35, 50, 60 }) );
    
    set_skill(SS_UNARM_COMBAT, 35);
    set_skill(SS_DEFENCE, 40);
    
    add_prop(LIVE_I_NEVERKNOWN, 1);
    
    add_prop(CONT_I_WEIGHT, 61000); // Gestosc ciala ludzkiego jest troche
    add_prop(CONT_I_VOLUME, 62000); // mniejsza od gestosci wody. ;-)
    add_prop(CONT_I_HEIGHT, 175);
    
    set_aggressive(&czy_atakowac());
    set_attack_chance(100); // Naprawde msciwi atakuja zawsze.
    set_npc_react(1); // Autoatakujemy bez wzgledu na to, czy to NPC czy gracz.
    
    set_whimpy(70);
}

int
czy_atakowac()
{
    /* Jesli istota jest humanoidem plci meskiej, to przystepujemy do ataku! */
    if (this_player()->query_humanoid() &&
        this_player()->query_gender() == G_MALE)
        return 1;
    else
    /* W przeciwnym razie - dajemy spokoj. */
        return 0;
}
