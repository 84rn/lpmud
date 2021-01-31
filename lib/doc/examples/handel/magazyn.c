/*
 * Przykladowy magazyn sklepowy
 */
 
inherit "/std/room"; /* obiekt jest pokojem */
inherit "/lib/store_support"; /* obiekt jest magazynem sklepowym */
#include <stdproperties.h> /* zebysmy mogli zdefiniowac ROOM_I_INSIDE */
 
void
create_room()
{
    set_short("Magazyn przykladowego sklepu.\n");
    set_long("Jestes w przykladowym magazynie. W tej lokacji umieszczane "+
        "sa wszystkie\nprzedmioty, ktore gracz sprzedal.\n");
    
    add_prop(ROOM_I_INSIDE, 1); /* To jest pomieszczenie */
    
    add_exit("sklep.c", "wschod"); /* Wyjscie do sklepu */ 
    
    /* Tutaj mozemy sklonowac kilka przedmiotow, jesli nie chcemy,
       by na poczatku sklep byl pusty.
    */
}

void         			   /* wywolywane za kazdym razem, jak jakis */
enter_inv(object ob, object skad)  /* obiekt wchodzi do wnetrza tego obiektu */
{				   /* ...czyli do pokoju */

    ::enter_inv(ob, skad); /* trzeba ZAWSZE wywolac gdy redefiniujemy enter_inv */
    
    store_update(ob); /* dba o to, zeby magazynie nie bylo za duzo rzeczy */
}

