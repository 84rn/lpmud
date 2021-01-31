/* 
 * /std/room/description.c
 *
 * This is a sub-part of /std/room.c
 *
 * In this module you will find the things relevant to the description of the
 * room.
 */

#include <composite.h>
#include <language.h>
#include <login.h>
#include <mudtime.h>
#include <ss_types.h>
#include <stdproperties.h>

static  mixed   room_descs;        /* Extra longs added to the rooms own */
static  int     searched;          /* Times this room has been searched */
static  string *herbs;             /* WHat herbs grows in this room? */

/*
 * Function name: 	add_my_desc
 * Description:   	Add a description printed after the normal
 *                      longdescription.
 * Arguments:	  	str: Description as a string
 *			cobj: Object responsible for the description
 *			      Default: previous_object()
 */
public void
add_my_desc(string str, object cobj = previous_object())
{
    if (query_prop(ROOM_I_NO_EXTRA_DESC))
	return;

    if (!room_descs)
	room_descs = ({ cobj, str });
    else
	room_descs = room_descs + ({ cobj, str });
}

/*
 * Function name:       change_my_desc
 * Description:         Change a description printed after the normal
 *                      longdescription. NOTE: if an object has more than
 * 			one extra description only one will change.
 * Arguments:           str: New description as a string
 *                      cobj: Object responsible for the description
 *                            Default: previous_object()
 */
public void
change_my_desc(string str, object cobj = previous_object())
{
    int i;
    mixed tmp_descs;

    if (query_prop(ROOM_I_NO_EXTRA_DESC))
	return;

    if (!cobj)
        return;

    i = member_array(cobj, room_descs);

    if (i < 0)
	add_my_desc(str, cobj);
    else
	room_descs[i + 1] = str;
}

/*
 * Function name: 	remove_my_desc
 * Description:   	Removes an earlier added  description printed after
 *                      the normal longdescription.
 * Arguments:	  	cobj: Object responsible for the description
 *			      Default: previous_object()
 */
public void
remove_my_desc(object cobj = previous_object())
{
    int i, sf;

    if (query_prop(ROOM_I_NO_EXTRA_DESC))
	return;

    sf = objectp(cobj);

    i = member_array(cobj, room_descs);
    while (i >= 0)
    {
	if (sf)
	    room_descs = exclude_array(room_descs, i, i + 1);
	else
	    room_descs = exclude_array(room_descs, i - 1, i);
	i = member_array(cobj, room_descs);
    }
}

/*
 * Function name: 	query_desc
 * Description:   	Gives a list of all added descriptions to this room
 * Returns:       	Array on the form:
 *			    desc1, obj1, desc2, obj2, ..... descN, objN
 */
public mixed
query_desc()
{
    return slice_array(room_descs, 0, sizeof(room_descs));
}

/*
 * Function name: exits_description
 * Description:   This function will return the obvious exits described
 *		  in a njice way.
 * Returns:	  The obvious exits
 */
public string
exits_description()
{
    string *exits;
    int size;

    exits = query_obvious_exits();

    switch(size = sizeof(exits))
    {
        case 0:
            return "";
        case 1:
            return "Jest tutaj jedno widoczne wyjscie: " + exits[0] + ".\n";
        case 2:
        case 3:
        case 4:
	    return "Sa tutaj " + LANG_SNUM(size, PL_MIA, PL_NIJAKI_NOS)
	         + " widoczne wyjscia: " + COMPOSITE_WORDS(exits) + ".\n";
        default:
            return "Jest tutaj " + LANG_SNUM(size, PL_MIA, PL_NIJAKI_NOS)
                 + " widocznych wyjsc: " + COMPOSITE_WORDS(exits) + ".\n";
    }

    return "Powazny blad w exits_description()! Zglos globalny blad. " +
        "TERAZ!!\n";
}

/*
 * Function name: long
 * Description:   Describe the room and possibly the exits
 * Arguments:	  str: name of item or 0
 * Returns:       A string holding the long description of the room.
 */
varargs public mixed
long(string str)
{
    int index;
    int size;
    mixed lg;

    lg = ::long(str);
    if (stringp(str))
	return lg;
    if (!stringp(lg))
	lg = "";

    /* This check is to remove extra descriptions that have been added by
     * an object that is now destructed.
     */
    while ((index = member_array(0, room_descs)) >= 0)
    {
	room_descs = exclude_array(room_descs, index, index + 1);
    }

    if (pointerp(room_descs))
    {
	index = -1;
	size = sizeof(room_descs);
	while((index += 2) < size)
	{
	    lg = lg + check_call(room_descs[index]);
	}
    }

    if (room_no_obvious)
    {
	return lg;
    }

    return lg + exits_description();
}

