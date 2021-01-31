/* death_mark.c */
/* Mrpr 901122 */
/* Tintin 911031 */
/* Nick 920211 */

#include <stdproperties.h>
#include <macros.h>

#define DEATH_ROOM "/d/Standard/room/death/death_room"

inherit "/std/object";

void start_death();

/*
 * Function name: create_object
 * Description:   Create the deathmark
 */
void
create_object()
{
    add_prop(OBJ_I_WEIGHT, 0); /* 0 g */
    add_prop(OBJ_I_VOLUME, 0); /* 0 ml */
    add_prop(OBJ_I_VALUE, 0); /* 0 copper */
    add_prop(OBJ_I_NO_DROP, 1); /* Call drop when property tested */
    add_name("znacznik_smierci", PL_MIA);
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
#if 0    
    
//    badge = present("death_badge", my_host);
//    badge->start_reincarnate(my_host);
//    dnr = (badge ? (int) badge->query_dnr() : 0);

    
    write("You can see a dark hooded man standing beside your corpse.\n" +
"He is wiping the bloody blade of a wicked looking scythe with slow\n" +
"measured motions. Suddenly he stops and seems to look straight at you\n" +
"with his empty... no, not empty but.... orbs....\n\n");
    
    if (dnr == 0)
    {
	say("@@see_death1:" + file_name(this_object()) + "@@");
	write("Death says: COME WITH ME, MORTAL ONE!\n\n");
	write("He reaches for you and suddenly you find yourself in another place.\n\n");
	my_host->move(DEATH_ROOM, 1);
    }
    else
    {
	say("@@see_death2:" + file_name(this_object()) + "@@");
	write("Death says: COME WITH M....\n\n" +
"He suddenly looks closer at you, focusing his empty orbs on your very\n" +
"essence. After a long scrutinization he gives you a disgusted look.\n\n" +
"Death says: DON'T YOU THINK ONCE IS ENOUGH? I KNEW THIS REINCARNATION\n" +
"BUSINESS WAS A BAD IDEA THE FIRST TIME I EVER HEARD OF IT!\n\n" +
"Death mumbles angrily as lifts you up and throw you on another short\n"+
"flight, to a by now rather well known place.\n\n");
        badge->set_dnr(badge->query_dnr() + 1);
	badge->start_reincarnate(my_host);
	remove_object();
    }

#endif
}

see_death1()
{
    if (previous_object()->query_wiz_level())
	return "You see Death with scythe and all, fetching a poor soul!\n";
    else
	return "You see a dark shape gathering some mist... or maybe you're just imagining that.\n";
}

see_death2()
{
    if (previous_object()->query_wiz_level())
	return "Death appears mumbles something about 'reincarnation' and leaves again.\n";
    else
	return "A dark shape materializes.... and dematerializes.... strange...\n";
}

/*
 * Function name: query_auto_load
 * Description:   Automatic load of this object
 */
query_auto_load()
{
    return MASTER_OB(this_object());
}
