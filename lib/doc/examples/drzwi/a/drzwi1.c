inherit "/std/door";

#include <pl.h>
#include "definicje.h"

void
create_door()
{
    ustaw_nazwe( ({ "drzwi", "drzwi", "drzwiom", "drzwi", "drzwiami", 
        "drzwiach" }), PL_NIJAKI_OS); 
        
    dodaj_przym("stalowy", "stalowi");
    
    set_other_room(SCIEZKA + "pokoj2.c");
    
    set_door_id(KOD_DRZWI);
    
    set_door_desc("Masz przed soba stalowe drzwi. Ich masywny wyglad "+
        "stwarza w tobie pewnosc, ze to co znajduje sie za nimi " +
        "jest calkowicie bezpieczne. Pod uchwytem widac zamek.\n");
    
    set_open_desc("Na wschodniej scianie daja sie zauwazyc otwarte stalowe "+
        "drzwi.\n");
    set_closed_desc("Na wschodniej scianie daja sie zauwazyc zamkniete "+
        "stalowe drzwi.\n");
        
    set_pass_command( ({"wschod", "e" }));
    set_pass_mess("przez stalowe drzwi na wschod");
    
    set_lock_command("zamknij");
    set_unlock_command("otworz");

    set_key(KOD_KLUCZA);
    set_lock_name("zamek");
}
