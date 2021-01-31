inherit "/std/door";

#include <pl.h>
#include "definicje.h"

void
create_door()
{
    ustaw_nazwe( ({ "drzwi", "drzwi", "drzwiom", "drzwi", "drzwiami", 
        "drzwiach" }), PL_NIJAKI_OS); 
        
    dodaj_przym("drewniany", "drewniani");
    
    set_other_room(SCIEZKA + "pokoj1.c");
    
    set_door_id(KOD_DRZWI);
    
    set_door_desc("Masz przed soba drewnine drzwi. Niezbyt dobrze zbita "+
       "kupa desek i nic wiecej. Nie spostrzegasz niczego, co sluzyloby "+
       "za zamek. Te drzwi pewnie sa zamykane tylko od jednej strony - "+
       "tej przeciwnej..\n");
    
    set_open_desc("Na zachodniej scianie daja sie zauwazyc otwarte stalowe "+
        "drzwi.\n");
    set_closed_desc("Na zachodniej scianie daja sie zauwazyc zamkniete "+
        "stalowe drzwi.\n");
        
    set_pass_command( ({"zachod", "w" }));
    set_pass_mess("przez drewniane drzwi na zachod");
}