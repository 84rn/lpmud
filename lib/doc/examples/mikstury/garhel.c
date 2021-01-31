inherit "/std/potion.c";

#include <pl.h>
#include <herb.h>

create_potion()
{
/*
 * Nazwa 'garhel' raczej ciezko sie odmienia, wiec sie nie wysilamy :)
 * z rodzajem podobnie, wiec podajemy pierwszy lepszy 
 */
    ustaw_nazwe_mikstury( ({ "garhel", "garhel", "garhel", "garhel",
        "garhel", "garhel" }), PL_ZENSKI );
        
    dodaj_przym("zoltawy", "zoltawi");
        
    set_id_long("Jest to Garhel, bardzo trudna do zrobienia i rzadko "+
        "spotykana mikstura. "+
        "Masz wielkie szczescie, ze znajdujesz sie w jej posiadaniu. "+
        "Ma ponoc bardzo silne dzialanie leczace. "+
        "Otrzymuje sie ja z ziola Blumpka oraz z niewielkiej ilosci "+
        "alkoholu.\n");
        
    set_unid_long("Jakis dziwny wywar ziolowy.\n");
    
    set_id_smell("Pachnie charakterystyczna wonia Blumpki, oraz zalatuje "+
        "lekko spitytusem.\n");
        
    set_unid_smell("Ciezko powiedziec... wydaje ci sie, ze raczej nie ma "+
        "zapachu. Zaraz... cos jakby spirytus ?\n");
        
    set_id_taste("Wyczuwasz znajomy ci smak alkoholu, oraz jakby hmm... "+
        "to musi byc Blumpka!\n");
        
    set_unid_taste("Wyczuwasz lekki posmak alkoholu...\n");
    
    set_effect(HERB_HEALING, "hp", 150);
    
}