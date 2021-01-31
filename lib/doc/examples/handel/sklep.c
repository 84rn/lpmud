/*
 * Przyklad najprostszego sklepu.
 */

inherit "/std/room"; /* ten obiekt jest pokojem */
inherit "/lib/shop"; /* ten obiekt jest sklepem */

#include <stdproperties.h> /* standardowe wlasciwosci */

void
create_room()
{
    set_short("Przykladowy sklep.\n");
    set_long("Znajdujesz sie w przykladowym sklepie. Wydaje sie byc "+
        "bardzo... niewykonczony. Na scianie wisi jakas tabliczka. "+
        "Oprocz niej jest jeszcze wyjscie do magazynu, bedacego "+
        "na zachod stad.\n");
        
    config_default_trade(); /* ustawia standardowe wartosci */
    
    set_store_room("/doc/examples/handel/magazyn.c"); /* ktory obiekt jest 
    							naszym magazynem */

    add_item( ({ "instrukcje", "tabliczke" }), "Tabliczka ze wskazowkami "+
        "dla klientow.\n");
        
    add_cmd_item( ({ "instrukcje", "tabliczke" }), "przeczytaj",
        "@@standard_shop_sign");
    
    add_prop(ROOM_I_INSIDE, 1); /* to jest pokoj, a nie lokacja na zewnatrz */
    
    add_exit("magazyn.c", "zachod", "@@wiz_check", 1); /* do magazynu */
    				/*  Sprawdza czy gostek moze tam wejsc */
}

void
init()
{
    ::init(); /* MUSI byc wywolane w kazdym inicie */
    
    init_shop(); /* dodanie komend takich jak 'kup', 'sprzedaj' itp */
}