/*
 * Function name: calc_pros
 * Description:   Finding formula, positive side effects
 * Arguments:     player - The herbalist
 * Returns:       A number
 */
int
calc_pros(object player)
{
    int p;

    /*
     * Players that are members of certain types of guilds get better chances.
     */
    if (player->query_guild_style_occ() == "cleric") p = p + 15;
    if (player->query_guild_style_lay() == "cleric") p = p + 10;
    if (player->query_guild_style_occ() == "ranger") p = p + 10;
    if (player->query_guild_style_lay() == "ranger") p = p + 5;
    p = p + player->query_skill(SS_HERBALISM);

    /* Penalty will be given if no skill. */
    /* To add a certain element of luck  - wise players might get lucky. */

    p = p + random(player->query_stat(SS_WIS) / 3);

    return p;
}

/*
 * Function name: calc_cons
 * Description:   Finding formula, negative side effects
 * Arguments:     player - The herbalist
 * Returns:       A number ;-)
 */
int
calc_cons(object player)
{
    int p;

    /* If no herbalism skill, players will really have problems. */
    if (!player->query_skill(SS_HERBALISM))
        p = p + 15;

    /* Extra penalty if the player cannot see in the room or is blind */
    if (!CAN_SEE_IN_ROOM(player) || !CAN_SEE(player, this_object()))
        p = p + 45;

    /* Penalty increases for each time the room has been searched.
     * This will of course be sad for those who comes to the room after
     * a non-skilled searcher has tried to search some times, but this
     * could indicate that the first person trampled the herbs underfoot :-)
     */
    p = p + searched * searched * 5;

    /* If we have good luck, we can also have bad luck....
     * ....and stupid players with low intelligence have more bad luck ;-)
     */
    p = p + random((100 - player->query_stat(SS_INT)) / 3);

    return p;
}

/*
 * Nazwa funkcji : no_find
 * Opis          : Zwraca komunikat jaki otrzymuje gracz jesli nie znajdzie
 *                 zadnych ziol.
 */
static string
no_find(string str = 0)
{
    return str ?
        "Nie znajdujesz zadnych ziol. " + str + "\n" :
        "Szukasz wszedzie, ale nie znajdujesz zadnych ziol.\n";
}

/*
 * Function name: search_for_herb
 * Description:   The herbalist has searched the room, now let's see if and
 *                what he has found.
 * Arguments:	  herb_file - the file of the herb the player is looking for 
 *                            (optional)
 * Returns:       The message to write
 */
static string
search_for_herbs(string herb_file = 0)
{
    object herb;

    if (!sizeof(herbs))
    {
        switch (query_prop(ROOM_I_TYPE))
        {
            case ROOM_IN_WATER:
            case ROOM_UNDER_WATER:
                return no_find("Moze poszloby ci lepiej, gdybys szukal" +
                       this_player()->koncowka("", "a") + " ich na suchym "
                     + "ladzie.");
            case ROOM_IN_AIR:
                return no_find("Moze poszloby ci lepiej, gdybys szukal" +
                       this_player()->koncowka("", "a") + " ich na bardziej "
                     + "twardym gruncie.");
        }
        if (query_prop(ROOM_I_INSIDE))
            return no_find("Moze poszloby ci lepiej, gdybys szukal" +
                   this_player()->koncowka("", "a") + " ich na otwartym "
                 + "terenie.");
        return no_find("");
    }

    if (!herb_file)
        herb_file = herbs[random(sizeof(herbs))];
    else if (!herb_file->do_id_check(this_player()))
        return no_find();

    if ((calc_pros(this_player()) - calc_cons(this_player()) -
                (herb_file->query_find_diff() * 10 - 50)) <= 0)
    {
        searched += 1;
        return no_find();
    }

    herb = clone_object(herb_file);
    say(QCIMIE(this_player(), PL_MIA) + " znajduje jakies ziola.\n");
    searched += 2;

    if (herb->move(this_player()))
    {
        herb->move(this_object(), 1);
        herb->start_decay();
        return "Znajdujesz " + herb->short(PL_BIE) + ", ale nie jestes "
             + "w stanie " + herb->koncowka("go", "jej", "go", "ich", "ich")
             + " ze soba zabrac.\n";
    }
    return "Znajdujesz " + herb->short(PL_BIE) + ".\n";
}

