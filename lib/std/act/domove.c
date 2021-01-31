
/* 
 *  /std/act/domove.c
 *
 *  Random walk: Standard action module for mobiles.
 */

#pragma save_binary
#pragma strict_types

static	int	 monster_ranmove,	/* Intervall between walks  */
		 monster_follow_flag,	/* Do we always remember followed ppl? */
		 monster_follow_alarm;	/* Alarm id for following */
static  string	*monster_restrain_path,	/* Delimiting path names    */
		 monster_home;		/* The home of the monster  */
static	object	 monster_follow;	/* Object of person to follow */
static	float	 monster_follow_time;	/* Following speed	    */

#define SEQ_RWALK "_mon_ranwalk"
#define SEQ_FOLL  "_mon_ranwalk"

public void unset_follow();
public void do_follow(string com);

/*
 * Function name: set_random_move
 * Description:   Set the ability to walk around, and the
 *                time affecting limit.
 * Arguments:	  time - The monster will move with 10 + random(time);
 *                flag - True if you want the monster to wander forever. If
 *                       0 or not present, the monster will wander randomly
 *                       for as long as it meets players and a short time
 *                       after that. The preferred behavior is to not wander
 *                       forever.
 */
varargs void
set_random_move(int time, int flag)
{
    monster_ranmove = time;
    if (!this_object()->seq_query(SEQ_RWALK))
    {
	this_object()->seq_new(SEQ_RWALK, flag);
    }
    this_object()->seq_clear(SEQ_RWALK);
    this_object()->seq_addfirst(SEQ_RWALK, "@@monster_ranwalk");
}

/*
 * Nazwa funkcji : set_follow
 * Opis          : Sprawia by NPC godzin kogos
 * Argumenty     : object - obiekt gonionego
 *		   float - czas pomiedzy ucieczka a gonieniem
 *		   int - 0, gdy NPC ma przestac gonic, gdy zgubi gonionego,
 *			 1, jesli ma zawsze gonic.
 */
varargs void
set_follow(object followed, float czas, int flag = 0)
{
    if (!followed)
    {
        unset_follow();
    }
    
    if (czas < 0.1)
        czas = 0.1;

    monster_follow = followed;
    monster_follow_time = czas;
    monster_follow_flag = flag;

    followed->add_following(this_object());
}

object
query_follow()
{
    return monster_follow;
}

/*
 * Function name: unset_follow
 * Description:   Stop following
 */
void
unset_follow()
{
    if (monster_follow)
	monster_follow->remove_following(this_object());
    monster_follow = 0;
    return ;
}

/*
 * Function name:   set_restrain_path
 * Description:	    Set the path that delimits the exits the monster will
 *		    choose. Exits of rooms that begin with the pathname
 *		    are elegible as random walking exits. The others are
 *		    not considered. If a monster gets stuck this way, it
 *		    will teleport home, which is a room that can be set
 *		    with set_monster_home().
 * Arguments:	    path: Either a path or an array of paths that form the
 *			  begin of legal room names.
 * See also:	    set_monster_home(), query_restrain_path()
 */
void
set_restrain_path(mixed path)
{
    if (!path)
	monster_restrain_path = ({ });
    else if (stringp(path))
	monster_restrain_path = ({ path });
    else if (pointerp(path))
	monster_restrain_path = path;
}

/*
 * Function name:   query_restrain_path
 * Description:     Give back the restrain_path (if any)
 * Returns:	    An array with pathstrings.
 * See also:	    set_monster_home(), set_restrain_path()
 */
string *
query_restrain_path()
{
    if (!monster_restrain_path)
	return ({ });
    else
	return monster_restrain_path;
}

/*
 * Function name:   set_monster_home
 * Description:	    Set the room filename to which the monster will teleport
 *		    if it managed to get itself stuck, that is, if it ended
 *		    up in a room with no elegible exits.
 * Arguments:	    The filename of the room.
 * See also:	    set_restrain_path(), query_restrain_path()
 */
