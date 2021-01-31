inherit "/std/armour.c";

#include <pl.h>
#include <wa_types.h>

create_armour()
{
    /* Nie musimy ustawiac nazwy, gdyz setup_shield() zrobi to za nas.
     * Wystarcza tylko przymiotniki i dlugi opis.
     */

    dodaj_przym("niewielki", "niewielcy");
    set_long("Dlugi opis tarczy. Piszac go, warto zasugerowac sie opisem " +
        "w 'man tarcze'.\n");

    /* Wreszcie definiujemy rodzaj tarczy i jej ewentualne ac. W tym
     * przypadku, jak mowi nam 'man tarcze', az powinno byc zerowe.
     */
    setup_shield(SH_LEKKA, 0);
    
    /* I to wszystko!
     * Prawda, ze proste?
     */
}