/*
 * Function name: add_herb_file
 * Description:   Add a herb file to our array
 * Arguments:     file - The filename to our herb
 * Returns:       1 if added
 */
int
add_herb_file(string file)
{
    if (!file)
        return 0;

    if (!herbs)
        herbs = ({ file });
    else
        herbs += ({ file });
    return 1;
}

/*
 * Function name: query_herb_files
 * Description:   Query the herb files
 * Returns:       The herb array or 0.
 */
string *
query_herb_files()
{
    return herbs;
}

/*
 * Function name: set_searched
 * Description:   Set the searched times number, perhaps there grow some new
 *                herbs at reset?
 * Arguments:     i - The new search number
 */
void
set_searched(int i)
{
    searched = i;
}

/*
 * Function name: query_searched
 * Description:   Query for the search number
 * Returns:       The search number
 */
int
query_searched()
{
    return searched;
}

/*
 * Nazwa funkcji : herb_search
 * Opis          : Sprawdza, czy gracz szuka ktoregos z rosnacych w lokacji
 *                 gatunkow ziol.
 * Argumenty     : str - Argument komendy 'szukaj'.
 * Zwraca        : Jak search_fun().
 */
static string
herb_search(string str)
{
    string *herb_names = ({});
    int i = -1;
    string *herbs = this_object()->query_herb_files();
    int size = sizeof(herbs);

    if (str == "ziol")
        return search_for_herbs();

    while (++i < size)
        if (str == herbs[i]->query_nazwa_ziola(PL_BIE))
            return search_for_herbs(herbs[i]);

    return 0;
}

/*
 * Nazwa funkcji : search_fun
 * Opis          : Sprawdza, czy gracz szuka jakiegos standardowego dla
 *                 lokacji obiektu (takiego jak ziola czy chrust).
 */
static string
search_fun(string str, int trail)
{
    string herbsearch;

    if (!trail && (herbsearch = herb_search(str)))
        return herbsearch;

    return ::search_fun(str, trail);
}

/*
 * Function name: track_now
 * Description:   Actually perform the tracking
 * Arguments:     pl - the tracker
 *                skill - the tracking skill used
 */
void
track_now(object player, int track_skill)
{
    string *track_arr,
            result = "Nie znajdujesz zadnych sladow.\n",
            dir,
           *dir_arr,
            race,
           *races = RACES + ({ "jakies zwierze" });
    int     i;
    mixed  *exits;
    
    if (!player)
        return ;

    track_arr = query_prop(ROOM_AS_DIR);

    // just in case, but presently, ROOM_I_INSIDE prevents setting of ROOM_AS_DIR
    if (query_prop(ROOM_I_INSIDE))
        track_skill -= 50;

    track_skill /= 2;
    track_skill += random(track_skill);

    if (CAN_SEE_IN_ROOM(player) && pointerp(track_arr) && track_skill > 0)
    {
        dir = track_arr[0];
        if (dir == "X" || dir == "M")
            dir = "do nikad";
/*
        if (strlen(dir)>5)
        {
            dir_arr = explode(dir," ");
            if (dir_arr[0] != "the")
            dir = "the " + dir;
        }
 */
        race = track_arr[1];
        switch (race)
        {
            case "elfke": race = "elfa"; break;
            case "krasnoludke": race = "krasnoluda"; break;
            case "hobbitke": race = "hobbita"; break;
            case "goblinke": race = "goblina"; break;
            case "orczyce": race = "orka"; break;
            case "ogrzyce": race = "ogra"; break;
            case "halflinke": race = "halflinga"; break;
            case "gnomke": race = "gnoma"; break;
        }
        
        result = "Jestes w stanie wyroznic kilka sladow na ziemi.\n";

        switch(track_skill)
        {
            case  1..10:
                break;
            case 11..20:
                if(random(2))
                {
                    exits = query_exit();
                    if(i = sizeof(exits))
                    {
			i = random(i / 3) * 3 + 1;
			if (pointerp(exits[i]))
			    dir = exits[i][1];
			else
			    dir = exits[i];
                    }
                }
                result += "Najswiezsze prowadza prawdopodobnie " + dir + ".\n";
                break;
            case 21..50:
                result += "Najswiezsze prowadza " + dir + ".\n";
                break;
            case 51..75:
                if(random(2))
                    race = ODMIANA_RASY[races[random(sizeof(races))]][0][PL_BIE];
                
                result += "Najswiezsze zostaly pozostawione " +
                    "prawdopodobnie przez " + race + " i prowadza " + dir + 
                    ".\n";
                break;
            case 76..150:
                result += "Najswiezsze zostaly pozostawione przez " + race +
                    " i prowadza " + dir + ".\n";
                break;
        }
    }

    player->catch_msg(result);
    player->remove_prop(LIVE_S_EXTRA_SHORT);
    tell_roombb(environment(player), QCIMIE(player, PL_MIA) + " wstaje.\n",
                ({player}), player);
    return;
}

