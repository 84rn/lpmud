inherit "/std/door";

#include <pl.h>
#include "definicje.h"

void
create_door()
{
    ustaw_nazwe( ({ "drzwi", "drzwi", "drzwiom", "drzwi", "drzwiami", 
        "drzwiach" }), PL_NIJAKI_OS); 
        
    dodaj_przym("drewniany", "drewniani");
    
    set_other_room(SCIEZKA + "pokoj2.c");
    
    set_door_id(KOD_DRZWI);
    
    set_door_desc("Masz przed soba drewnine drzwi. Niezbyt dobrze zbita "+
       "kupa desek i nic wiecej. Zuwazasz zasuwke, ktora mozesz "+
       "zamknac drzwi.\n");
    
    set_open_desc("Na wschodniej scianie daja sie zauwazyc otwarte drewniane "+
        "drzwi.\n");
    set_closed_desc("Na wschodniej scianie daja sie zauwazyc zamkniete "+
        "drewniane drzwi.\n");
        
    set_pass_command( ({"wschod", "e" }));
    set_pass_mess("przez drewniane drzwi na wschod");
    
    set_lock_command("zamknij");
    set_unlock_command("otworz");

    set_lock_mess( ({ "zasuwa zasuwke w drewnianych drzwiach.\n", 
        "Slyszysz jakis szczek po drugiej stronie drzwi... jakby ktos " +
        "przesuwal zasuwke?\n", "Zasuwasz zasuwke w drewnianych drzwiach.\n" }));

    set_unlock_mess( ({ "odsuwa zasuwke w drewnianych drzwiach.\n", 
         "Slyszysz jakis szczek po drugiej stronie drzwi... jakby ktos " +
         "przesuwal zasuwke?\n", "Odsuwasz zasuwke w drewnianych drzwiach.\n" }));
        
/* Nazwa zamka w bierniku, bo sluzy wylacznie do komendy obejrzyj.
 */
    set_lock_name("zasowke"); 
    set_lock_desc("Ohh, Zwykla zasuwka... taka jak w kazdym przykladzie. "+
       "Z pewnoscia nie jest w stanie za duzo wytrzymac.\n");
}
