
/* Sloj na ziola
 * Pshoddy
 */

inherit "/std/receptacle.c";

#include <stdproperties.h>
#include <pl.h>

create_container()
{
    set_long("Zwykly sloik do przechowywania ziol.\n");


    ustaw_nazwe( ({ "sloik", "sloika", "sloikowi", "sloik", 
        "sloikiem", "sloiku" }), ({ "sloiki", "sloikow",
        "sloikom", "sloiki", "sloikami", "sloikach" }) , 
        PL_MESKI_NOS_NZYW);
        

    dodaj_przym( "szklany", "szklani" );
    
    add_prop(CONT_I_MAX_WEIGHT, 1000);
    add_prop(CONT_I_MAX_VOLUME, 1000);
    add_prop(CONT_I_RIGID,1);
    add_prop(CONT_I_TRANSP,1);
    
    
}


/* Tylko ziola mozna wkladac do naszego sloika.*/
 
int
prevent_enter(object ob)
{
  if (function_exists("create_object",ob) == "/std/herb")
     { 
         return 0; 
     }
  return 1;
}         

/* Ziola w sloiku nie wiedna... */
  
void
enter_inv(object ob, object from)
{
  ::enter_inv(ob, from);
  ob->stop_decay();
}

/* ...ale poza nim tak. */

void
leave_inv(object ob, object to)
{
  ::leave_inv(ob, to);
  ob->restart_decay();
}    


  
  