/*
 * Function name: track_room
 * Description:   Someone looks for tracks in this room
 */
void
track_room()
{
    int     time,
            track_skill;
    object  paralyze;

    time = query_prop(OBJ_I_SEARCH_TIME);
    if (time < 1)
        time = 10;
    else
        time += 5;

    track_skill = this_player()->query_skill(SS_TRACKING);
    time -= track_skill/10;

    if (time < 1)
        track_now(this_player(), track_skill);
    else
    {
        set_alarm(itof(time), 0.0, &track_now(this_player(), track_skill));

        seteuid(getuid());
        paralyze = clone_object("/std/paralyze");
        paralyze->set_standard_paralyze("tropic");
        paralyze->set_stop_fun("stop_track");
        paralyze->set_stop_verb("przestan");
        paralyze->set_stop_message("Przestajesz szukac sladow.\n");
        paralyze->set_remove_time(time);
        paralyze->set_fail_message("Jestes zajet" + 
            (this_player()->query_gender() == G_FEMALE ? "a" : "y") +
            " szukaniem sladow. Wpisz 'przestan', jesli chcesz zrobic cos " +
            "innego.\n");
        paralyze->move(this_player(),1);
    }
 }

/*
 * Function name: stop_track
 * Description:   interrupt tracking
 * Arguments:
 * Returns:
 */
varargs int
stop_track(mixed arg)
{
    if (!objectp(arg))
    {
	mixed *calls = get_all_alarms();
	mixed *args;
	int i;

	for (i = 0; i < sizeof(calls); i++)
	    if (calls[i][1] == "track_now")
	    {
		args = calls[i][4];
		if (args[0] == this_player())
		    remove_alarm(calls[i][0]);
	    }
    }
    saybb(QCIMIE(this_player(), PL_MIA) + " przestaje szukac sladow.\n");
    this_player()->remove_prop(LIVE_S_EXTRA_SHORT);

    return 0;
}

/*
 * Nazwa funkcji  : check_time
 * Opis           : Definiuje komunikat dla komendy 'czas'. Powinna byc
 *                  maskowana w domenach, stosownie do lokalnego kalendarza.
 * Funkcja zwraca : Pelen komunikat lub 0 (w tym drugim przypadku uzywany
 *                  jest tekst standardowy).
 */
public string
check_time()
{
    return 0;
}

/*
 * Nazwa funkcji : pora_dnia
 * Opis          : Zwraca obecna pore dnia w lokacji, zgodnie z definicjami
 *                 z <mudtime.h>.
 */
public int
pora_dnia()
{
    return MT_POPOLUDNIE;
}

/*
 * Nazwa funkcji : pora_roku
 * Opis          : Zwraca obecna pore roku w lokacji, zgodnie z definicjami
 *                 z <mudtime.h>.
 */
public int
pora_roku()
{
    return MT_LATO;
}

/*
 * Nazwa funkcji  : dzien_noc
 * Opis           : Sprawdza, czy w lokacji jest obecnie dzien, czy noc.
 * Funkcja zwraca : 0, jesli dzien; 1, jesli noc.
 */
public int
dzien_noc()
{
    switch (pora_dnia())
    {
        case MT_WCZESNY_RANEK:
        case MT_RANEK:
        case MT_POLUDNIE:
        case MT_POPOLUDNIE:
        case MT_WIECZOR:
            return 0;
        case MT_POZNY_WIECZOR:
        case MT_NOC:
        case MT_SWIT:
            return 1;
        default:
            throw("Illegal value returned: pora_dnia() = " + pora_dnia()
                + ".\n");
    }
}