void
set_monster_home(string home)
{
    monster_home = home;
}

/*
 * Function name:   query_monster_home
 * Description:     Return the filename of the home to which a monster will
 *		    teleport if it got itself stuck.
 * Returns:	    That filename.
 * See also:	    set_monster_home(), set_restrain_path()
 */
string
query_monster_home()
{
    return monster_home;
}

/*
 * Function name:   filter_exits
 * Description:	    Filter out all non-wanted exits of a given array.
 *		    Non-wanted exits are exits that do not fall within the
 *		    given path that was set with set_restrain_path().
 * Arguments:	    exits: An array of exits, as delivered by query_exits().
 * Returns:	    0 if no exits, or an array with legal exits.
 */
mixed *
filter_exits(mixed *exits)
{
    string tmp, *path_arr;
    mixed *res_exits;
    int i, j;

    if (!exits || !sizeof(exits))
	return ({ });

    if (!monster_restrain_path || !sizeof(monster_restrain_path))
	return exits;

    seteuid(getuid());
    res_exits = ({ });
    for (i = 0; i < sizeof(exits); i += 3)
    {
	for (j = 0; j < sizeof(monster_restrain_path); j++)
	{
	    if (sscanf(exits[i], monster_restrain_path[j] + "%s", tmp))
	    {
		res_exits += exits[i..i+2];
		break;
	    }
	}
    }
    return res_exits;
}

public void
hook_follow_me(string com)
{
    if (previous_object() != monster_follow)
        return ;
    
    if (monster_follow_alarm)
    {
        if (!monster_follow_flag)
            unset_follow();
            
        return ;
    }

    monster_follow_alarm = set_alarm(monster_follow_time, 0.0, &do_follow(com));
}

public void
hook_follow_you_lost_me()
{
    if (previous_object() != monster_follow)
        return ;
        
    unset_follow();
}

public void
do_follow(string com)
{
    this_object()->command("$" + com);
    monster_follow_alarm = 0;
}

/*
 * Sequence functions
 */

/*
 * Function name:   monster_ranwalk
 * Description:	    Add a random direction to walk. Add the function call too.
 */
void
monster_ranwalk()
{
    mixed *exits;
    int il;
    string ex;

    if (!environment(this_object()))
	return;

    this_object()->seq_clear(SEQ_RWALK);

    if (this_object()->query_attack() || 
        (monster_follow && present(monster_follow, environment(this_object()))))
    {
        il = 5 + random(monster_ranmove);
        this_object()->seq_addfirst(SEQ_RWALK, ({ il, "@@monster_ranwalk" }) );
        return;
    }

    exits = filter_exits(environment(this_object())->query_exit());

    if (!sizeof(exits))
    {
        if (monster_home)
        {
            seteuid(getuid(this_object()));
            this_object()->move_living("do domu", monster_home);
        }
        il = 7 + random(monster_ranmove);
        this_object()->seq_addfirst(SEQ_RWALK, ({ il, "@@monster_ranwalk" }) );
        return;
    }

    il = random(sizeof(exits)) / 3;
    
    if (pointerp(exits[il * 3 + 1]))
        ex = exits[il * 3 + 1][0];
    else
        ex = exits[il * 3 + 1];

    il = 7 + random(monster_ranmove);
    this_object()->seq_addfirst(SEQ_RWALK,
        ({ ("@@oke_to_move|" + ex + "@@"), il, "@@monster_ranwalk" }) );
}

/*
 * Function name: oke_to_move
 * Description:   Checks whether the npc is fighting someone, if he is in
 *                in combat, the move-command will be delayed till the 
 *                war is over.
 * Arguments:	  exit  : the exit that is generated for the monster to take.
 * Returns:       0     : if in combat
 *                string: the exit that the monster takes if not in combat.
 */
mixed
oke_to_move(string exit)
{
    if (this_object()->query_attack())
        return 0;

    return exit;
}

/*
 * Function name: 
 * Description:	  
 * Arguments:	  
 * Returns:	  
 */
