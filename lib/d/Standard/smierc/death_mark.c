/* death_mark.c */
/* Mrpr 901122 */
/* Tintin 911031 */
/* Nick 920211 */

#include <stdproperties.h>
#include <macros.h>

#define DEATH_ROOM "/d/Standard/smierc/death_room"

inherit "/std/object";

void start_death();

/*
 * Function name: create_object
 * Description:   Create the deathmark
 */
void
create_object()
{
    add_name("_default_death_");
    add_prop(OBJ_I_WEIGHT, 0); /* 0 g */
    add_prop(OBJ_I_VOLUME, 0); /* 0 ml */
    add_prop(OBJ_I_VALUE, 0); /* 0 copper */
    add_prop(OBJ_I_NO_DROP, 1); /* Call drop when property tested */
    set_no_show();
}

/*
 * Function name: init
 * Description:   Init this object
 */
init()
{
    /* 
     * When the object autoloads it must not start at once
     */
    set_alarm(1.0, 0.0, &start_death());  
}

/*
 * Function name: start_death
 * Description:   Start the death sequence.
 */
void
start_death()
{
    int dnr;
    object ned, my_host, badge;
    
    my_host = environment(this_object());
    
    if (!my_host || !living(my_host) || !my_host->query_ghost())
    {
	remove_object();
	return;
    }
    
    my_host->move(DEATH_ROOM, 1);
    
    return ;
}

/*
 * Function name: query_auto_load
 * Description:   Automatic load of this object
 */
query_auto_load()
{
    return MASTER_OB(this_object());
}
