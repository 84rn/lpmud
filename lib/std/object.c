/*
 * /std/object.c
 *
 * Contains all basic routines for configurable objects.
 */

#pragma save_binary
#pragma strict_types

inherit "/std/callout";

#include <composite.h>
#include <macros.h>
#include <ss_types.h>
#include <std.h>
#include <stdproperties.h>
#include <debug.h>

static string   *obj_shorts,    /* Odmiana short'a przez przypadki w lp. */
		*obj_pshorts,   /* Odmiana short'a przez przypadki w lmn. */
		obj_long,       /* Long description */
		obj_subloc,     /* Current sublocation */
		*obj_commands;	/* The commands for each command item */
static mixed	*obj_names = ({ ({}), ({}), ({}), ({}), ({}), ({}) }),
				/* The name(s) of the object */
		*obj_pnames = ({ ({}), ({}), ({}), ({}), ({}), ({}) }),
				/* The plural name(s) of the object */
		*obj_rodzaje = ({ ({}), ({}), ({}), ({}), ({}), ({}) }),
		*obj_prodzaje = ({ ({}), ({}), ({}), ({}), ({}), ({}) }),   
				/* Rodzaje gramatyczne nazw obiektu */
		obj_przym = ({ ({}), ({}) }),
				/* Lista przymiotnikow - mian lp i lmn */
		obj_items,	/* The items (pseudo look) of this object */
		obj_cmd_items,	/* The command items of this object */
		obj_state,      /* The internal state, used for light etc */
		magic_effects;  /* Magic items effecting this object. */
static int      obj_no_show,    /* Don't display this object. */
		obj_no_show_c,  /* Don't show this object in composite desc */
		obj_no_change,  /* Lock value for configuration */
		obj_rodzaj_shorta;
				/* Jak sama nazwa wskazuje */
static object   obj_previous;   /* Caller of function resulting in VBFC */
static mapping  obj_props;      /* Object properties */
private static int hb_index,    /* Identification of hearbeat callout */
		reset_interval; /* Constant used to set reset interval */
private static int search_alarm; /* Alarm used for searching */

/*
 * Prototypes
 */
public			void    add_prop(string prop, mixed val);
public  		void    remove_prop(string prop);
public			void    set_no_show_composite(int i);
public	varargs		void	add_name(mixed name, int przyp, mixed rodzaje);

public			int     search_hidden(object obj, object who);
public			int     is_live_dead(object obj, int what);
public	varargs		int	dodaj_nazwy(string *nazwy, mixed pnazwy, int rodzaj);
public  varargs	nomask	int	check_recoverable(int flag);

public  varargs 	mixed	check_call(mixed retval, object for_obj);
public  		mixed   query_prop(string prop);
public  varargs		mixed	query_adj(int arg, int przyp);
public	varargs		mixed	query_przym(int arg, int przyp, int rodzaj);
public	varargs		mixed	query_pprzym(int arg, int przyp, int rodzaj);

/* 
 * PARSE_COMMAND
 *
 * These lfuns are called from within the efun parse_command() to get the
 * three different sets of ids. If no plural ids are returned then the
 * efun will try to make pluralforms from the singular ids.
 *
 * If no normal ids are returned then parse_command will never find the object.
 */
public string *
parse_command_id_list(int przyp)
{
    return obj_names[przyp];
}
public int *
parse_command_rodzaj_id_list(int przyp)
{
    return obj_rodzaje[przyp];
}
public string *
parse_command_plural_id_list(int przyp)
{
    return obj_pnames[przyp];
}
public int *
parse_command_plural_rodzaj_id_list(int przyp)
{
    return obj_prodzaje[przyp];
}

public string *
parse_command_adjectiv1_id_list()
{
    return obj_przym[0];
}

public string *
parse_command_adjectiv2_id_list()
{
    return obj_przym[1];
}


/*
 * Function name: set_heart_beat
 * Description:   Emulate old heartbeat code
 * Arguments:     repeat - 1 to enable, 0 to disable
 * Returns:       Return value from set_alarm()
 */
nomask int
set_heart_beat(mixed repeat, string func = "heart_beat")
{
    float delay;
    int ret;
    object tp;
    
    if (intp(repeat))
	delay = itof(repeat * 2);
    else if (floatp(repeat))
	delay = repeat;
    else
	throw("Wrong argument 1 to set_heart_beat.\n");


	remove_alarm(hb_index);

    if (delay > 0.0)
    {
	tp = this_player();
	set_this_player(0);
	ret = set_alarm(delay, delay, mkfunction(func));
	set_this_player(tp);
    }
    return hb_index = ret;
}

/*
 * Function name: create_object
 * Description:   Create the object (Default for clones)
 */
public void
create_object()
{
    add_prop(OBJ_I_WEIGHT, 1000);       /* 1 Kg is default */
    add_prop(OBJ_I_VOLUME, 1000);       /* 1 l is default */
    obj_no_change = 0;
    obj_no_show = 0;

    dodaj_nazwy( ({ "obiekt", "obiektu", "obiektowi", "obiekt", "obiektem",
        "obiekcie" }), ({ "obiekty", "obiektow", "obiektom", "obiekty",
        "obiektami", "obiektach" }), PL_MESKI_NOS_NZYW);
}


/*
 * Function name: create
 * Description  : Object constructor, called directly after load / clone.
 *                It calls the public create function and sets the only
 *                default variable.
 */
public nomask void
create()
{
    string ob_name;
    
    ob_name = OB_NAME(this_object());

    obj_names = ({ ({ ob_name }), ({ ob_name }), ({ ob_name }),
		   ({ ob_name }), ({ ob_name }), ({ ob_name }) });
    obj_rodzaje = ({ ({ 4 }), ({ 4 }), ({ 4 }),
		     ({ 4 }), ({ 4 }), ({ 4 }) });

    this_object()->create_object();
}

#if 0
/*
 * Function name: reset_object
 * Description:   Reset the object (Default for clones)
 */
public void
reset_object() 
{
}
#endif

/*
 * Function name: reset
 * Description:   Reset the object (always called, used as constructor)
 * Arguments:     arg: The reset argument.
 */
public nomask void
reset()
{
    float reset_time = -log(rnd()) * 540000.00;
    mixed *calls = get_all_alarms();
    int index = sizeof(calls);
    
    while(--index >= 0)
      if (calls[index][1] == "reset")
          remove_alarm(calls[index][0]);

    if (!reset_interval)
	return;

    reset_time /= itof(reset_interval);
    if (reset_time < 0.0)
	reset_time = 0.0;

    if (function_exists("reset_object", this_object()))
	set_alarm(reset_time, 0.0, reset);

    this_object()->reset_object();
}

/*
 * Function name: enable_reset
 * Description  : Used to enable or disable resets in an object. The reset
 *                interval is based on the factor given as argument to this
 *                function. By default, the reset time will averagely be
 *                slightly larger than one hour. By using a factor, this period
 *                can be increased or decreased, using the following formula:
 *                    Reset interval = 60 minutes * (100 / factor)
 * Arguments    : (optional) int factor - when omitted, the factor will default
 *                to 100 (60 minutes). Use 0 to disable resets in this object.
 *                Valid values for the factor are in the range 10 to 200, which
 *                make for a reset interval of approximately 600 to 30 minutes
 *                on average.
 */
nomask void
enable_reset(int on=100)
{
    float reset_time;
    mixed *calls = get_all_alarms();
    int index;
    
    if (obj_no_change || (on && (on<10 || on>200)))
	return;

    reset_interval = on;

    index = sizeof(calls);
    while(--index >= 0)
        if (calls[index][1] == "reset")
 	    remove_alarm(calls[index][0]);

    if (reset_interval) {
	reset_time = -log(rnd()) * 540000.0 / itof(reset_interval);

	if (reset_time < 0.0)
	    reset_time = 0.0;

	if (function_exists("reset_object", this_object()))
	    set_alarm(reset_time, 0.0, reset);
    }
}

/*
 * Function name: get_this_object()
 * Description  : Always returns the objectpointer to this object.
 * Returns      : object - this_object()
 */
object
get_this_object()
{
    return this_object();
}

/*
 * Function name: update_actions
 * Description:   Updates our defined actions in all relevant objects.
 */
public void
update_actions()
{
    if (environment(this_object()))
	move_object(environment(this_object()));
}

/*
 * Nazwa funkcji : id
 * Opis          : Ta funkcja jest uzywana do sprawdzenia, czy dany obiekt
 *		   ma podana nazwe, w liczbie pojedynczuej, w podanym 
 *		   przypadku.
 * Argumenty     : string  - sprawdzana nazwa,
 *		   int     - [opcjonalnie, domyslnie mianownik] sprawdzany 
 *		             przypadek.
 * Funkcja zwraca: int 1/0 - prawda, jesli rzeczywiscie obiekt ma taka nazwe
 *			     w podanym przypadku w liczbie pojedynczej.
 */
public varargs int
id(string str, int przyp = PL_MIA)
{ 
     return (member_array(str, obj_names[przyp]) >= 0);
}

/*
 * Nazwa funkcji : plural_id
 * Opis          : Ta funkcja jest uzywana do sprawdzenia, czy dany obiekt
 *		   ma podana nazwe, w liczbie mnogiej, w podanym przypadku.
 * Argumenty     : string  - sprawdzana nazwa,
 *		   int     - [opcjonalnie, domyslnie mianownik] sprawdzany 
 *		             przypadek.
 * Funkcja zwraca: int 1/0 - prawda, jesli rzeczywiscie obiekt ma taka nazwe
 *			     w podanym przypadku w liczbie mnogiej.
 */
public varargs int
plural_id(string str, int przyp)
{
     return (member_array(str, obj_pnames[przyp]) >= 0);
}

/*
 * Nazwa funkcji : long
 * Opis          : Opisuje obiekt, lub jeden z pseudo-itemow zdefiniowanych
 *		   w nim. Rozpoznaje ewentualne VBFC.
 * Argumenty     : string str - pseudo-item do opisania. Jest to rzecz
 *				dodana przy uzyciu add_item. Jesli ta
 *				zmienna jest ustawiona na 0, zwrocony
 *				zostanie caly opis obiektu.
 *		   object for_obj - obiekt, dla ktorego przeznaczony jest
 *				    ten dlugi opis.
 * Funkcja zwraca: string - opis calego obiektu, albo pseudo-itemu.
 */
varargs public mixed
long(string str, object for_obj)
{
    int index;

    if (!str)
        return check_call(obj_long, for_obj) ?:
               "Jest to nieopisany obiekt.\n";

    if (!pointerp(obj_items))
	return 1;

    index = sizeof(obj_items);
    while(--index >= 0)
        if (member_array(str, obj_items[index][0]) >= 0)
            return check_call(obj_items[index][1]) ?:
                   "Nie zauwazasz niczego specjalnego.\n";

    /* If we end up here there were no such item. Why 1? /Mercade */
    return 1;
}

/*
 * Function name: query_long
 * Description  : Gives the set long description. This does not evaluate
 *                possible VBFC but returns exactly what was set as long
 *                description with set_long().
 * Returns      : mixed - exactly what was given to set_long(). This can
 *                        either be the long description of the VBFC in
 *                        string of functionpointer form.
 */
public mixed
query_long()
{
    return obj_long;
}

/*
 * Function name: check_seen
 * Description:   True if this object can be seen by a given object
 * Arguments:     for_obj: The object for which visibilty should be checked
 * Returns:       1 if this object can be seen.
 */
public int
check_seen(object for_obj)
{
    int aw, seed;
#if 0
    if (!objectp(for_obj) ||
	obj_no_show ||
	(!for_obj->query_wiz_level() &&
	 (for_obj->query_prop(LIVE_I_SEE_INVIS) <
	  this_object()->query_prop(OBJ_I_INVIS) ||
	  for_obj->query_skill(SS_AWARENESS) <
	  this_object()->query_prop(OBJ_I_HIDE))))
    {
	return 0;
    }
#endif
    if (!objectp(for_obj) ||
	obj_no_show ||
	(!for_obj->query_wiz_level() &&
	 (for_obj->query_prop(LIVE_I_SEE_INVIS) <
	  this_object()->query_prop(OBJ_I_INVIS))))
	return 0;
	
    aw = for_obj->query_skill(SS_AWARENESS) / 2;
    sscanf(OB_NUM(for_obj), "%d", seed);

    if (aw + random(aw, (seed + query_prop(OBJ_I_HIDE))) < 
		query_prop(OBJ_I_HIDE))
	return 0;


#if 0
    if (for_obj->query_skill(SS_AWARENESS) <
	  this_object()->query_prop(OBJ_I_HIDE))))
    {
	return 0;
    }
#endif

    return 1;
}

/*
 * Nazwa funkcji : odmien_short
 * Opis          : Wymusza odmiane shorta w liczbie pojedynczej przez
 *		   przypadki, korzystajac z przymiotnikow i glownej
 *		   nazwy obiektu.
 */
public void
odmien_short()
{
    int c;

    obj_shorts = allocate(6);
    
    for (c = 0; c < 6; c++)
        if (sizeof(obj_przym[0]))
            obj_shorts[c] = implode(query_przym(1, c), " ") + " " + 
              obj_names[c][0];
        else
            obj_shorts[c] = obj_names[c][0];

    obj_rodzaj_shorta = obj_rodzaje[0][0];
}

/*
 * Nazwa funkcji : short
 * Opis          : Zwraca krotki opis tego obiektu. Tablica odmiany shorta 
 *		   przez przypadki zostanie utworzona, jesli obiekt jeszcze
 *		   nie ma jej ustawionej (albo w kodzie, albo poprzez 
 *		   poprzednie wywolanie short() ). Tworzenie tablicy odmiany
 *		   shorta polega na laczeniu dostepnych przymiotnikow 
 *		   z nazwami. Shorty moga byc wartosciami typu VBFC.
 * Argumenty     : object for_obj - Obiekt ktory chce shorta.
 *                 int    przyp   - Przypadek w jakim short ma byc zwrocony.
 * Funkcja zwraca: string - krotki opis obiektu w liczbie pojedynczej.
 */
public varargs string 
short(mixed for_obj, mixed przyp)
{
    int c;
    
    if (!objectp(for_obj))
    {
        if (intp(for_obj))
            przyp = for_obj;
        else if (stringp(for_obj))
            przyp = atoi(for_obj);
        
        for_obj = previous_object();
    }
    else
        if (stringp(przyp))
            przyp = atoi(przyp);

    if (!pointerp(obj_shorts))
        odmien_short();
        
    return check_call(obj_shorts[przyp], for_obj);
}

/*
 * Function name:   vbfc_short
 * Description:     Gives short as seen by previous_object
 * Returns:         string holding short()
 * Arguments:       pobj: Object which to make the relation for
 *                  if not defined we assume that we are doing a vbfc
 *                  through the vbfc_object
 */
varargs public string
vbfc_short(mixed pobj, mixed przyp)
{
    object tp;
    
    if (!objectp(pobj))
    {
        if (intp(pobj))
            przyp = pobj;
        else if (stringp(pobj))
            przyp = atoi(pobj);
        
        pobj = previous_object(-1);
    }
    else
        if (stringp(przyp))
            przyp = atoi(przyp);

    if (!this_object()->check_seen(pobj) ||
	!CAN_SEE_IN_ROOM(pobj))
    {
	return ({ "cos", "czegos", "czemus", "cos", "czyms", "czyms" })[przyp];
    }

    return short(pobj, przyp);
}

/*
 * Nazwa funkcji : 
 * Opis          : 
 * Argumenty     : 
 * Funkcja zwraca: 
 */
varargs public string
vbfc_cshort(mixed pobj, mixed przyp)
{
    if (!objectp(pobj))
    {
        if (intp(pobj))
            przyp = pobj;
        else if (stringp(pobj))
            przyp = atoi(pobj);
        
        pobj = previous_object(-1);
    }
    else
        if (stringp(przyp))
            przyp = atoi(przyp);
    
    return capitalize(this_object()->vbfc_short(pobj, przyp));
}

/*
 * Nazwa funkcji : query_short
 * Opis          : Funkcja zwraca krotki opis w liczbie pojedycznej w podanym
 *		   przypadku, bez odwolywania sie do VBFC. Zwraca dokladnie 
 *		   to samo, co zostalo ustawione przez ustaw_shorty().
 *		   *UWAGA* Mozliwe, ze ta funkcja w przyszlosci zostanie 
 *		   usunieta!
 * Argumenty     : int przyp - short w ktorym przypadku ma zostac zwrocony.
 * Funkcja zwraca: string - krotki opis obiektu, w zadanym przypadku, 
 *			    w liczbie pojedynczej.
 */
public varargs string
query_short(int przyp)
{
    if (!pointerp(obj_shorts))
/*        odmien_short();*/
        return 0;
        
    return obj_shorts[przyp];
}

/*
 * Nazwa funkcji : odmien_plural_short
 * Opis          : Wymusza odmiane shorta w liczbie mnogiej przez
 *		   przypadki, korzystajac z przymiotnikow i glownej
 *		   nazwy obiektu.
 */
public void
odmien_plural_short()
{
    int c;

    if (!sizeof(obj_pnames[0]))
        return 0;
        
    obj_pshorts = allocate(6);

    for (c = 0; c < 6; c++)
        if (sizeof(obj_przym[0]))
            obj_pshorts[c] = implode(query_pprzym(1, c), " ") + " " +
              obj_pnames[c][0];
        else
            obj_pshorts[c] = obj_pnames[c][0];
}

/*
 * Nazwa funkcji : plural_short
 * Opis          : Zwraca krotki opis tego obiektu w liczbie mnogiej. 
 *		   Tablica odmiany mnogiego shorta przez przypadki zostanie 
 *		   utworzona, jesli obiekt jeszcze nie ma jej ustawionej 
 *		   (albo w kodzie poprzez ustaw_shorty(), albo poprzez 
 *		   poprzednie wywolanie short() ). Tworzenie tablicy odmiany
 *		   shorta polega na laczeniu dostepnych przymiotnikow 
 *		   z nazwami w liczbie mnogiej. Shorty moga byc wartosciami 
 *		   typu VBFC.
 * Argumenty     : object for_obj - Obiekt ktory chce shorta.
 *                 int    przyp   - Przypadek w jakim short ma byc zwrocony.
 * Funkcja zwraca: string - krotki opis obiektu w liczbie mnogiej.
 */
public varargs string
plural_short(mixed for_obj, int przyp)
{
    int c;

    if (intp(for_obj)) 
    {
	przyp = for_obj;
	for_obj = this_player();
    }
    /* Nie sprawdzamy czy przyp to int, bo nie ma makr do tej funkcji */

    if (!pointerp(obj_pshorts))
        odmien_plural_short();

    
    if (objectp(for_obj) && !check_seen(for_obj))
        return 0;

    return check_call(obj_pshorts[przyp], for_obj);
}

/*
 * Nazwa funkcji : query_plural_short
 * Opis          : Funkcja zwraca krotki opis w liczbie mnogiej w podanym
 *		   przypadku, bez odwolywania sie do VBFC. Zwraca dokladnie 
 *		   to samo, co zostalo ustawione przez ustaw_shorty().
 *		   *UWAGA* Mozliwe, ze ta funkcja w przyszlosci zostanie 
 *		   usunieta!
 * Argumenty     : int przyp - short w ktorym przypadku ma zostac zwrocony.
 * Funkcja zwraca: string - krotki opis obiektu, w zadanym przypadku, 
 *			    w liczbie mnogiej.
 */
public varargs string
query_plural_short(int przyp)
{
    if (!pointerp(obj_pshorts))
/*        odmien_plural_short();*/
          return 0;
        
    return obj_pshorts[przyp];
}

/*
 * Nazwa funkcji : query_tylko_mn
 * Opis          : Zwraca 1, gdy short (jesli zdefiniowany), lub nazwa 
 *		   glowna obiektu jest nazwa nie majaca liczby pojedynczej.
 *		   (np. slowo 'drzwi').
 * Funkcja zwraca: int - patrz wyzej.
 */
public int 
query_tylko_mn()
{
    if (obj_rodzaj_shorta != 0)
        return (obj_rodzaj_shorta < 0);

    return (obj_rodzaje[0][0] < 0);
}

/*
 * Nazwa funkcji : koncowka
 * Opis          : Zaleznie od rodzaju nazwy glownej obiektu, zwraca jedna z
 *		   koncowek. Oczywiscie moze miec szersze zastosowanie
 *		   do jakichkolwiek stringow, majacych byc uzaleznionych
 *		   od rodzaju. Ostatnie dwie koncowki beda uzyte, gdy
 *		   obiekt ma nazwy wylacznie w liczbie mnogiej (np. drzwi),
 *		   lub aktualnie wystepuje w mnoogiej formie (np. monety).
 * Argumenty     : Piec mozliwych koncowek:
 *		     o string meska, lp
 *		     o string zenska, lp
 *		     o string nijaka, lp
 *		     o string meskoosobowa, lmn
 *		     o string niemeskoosobowa, lmn
 *		   Tylko pierwsze dwa argumenty sa obowiazkowe. W przypadku, 
 *		   gdy nie poda sie koncowki dla rodzaju nijakiego, funkcja,
 *		   gdy zapytana, bedzie zwracac meska koncowke. Dwie ostatnie
 *		   koncowki sa dla liczby mnogiej - beda uzyte, gdy nazwa
 *		   glowna obiektu nie posiada swojej liczby pojedynczej.
 * Funkcja zwraca: Wybrana koncowke, w zaleznosci od aktualnego rodzaju
 *		   shorta lub nazwy obiektu.
 */
public string 
koncowka(string meski, string zenski, string nijaki = 0, 
    string mos = 0, string mnos = 0)
{
    int rodzaj = this_object()->query_rodzaj();
    
    if (query_tylko_mn())
    {
        if (rodzaj == -(PL_MESKI_OS+1) )
            return mos;
        else
            return mnos;
    }
    
    switch (rodzaj)
    {
        case PL_MESKI_OS:
        case PL_MESKI_NOS_ZYW:
        case PL_MESKI_NOS_NZYW:
            return meski;
        case PL_ZENSKI:
            return zenski;
        case PL_NIJAKI_OS:
        case PL_NIJAKI_NOS:
            if (!nijaki)
                return meski;
            else
                return nijaki;
    }
}

/*
 * Nazwa funkcji : query_zaimek
 * Opis          : Funkcja zwraca zaimek przedmiotu, na ktorej wywolujemy
 *		   te funkcje, w podanym przypadku i liczbie.
 * Argumenty     : int przypadek - przypadek szukanego zaimka.
 *		   int dluga	 - czy forma zaimka ma byc w dlugiej formie.
 *				   (np. zaimek mezczyzny, w celowniku ma 
 *				   dluga forme 'jemu', zas krotka 'mu').
 *				   (opcjonalny)
 *		   int liczba	 - 0 pojedyczna, 1 mnoga (opcjonalny).
 * Funkcja zwraca: string - zaimek - patrz wyzej.
 */
public string
query_zaimek(int przypadek, int dluga = 0, int liczba = 0)
{
    int rodzaj;

    rodzaj = this_object()->query_rodzaj();

    if (!liczba)
    {
	switch (przypadek)
	{
        case PL_MIA: 
            switch (rodzaj)
            {
		case PL_MESKI_OS:
		case PL_MESKI_NOS_ZYW:
		case PL_MESKI_NOS_NZYW: return "on";
		case PL_ZENSKI: return "ona";
		case PL_NIJAKI_OS:
		case PL_NIJAKI_NOS: return "ono";
            }
        case PL_DOP:
            switch (rodzaj)
            {
		case PL_ZENSKI: return "jej";
		default: return (dluga ? "jego" : "go");
            }
        case PL_CEL:
            switch(rodzaj)
            {
		case PL_ZENSKI: return "jej";
		default: return (dluga ? "jemu" : "mu");
            }
        case PL_BIE:
            switch(rodzaj)
            {
		case PL_MESKI_OS:
		case PL_MESKI_NOS_ZYW:
		case PL_MESKI_NOS_NZYW: return (dluga ? "jego" : "go");
		case PL_ZENSKI: return "ja";
		case PL_NIJAKI_OS:
		case PL_NIJAKI_NOS: return (dluga ? "jego" : "je");
            }
        case PL_NAR:
            switch(rodzaj)
            {
		case PL_ZENSKI: return "nia";
		default: return "nim";
            }
        case PL_MIE:
            switch(rodzaj)
            {
		case PL_ZENSKI: return "niej";
		default: return "nim";
            }
	}
    }
    else
    {
	switch (przypadek)
	{
	    case PL_MIA:
		switch(rodzaj)
		{
		    case PL_MESKI_OS: return "oni";
		    default: return "one";
		}
	    case PL_DOP:
		return "ich";
	    case PL_CEL:
		return "im";
	    case PL_BIE:
		switch(rodzaj)
		{
		    case PL_MESKI_OS: return "ich";
		    default: return "je";
		}
	    case PL_NAR:
		return "nimi";
	    case PL_MIE:
		return "nich";
	}
    }
    return 0;
}

/*
 * Function name: add_prop
 * Description:   Add a property to the property list
 *                If the property already exist, the value is replaced
 *                If a function "add_prop" + propname is declared or
 *                is shadowing this_object then that function is called
 *                prior to the setting of the property. 
 *                NOTE
 *                  If the optional function above returns something other
 *                  than 0. The property will NOT be set.
 *
 * Arguments:     prop - The property string to be added.
 *                val: The value of the property
 * Returns:       None.
 */
public void
add_prop(string prop, mixed val)
{
    mixed oval;

    /* If there isn't a value, remove the current value. */
    if (!val)
    {
	remove_prop(prop);
	return;
    }

    /* All changes might have been locked out. */
    if (obj_no_change)
    {
	return;
    }

    if (call_other(this_object(), "add_prop" + prop, val))
    {
	return;
    }

    if (!mappingp(obj_props))
    {
	obj_props = ([ ]);
    }

    oval = query_prop(prop);
    obj_props[prop] = val;

    if (environment())
    {
	environment()->notify_change_prop(prop, query_prop(prop), oval);
    }
}

/*
 * Function name: change_prop
 * Description  : This function is a mask of add_prop. For details, see
 *                the header of that function.
 */
public void
change_prop(string prop, mixed val)
{
    add_prop(prop, val);
}

/*
 * Function name: remove_prop
 * Description:   Removes a property string from the property list.
 * Arguments:     prop - The property string to be removed.
 */
public void
remove_prop(string prop)
{
    /* All changes may have been locked out. */
    if (obj_no_change ||
	!mappingp(obj_props) ||
	!prop)
    {
	return;
    }

    if (call_other(this_object(), "remove_prop" + prop))
    {
	return;
    }

    if (environment())
    {
	environment()->notify_change_prop(prop, 0, query_prop(prop));
    }

    obj_props = m_delete(obj_props, prop);
}

#define CFUN
#ifndef CFUN
/*
 * Function name: query_prop
 * Description:   Find the value of a property.
 * Arguments:     prop - The property searched for.
 * Returns:       The value or 0.
 */
public mixed
query_prop(string prop)
{
    if (!mappingp(obj_props))
	return 0;
    return check_call(obj_props[prop]);
}
#else
public mixed
query_prop(string prop) = "query_prop";
#endif

/*
 * Function name: query_props
 * Description:   Give all the existing properties
 * Returns:       An array of property names or 0.
 */
public nomask mixed
query_props() 
{
    if (mappingp(obj_props))
	return m_indexes(obj_props);
    else
	return 0;
}

/*
 * Function name: query_prop_setting
 * Description:   Returns the true setting of the prop
 * Arguments:     prop - The property searched for
 * Returns:       The true setting (mixed)
 */
public nomask mixed
query_prop_setting(string prop)
{
    if (!mappingp(obj_props))
	return 0;
    return obj_props[prop];
}

/*
 * Function name: notify_change_prop
 * Description:   This function is called when a property in an object
 *                in the inventory has been changed.
 * Arguments:     prop - The property that has been changed.
 *                val  - The new value.
 *                oval - The old value
 */
public void
notify_change_prop(string prop, mixed val, mixed oval)
{
}

/*
 * Function name: mark_state
 * Description:   Mark the internal state so that update is later possible
 */
public void
mark_state()
{
    /* More properties can be added here if need be
     */
    obj_state = ({ query_prop(OBJ_I_LIGHT), query_prop(OBJ_I_WEIGHT),
		   query_prop(OBJ_I_VOLUME) });
}

/*
 * Function name: update_state
 * Description:   Update the environment according to the changes in our
 *                state;
 */
public void
update_state()
{
    int l, w, v;

    l = query_prop(OBJ_I_LIGHT); 
    w = query_prop(OBJ_I_WEIGHT);
    v = query_prop(OBJ_I_VOLUME);

    /* More properties can be added here if need be
     */
    if (environment(this_object()))
	environment(this_object())->update_internal(l - obj_state[0],
						    w - obj_state[1],
						    v - obj_state[2]);
}
    
/*
 * Function name: move
 * Description:   Move this object to the destination given by string /
 *                obj. If the second parameter exists then weight
 *                accounting and tests on destination is not done.
 * Arguments:     dest: Object or filename to move to,
 *                subloc: 1 == Always move, otherwise name of sublocation
 * Returns:       Result code of move:
		  0: Success.
		  1: To heavy for destination.
		  2: Can't be dropped.
		  3: Can't take it out of it's container.
		  4: The object can't be inserted into bags etc.
		  5: The destination doesn't allow insertions of objects.
		  6: The object can't be picked up.
		  7: Other (Error message printed inside move() func)
		  8: Too big volume for destination
		  9: The container is closed, can't remove
		 10: The container is closed, can't put object there
 */
varargs public int
move(mixed dest, mixed subloc)
{
    object          old;
    int             is_room, rw, rv, is_live_dest, is_live_old,
		    uw,uv,
		    sw,sv;
    mixed           tmp;

    if (!dest)
	return 5;
    old = environment(this_object());
    if (stringp(dest))
    {
	call_other(dest, "??");
	dest = find_object(dest);
    }
    if (!objectp(dest))
	dest = old;

    if (subloc == 1)
    {
	move_object(dest);
    }
    else if (old != dest)
    {
	if (!dest || !dest->query_prop(CONT_I_IN) || dest->query_prop(CONT_M_NO_INS))
	    return 5;
	if ((old) && (old->query_prop(CONT_M_NO_REM)))
	    return 3;
	if (old && old->query_prop(CONT_I_CLOSED))
	    return 9;

	if (old)
	    is_live_old = (function_exists("create_container",
					   old) == "/std/living");
	is_live_dest = (function_exists("create_container",
					dest) == "/std/living");

	if (old && is_live_old && this_object()->query_prop(OBJ_M_NO_DROP))
	    return 2;

	is_room = (int) dest->query_prop(ROOM_I_IS);

	if (!is_live_dest)
	{
	    if ((!is_room) && (this_object()->query_prop(OBJ_M_NO_INS)))
		return 4;
	    if (dest && dest->query_prop(CONT_I_CLOSED))
		return 10;
	}
	else
	{
	    if ((!is_live_old) && (this_object()->query_prop(OBJ_M_NO_GET)))
		return 6;
	    else if (is_live_old && this_object()->query_prop(OBJ_M_NO_GIVE))
		return 3;
	}
	
	if (!is_room)
	{
	    rw = dest->query_prop(CONT_I_MAX_WEIGHT) - 
		dest->query_prop(OBJ_I_WEIGHT);
	    rv = dest->volume_left();
	    
	    if (rw < 0) rw = 0; // na wypadki, kiedy rw < 0, np po uzyciu
	    if (rv < 0) rv = 0; // komendy money, ktora mozna sie PRZEladowac
	        
	    if (!query_prop(HEAP_I_IS))
	    {
		if (rw < query_prop(OBJ_I_WEIGHT))
		    return 1;
		if (rv < query_prop(OBJ_I_VOLUME))
		    return 8;
	    }
	    else
	    {
		sw = 0;
		sv = 0;
		if (rw < query_prop(OBJ_I_WEIGHT))
		{
		    uw = query_prop(HEAP_I_UNIT_WEIGHT);
		    if (uw > rw)
		    {
			return 1;
		    }
		
		    sw = rw / uw; /* This amount of the heap can be carried */
		    sv = sw;
		}

		if (rv < query_prop(OBJ_I_VOLUME))
		{
		    uv = query_prop(HEAP_I_UNIT_VOLUME);
		    if (uv > rv)
			return 8;

		    sv = rv / uv; /* This amount of the heap can be carried */
		    if (!sw)
			sw = sv;
		}
		if (sw || sv)
		    this_object()->split_heap((sw < sv) ? sw : sv);
	    }
	}

        if (old && old->prevent_leave(this_object()))
            return 7;
       
        if (dest && dest->prevent_enter(this_object()))
            return 7;
            
	move_object(dest);
    }

    if (subloc != 1)
	obj_subloc = subloc;

    if (old != dest)
    {
	if (old) 
	{
	    this_object()->leave_env(old, dest);
	    old->leave_inv(this_object(),dest);
	}

	if (dest)
	{
	    this_object()->enter_env(dest, old);
	    dest->enter_inv(this_object(),old);
	}
    }
    mark_state();
    return 0;
}

/*
 * Function name: query_subloc
 * Description:   Get the current sub location's name
 */
mixed
query_subloc() 
{ 
    return obj_subloc; 
}

/*
 * Function name: enter_inv
 * Description  : This function is called each time an object enters the
 *                inventory of this object. If you mask it, be sure that
 *                you _always_ call the ::enter_inv(ob, old) function.
 * Arguments    : object ob  - the object entering our inventory.
 *                object old - wherever 'ob' came from. This can be 0.
 */
void
enter_inv(object ob, object old) 
{
}

/*
 * Function name: enter_env
 * Description  : This function is called each time this object enters a
 *                new environment. If you mask it, be sure that you
 *                _always_ call the ::enter_env(dest, old) function.
 * Arguments    : object dest - the destination we are entering.
 *                object old  - the location we came from. This can be 0.
 */
void
enter_env(object dest, object old) 
{
}

/*
 * Function name: leave_inv
 * Description  : This function is called each time an object leaves the
 *                inventory of this object. If you mask it, be sure that
 *                you _always_ call the ::leave_inv(ob, old) function.
 * Arguments    : object ob   - the object leaving our inventory.
 *                object dest - wherever 'ob' goes to. This can be 0.
 */
void
leave_inv(object ob, object dest) 
{
}

/*
 * Function name: leave_env
 * Description  : This function is called each time this object leaves an
 *                old environment. If you mask it, be sure that you
 *                _always_ call the ::leave_env(dest, old) function.
 * Arguments    : object old  - the location we are leaving.
 *                object dest - the destination we are going to. Can be 0.
 */
void
leave_env(object old, object dest) 
{
}

/*
 * Function name: recursive_rm
 * Description  : When an object is removed, its complete inventory is
 *                mapped through this function. If it is an interactive
 *                object, it will be moved to its default starting
 *                location else it will be descructed.
 * Arguments    : object ob - the object to remove.
 */
void
recursive_rm(object ob)
{
    if (query_ip_number(ob))
	ob->move(ob->query_default_start_location());
    else
	ob->remove_object();
}

/*
 * Function name: remove_object
 * Description:   Removes this object from the game.
 * Returns:       True if the object was removed.
 */
public int
remove_object()
{
    map(all_inventory(), recursive_rm);
    if (environment(this_object()))
	environment(this_object())->leave_inv(this_object(),0);
    this_object()->leave_env(environment(this_object()),0);
    destruct();
    return 1;
}

/*
 * Function name: vbfc_caller
 * Description:   This function will hopfully return correct object which this
 *                vbfc is for.
 * Returns:       The object who wants a vbfc
 */
public object
vbfc_caller() 
{ 
    return obj_previous; 
}

#ifndef CFUN
/*
 * Function name: check_call
 * Description  : This is the function that resolves VBFC. Call it with a
 *                an argument in either function-VBFC or string-VBFC and this
 *                function will parse the argument, make the necessary call
 *                and return the queried value. If a non-VBFC argument is
 *                passed along, it will be returned unchanged. For more
 *                information on VBFC, see 'man VBFC'.
 * Arguments    : mixed retval   - an argument containing VBFC.
 *                object for_obj - the object that wants to know, if omitted,
 *                                 use previous_object().
 * Returns      : mixed - the resolved VBFC.
 */
public nomask varargs mixed
check_call(mixed retval, object for_obj = previous_object())
{
    int             more;
    string          a, b;
    mixed           proc_ret;

    if (functionp(retval))
    {
        obj_previous = for_obj;

        proc_ret = retval();

        obj_previous = 0;
        return proc_ret;
    }

    if (!stringp(retval))
    {
        return retval;
    }

    obj_previous = for_obj;

    more = sscanf(retval, "@@%s@@%s", a, b);

    if (more == 0 && wildmatch("@@*", retval))
        proc_ret = process_value(extract(retval, 2), 1);
    else if (more == 1 || (more == 2 && !strlen(b)))
        proc_ret = process_value(a, 1);
    else
        proc_ret = process_string(retval, 1);

    obj_previous = 0;
    return proc_ret;
}
#else
public nomask varargs mixed
check_call(mixed retval, object for_obj = previous_object()) = "check_call";
#endif

/*
 * Function name: reset_euid
 * Description  : This function can be called externally to make sure that
 *                the euid of this object is set exactly to the uid of the
 *                object. All it does is: seteuid(getuid(this_object()));
 */
public void
reset_euid() 
{ 
    seteuid(getuid()); 
}

/*
 * Function name: add_list
 * Description:   Common routine for set_name, set_pname, set_adj etc
 * Arguments:     list: The list of elements
 *                elem: string holding one new element.
 *                first: True if it is the main name, pname adj
 * Returns:       The new list.
 */
private string *
add_list(string *list, mixed elem, int first)
{
    string *e;

    if (obj_no_change)
        return list;

    if (pointerp(elem))
        e = elem;
    else
        e = ({ elem });
    
    if (!pointerp(list))
        list = ({ }) + e;
    else if (first)
        list = e + (list - e);
    else
        list = list + (e - list);

    return list;
}

/*
 * Function name: del_list
 * Description:   Removes one or many elements from a list.
 * Arguments:     list_old: The list as it looks.
 *                list_del: What should be deleted
 * Returns:       The new list.
 */
private string *
del_list(string *list_old, mixed list_del)
{
#if 0
    int il, pos;

    if (obj_no_change)
	return list_old;                /* All changes has been locked out */

    if (!pointerp(list_del))
	list_del = ({ list_del });

    for (il = 0; il < sizeof(list_del); il++)
    {
	pos = member_array(list_del[il], list_old);
	if (pos >= 0)
	    list_old = exclude_array(list_old, pos, pos);
    }
    return list_old;
#endif
    if (obj_no_change)
	return list_old;       		/* All changes has been locked out */

    if (!list_old)
        return list_old;

    if (!pointerp(list_del))
	list_del = ({ list_del });

    return (list_old - (string *)list_del);

}

/*
 * Function name: query_list
 * Description:   Gives the return of a query on a list.
 * Arguments:     list: The list in question
 *                arg: If true then the entire list is returned.
 * Returns:       A string or an array as described above.
 */
private mixed
query_list(mixed list, int arg)
{
    if (!pointerp(list))
	return 0;

    if (!arg && sizeof(list))
	return list[0];
    else
	return list + ({});
}

/*
 * Nazwa funkcji  : query_rodzaj
 * Opis           : Zwraca rodzaj gramatyczny shorta (jesli jest 
 *		    zdefiniowany) lub glownej nazwy obiektu.
 * Funkcja zwraca : int - patrz opis
 */
public int
query_rodzaj()
{
    int rodzaj;

    if (obj_rodzaj_shorta)
        rodzaj = obj_rodzaj_shorta;
    else
        rodzaj = obj_rodzaje[0][0];
        
    if (rodzaj < 0)
        return (-rodzaj - 1);
    else
        return (rodzaj - 1);

}

public int
query_rodzaj_shorta()
{
    return obj_rodzaj_shorta;
}

/*
 * Nazwa funkcji : query_real_rodzaj
 * Opis          : Zwraca prawdziwy rodzaj pierwszej nazwy, lub wszystkich
 *		   nazw w podanym przypadku, w liczbie pojedynczej. 
 *		   Prawdziwy, gdyz rodzaje sa zapamietywane w obiekcie w 
 *		   specjalny sposob, rozniacy sie nieco od ogolnie znanego. 
 *		   (wartosci PL_ZENSKI, itd). Przy zapamietywaniu rodzaju, 
 *		   numer mu przyporzadkowany jest zwiekszany o 1, a jesli 
 *		   dana nazwa nie posiada liczby pojedynczej, zmienia sie 
 *		   mu jeszcze znak na ujemny.
 * Argumenty     : int wszystkie - czy zwrocic rodzaj wszystkich nazw w danym
 *				   przypadku, czy tylko pierwszej.
 *		   int przyp - przypadek, w ktorym rodzaje maja byc zwracane.
 * Funkcja zwraca: int - prawdziwy rodzaj lub rodzaje obiektu, liczbie 
 *			 pojedynczej, w podanym przypadku.
 */
public mixed
query_real_rodzaj(int wszystkie, int przyp)
{
    return query_list(obj_rodzaje[przyp], wszystkie);
}

/*
 * Nazwa funkcji : query_real_prodzaj
 * Opis          : Zwraca prawdziwy rodzaj pierwszej nazwy, lub wszystkich
 *		   nazw w podanym przypadku, w liczbie mnogiej. 
 *		   Prawdziwy, gdyz rodzaje sa zapamietywane w obiekcie w 
 *		   specjalny sposob, rozniacy sie nieco od ogolnie znanego. 
 *		   (wartosci PL_ZENSKI, itd). Przy zapamietywaniu rodzaju, 
 *		   numer mu przyporzadkowany jest zwiekszany o 1, a jesli 
 *		   dana nazwa nie posiada liczby pojedynczej, zmienia sie 
 *		   mu jeszcze znak.
 * Argumenty     : int wszystkie - czy zwrocic rodzaj wszystkich nazw w danym
 *				   przypadku, czy tylko pierwszej.
 *		   int przyp - przypadek, w ktorym rodzaje maja byc zwracane.
 * Funkcja zwraca: int - prawdziwy rodzaj lub rodzaje obiektu, liczbie 
 *			 mnogiej, w podanym przypadku.
 */
public varargs mixed
query_real_prodzaj(int arg, int przyp)
{
    return query_list(obj_prodzaje[przyp], arg);
}

/*
 * Nazwa funkcji : query_rodzaj_nazwy
 * Opis          : Funkcja zwraca jaki rodzaj ma podana nazwa, o ile
 *		   wogole zostala ona dodana w obiekcie. Mozna rowniez
 *		   wyszczegolnic, w jakiej liczbe szukac nazwy
 *		   (pojedynczej czy mnogiej), oraz w jakim przypadku.
 *		   Wartosc -1 podana w argumencie liczba albo przypadek,
 *		   spowoduje, ze przeszukiwane beda odpowiednio obie
 *		   liczby, albo wszystkie rodzaje.
 * Argumenty     : string nazwa  - nazwa, ktorej rodzaju szukamy.
 *		   int	  liczba - wyszczegolnienie, w jakiej liczbie
 *				   szukac danej nazwy.
 *					-1 - w obu (domyslnie)
 *					 0 - w pojedynczej
 *					 1 - w mnogiej.
 *		   int 	  przyp  - wyszczegolnienie, w jakim przypadku
 *				   szukac danej nazwy. -1 oznacza, ze we
 *				   wszystkich.
 * Funkcja zwraca: Rodzaj, albo -1, jesli obiekt nie posiadal podanej nazwy.
 */
public int
query_rodzaj_nazwy(string nazwa, int liczba = -1, int przyp = -1)
{
    int x, y;
    int rodzaj = 13; // Nierealna wartosc rodzaju.

    if (przyp == -1)
    {
        if (liczba != 1) // 0 lub -1, czyli pojedyncza
        {
            x = -1;
            while (++x < 6)
                if ((y = member_array(nazwa, obj_names[x])) != -1)
                {
                    rodzaj = obj_rodzaje[x][y];
                    break;
                }
        }

        if (liczba != 0) // 1 lub -1, czyli mnoga
        {
            x = -1;
            while (++x < 6)
                if ((y = member_array(nazwa, obj_pnames[x])) != -1)
                {
                    rodzaj = obj_prodzaje[x][y];
                    break;
                }
        }
    }
    else
    {
        if (liczba != 1) // 0 lub -1, czyli pojedyncza
            if ((y = member_array(nazwa, obj_names[przyp])) != -1)
               rodzaj = obj_rodzaje[przyp][y];
               
        if ((rodzaj == 13) && (liczba != 0)) // 1 lub -1, czyli mnoga
            if ((y = member_array(nazwa, obj_pnames[przyp])) != -1)
		rodzaj = obj_prodzaje[przyp][y];
    }
    
    if (rodzaj == 13)
        return -1;
    
    if (rodzaj < 0)
        return (-rodzaj - 1);
    else
        return (rodzaj - 1);
}


#ifndef CFUN
static int
usun_stare_nazwy(mixed nazwy, int przyp, int lmn, int czy_dodaj = 0)
{
    int c, d, *tmp2, size;
    string *tmp;

    if (!pointerp(nazwy))
        nazwy = ({ nazwy });
        
    if (!(c = sizeof(nazwy)))
        return 1;
        
    if (lmn)
        size = sizeof(obj_pnames[przyp]);
    else
        size = sizeof(obj_names[przyp]);
        
    if (czy_dodaj && (c == 1) && size)
    {
        if (lmn)
        {
            if (obj_pnames[przyp][0] == nazwy[0])
                return 0;
        }
        else
            if (obj_names[przyp][0] == nazwy[0])
                return 0;
    }
    
    size--;
    
    if (!lmn)
    {
        while (--c >= 0)
        {
            tmp = ({}); tmp2 = ({});
            d = size;
            while (d >= 0)
            {
                if (nazwy[c] == obj_names[przyp][d])
                {
                    // Usuwamy element d.
                    if (d) // nie jest na poczatku
                    {
                         tmp = obj_names[przyp][0..d-1];
                         tmp2 = obj_rodzaje[przyp][0..d-1];
                    }
                    if (d != size) // ani na koncu
                    {
                         tmp += obj_names[przyp][d+1..size];
                         tmp2 += obj_rodzaje[przyp][d+1..size];
                    }
                    size--;
                    obj_names[przyp] = tmp; obj_rodzaje[przyp] = tmp2;
                }
                d--;
            }
        }
    }
    else //lmn
    {
        while (--c >= 0)
        {
            tmp = ({}); tmp2 = ({});
            d = size;
            while (d >= 0)
            {
                if (nazwy[c] == obj_pnames[przyp][d])
                {
                    // usuwamy element d
                    if (d) // nie jest na poczatku
                    {
                         tmp = obj_pnames[przyp][0..d-1];
                         tmp2 = obj_prodzaje[przyp][0..d-1];
                    }
                    if (d != size) // ani na koncu
                    {
                         tmp += obj_pnames[przyp][d+1..size];
                         tmp2 += obj_prodzaje[przyp][d+1..size];
                    }
                    size--;
                    obj_pnames[przyp] = tmp; obj_prodzaje[przyp] = tmp2;
                }
                d--;
            }
        }
    }
    return 1;
}
#else
public int
usun_stare_nazwy(mixed nazwy, int przyp, int lmn, int czy_dodaj) = "usun_stare_nazwy";
#endif


/*
 * Nazwa funkcji  : ustaw_nazwe
 * Opis           : Jest to pomocnicza funkcja, majaca zapobiec ewentualnym
 *                  bledom w czasie wpisywania odmiany nazwy przez przypadki.
 *                  Sluzy do definiowania odmiany glownej nazwy obiektu.
 *                  Podaje sie dwie tablice i warunkiem ich wpisania do nazw
 *                  jest prawidlowa ilosc elementow(6, bo jest 6 przypadkow). 
 *		    Mozna tez, dla nazw nie posiadajacych lp, podac tylko 
 *		    jedna tablice, z odmiana w lmn. Mudlib uwzgledni to we 
 *		    wszystkich odmianach.
 * Argumenty      : string *nazwy - tablica zawierajaca odmiane nazwy obiektu
 *                                  przez przypadki w liczbie pojedynczej.
 *                  string *pnazwy - tablica zawierajaca odmiane nazwy
 *                                   obiektu przez przypadki w liczbie
 *                                   mnogiej.
 *		    int rodzaj - rodzaj gramatyczny, w jakim ta nazwa
 *				 jest.
 * Funkcja zwraca : 1, gdy nazwy zostaly wprowadzone do zmiennych, 0
 *                  w przeciwnym wypadku.
 */
public varargs int
ustaw_nazwe(string *nazwy, mixed pnazwy, 
            int rodzaj = this_object()->query_rodzaj())
{
    int c, d, size;
    string *tmp;
    int *tmp2;
 
    if (sizeof(nazwy) != 6)
    {
        throw("Zla ilosc przypadkow w odmianie liczby pojedynczej.\n");
        return 0;
    }

    if (pointerp(pnazwy))
    {
        if (sizeof(pnazwy) != 6)
        {
            throw("Zla ilosc przypadkow w odmianie liczby mnogiej.\n");
            return 0;
        }
            
        rodzaj += 1;
    }
    else
    {
        if (!intp(pnazwy))
            return 0;
        
        rodzaj = -(pnazwy + 1);
        pnazwy = nazwy;
    }
        
    for (c = 0; c < 6; c++)
    {
        usun_stare_nazwy(nazwy[c], c, 0, 0);
        obj_names[c] = ({ nazwy[c] }) + obj_names[c];
        obj_rodzaje[c] = ({ rodzaj }) + obj_rodzaje[c];
        
        usun_stare_nazwy(pnazwy[c], c, 1, 0);
        obj_pnames[c] = ({ pnazwy[c] }) + obj_pnames[c];
        obj_prodzaje[c] = ({ rodzaj }) + obj_prodzaje[c];
    }
    
//    catch(SLOWNIK->sprawdz_odmiane(nazwy, pnazwy, rodzaj));

    return 1;
}

/*
 * Nazwa funkcji  : dodaj_nazwy
 * Opis           : Jest to pomocnicza funkcja, majaca zapobiec ewentualnym
 *                  bledom w czasie wpisywania odmiany nazwy przez przypadki.
 *                  Sluzy do dodawania nazw, ktorymi bedziemy mogli
 *		    'zahaczyc' nasz obiekt (czyli np. wykonac na nim jakies
 *		    komendy). Podaje sie dwie tablice i warunkiem ich wpisania
 *		    do nazw jest prawidlowa ilosc elementow (6, bo jest 
 *		    6 przypadkow). Mozna tez, dla nazw nie posiadajacych, 
 *		    liczby pojedynczej, podac tylko jedna tablice, z odmiana
 *		    w liczbie mnogiej. Mudlib uwzgledni to we wszystkich 
 *		    odmianach.
 * Argumenty      : string *nazwy - tablica zawierajaca odmiane nazwy obiektu
 *                                  przez przypadki w liczbie pojedynczej.
 *                  string *pnazwy - tablica zawierajaca odmiane nazwy
 *                                   obiektu przez przypadki w liczbie
 *                                   mnogiej.
 *		    int rodzaj - rodzaj gramatyczny, w jakim ta nazwa
 *				 jest.
 * Funkcja zwraca : 1, gdy nazwy zostaly wprowadzone do zmiennych, 0
 *                  w przeciwnym wypadku.
 */
public varargs int
dodaj_nazwy(string *nazwy, mixed pnazwy,
	    int rodzaj = this_object()->query_rodzaj())
{
    int c, d, size;
    string *tmp;
    int *tmp2;
    
    if (sizeof(nazwy) != 6)
    {
        throw("Zla ilosc przypadkow w odmianie liczby pojedynczej.\n");
        return 0;
    }

    if (pointerp(pnazwy))
    {
        if (sizeof(pnazwy) != 6)
        {
            throw("Zla ilosc przypadkow w odmianie liczby mnogiej.\n");
            return 0;
        }
            
        rodzaj += 1;
    }
    else
    {
        if (!intp(pnazwy))
            return 0;
        
        rodzaj = -(pnazwy + 1);
        pnazwy = nazwy;
    }

    for (c = 0; c < 6; c++)
    {
        if (usun_stare_nazwy(nazwy[c], c, 0, 1))
        {
	    obj_names[c] =  obj_names[c] + ({ nazwy[c] });
	    obj_rodzaje[c] = obj_rodzaje[c] + ({ rodzaj });
	}
	
	if (usun_stare_nazwy(pnazwy[c], c, 1, 1))
	{
	    obj_pnames[c] += ({ pnazwy[c] });
	    obj_prodzaje[c] += ({ rodzaj });
	}
    }

//    catch(SLOWNIK->sprawdz_odmiane(nazwy, pnazwy, rodzaj));

    return 1;
}

public int
dodaj_przym(string lp_mian, string lmn_mian)
{
    if (!lp_mian || !lmn_mian)
        return 0;
        
    obj_przym[0] = add_list(obj_przym[0], lp_mian, 0);
    obj_przym[1] = add_list(obj_przym[1], lmn_mian, 0);
    
    return 1;
}

/*
 * Nazwa funkcji : query_przym
 * Opis          : Zwraca przymiotnik(i) obiektu w liczbie pojedynczej.
 * Argumenty     : int arg   - jesli prawdziwy, zostana zwrocone
 *			       wszystkie przymiotniki danego przypadku.
 *		   int przyp - numer przypadku.
 *		   int rodzaj- opcjonalnie, wymuszenie zmiany rodzaju.
 *			       nowy rodzaj powinien byc w formie, w jakiej
 *			       rodzaje sa zapamietywane w mudlibie.
 *			       (patrz: naglowek funkcji query_rodzaj)
 * Funkcja zwraca: mixed     - int 0    - nie ma zadnych przymiotnikow.
 *			       string   - jeden przymiotnik, jesli arg
 *					  jest falszywy (rowny 0).
 *			       string * - tablica z wszystkimi
 *					  przymiotnikami w danym przypadku.
 */
public varargs mixed
query_przym(int arg, int przyp, int rodzaj = this_object()->query_real_rodzaj())
{
    int ix, size, liczba;
    string *wynik = ({});
    
    if (!sizeof(obj_przym[0]))
        return ({ });
    
    if (rodzaj < 0)
    {
        liczba = 1;
        rodzaj = -rodzaj;
    }
    else liczba = 0;

    rodzaj -= 1;
    				  
    if (arg)
    {
        size = sizeof(obj_przym[0]);
        ix = -1;
        while(++ix < size)
        {
            wynik = wynik + ({ oblicz_przym(obj_przym[0][ix],
                obj_przym[1][ix], przyp, rodzaj, liczba) });
        }
        
        return wynik;
    }
    
    return oblicz_przym(obj_przym[0][0], obj_przym[1][0], przyp, 
        rodzaj, liczba);
}

/*
 * Nazwa funkcji : query_pprzym
 * Opis          : Zwraca przymiotnik(i) obiektu w liczbie mnogiej.
 * Argumenty     : int arg   - jesli prawdziwy, zostana zwrocone
 *			       wszystkie przymiotniki danego przypadku.
 *		   int przyp - numer przypadku.
 *		   int rodzaj- opcjonalnie, wymuszenie zmiany rodzaju.
 *			       nowy rodzaj powinien byc w formie, w jakiej
 *			       rodzaje sa zapamietywane w mudlibie.
 *			       (patrz: naglowek funkcji query_rodzaj)
 * Funkcja zwraca: mixed     - int 0    - nie ma zadnych przymiotnikow.
 *			       string   - jeden przymiotnik, jesli arg
 *					  jest falszywy (rowny 0).
 *			       string * - tablica z wszystkimi
 *					  przymiotnikami w danym przypadku.
 */
mixed
query_pprzym(int arg, int przyp, int rodzaj = this_object()->query_real_rodzaj())
{
    int x;
    string *wynik = ({});
    
    if (!sizeof(obj_przym[0]))
        return ({});
        
    if (rodzaj < 0)
        rodzaj = -rodzaj;

    rodzaj -= 1;

    if (arg)
    {
        x = sizeof(obj_przym[0]);
        while(--x >= 0)
        {
            wynik = wynik + ({ oblicz_przym(obj_przym[0][x], obj_przym[1][x],
                przyp, rodzaj, 1) });
        }
        return wynik;
    }
    
    return oblicz_przym(obj_przym[0][0], obj_przym[1][0], przyp, rodzaj, 1);
}

/*
 * Nazwa funkcji  : ustaw_shorty
 * Opis           : Ustawia pelna odmiane krotkich opisow obiektu przez
 *                  przypadki, zarowno w liczbie pojedynczej jak i mnogiej.
 *                  Obie tablice z shortami musza zawierac po 6 elementow.
 * Argumenty      : string  *shorty - tablica z odmiana krotkich opisow
 *                                    obiektu przez przypadki w liczbie p.
 *                  string *pshorty - tablica z odmiana shortow obiektu
 *                                    przez przypadki w liczbie mnogiej.
 *		    int rodzaj - rodzaj gramatyczny, w jakim ta nazwa
 *				 jest.
 * Funkcja zwraca : 1, gdy podana zostala wlasciwa ilosc elementow i
 *                  shorty zostaly ustawione. 0 w przeciwnym razie.
 */
public varargs int
ustaw_shorty(string *shorty, mixed pshorty, 
    int rodzaj = this_object()->query_rodzaj())
{

    if (sizeof(shorty) != 6)
    {
        throw("Zla ilosc przypadkow w odmianie shortow liczby pojedynczej.\n");
        return 0;
    }

    rodzaj += 1;

    if (pointerp(pshorty))
    {
        if (sizeof(pshorty) != 6)
        {
            throw("Zla ilosc przypadkow w odmianie liczby mnogiej.\n");
            return 0;
        }
            
    }
    else
    {
        if (!intp(pshorty))
            return 0;
        
        rodzaj = -pshorty;
        pshorty = shorty;
    }

    obj_shorts = shorty + ({});
    
    // mozna nie dodawac, gdy jest rodzaj tylko_mnoga. 
    // nie wiem po co dodaje. plural_short() moze sie zwracac do short(),
    // gdy napotka na ujemny rodzaj.
    obj_pshorts = pshorty + ({});
    
    obj_rodzaj_shorta = rodzaj;
    
    return 1;
}

/*
 * Nazwa funkcji : usun_shorty
 * Opis          : Usuwa cala obecna odmiane shortow obiektu.
 */
public void
usun_shorty()
{
    if (obj_no_change)
	return 0;
	
    obj_shorts = 0;
    obj_pshorts = 0;
    
    obj_rodzaj_shorta = 0;
}

/*
 * Nazwa funkcji : set_name
 * Opis          : Ustawia na poczatku tablicy nazw jedna lub wiecej nazwe w 
 *		   podanym przypadku, w liczbie pojedynczej. (nazwa glowna). 
 *		   Mozna rowniez podac w jakim rodzaju gramatycznym bedzie 
 *		   kazda z nazw. W przypadku, gdy sie poda za malo 
 *		   rodzajow (albo nie poda sie ich wcale), funkcja sama 
 *		   dopelni tablice rodzajow tak, by kazda dodawana nazwa miala
 *		   swoj rodzaj. Jesli zajdzie potrzeba dopelnienia, a bedzie 
 *		   podany chociaz jeden rodzaj, funkcja dopelni pierwszym
 *		   podanym rodzajem. Gdy sie nie poda zadnego rodzaju, 
 *		   zostanie uzyty domyslny rodzaj - zenski. Rodzaje powinny 
 *		   miec specjalna, przetworzona wartosc - patrz naglowek 
 *		   funkcji query_rodzaj().
 * Argumenty     : name - nazwa, lub tablica nazw do dodania, w lp.
 *		   przyp - przypadek
 *		   rodzaje - rodzaj, lub tablica z prawdziwymi rodzajami
 *			     (w formie, w jakiej mudlib je zapamietuje).
 */
public varargs void
set_name(mixed name, int przyp = 0, mixed rodzaje = ({}))
{ 
    int x, wypelniany_rodzaj, size_rodzaje;
    
    if (!pointerp(name))
        name = ({ name });

    if (!pointerp(rodzaje))
       rodzaje = ({ rodzaje });
       
    size_rodzaje = sizeof(rodzaje);
    if (size_rodzaje)
        wypelniany_rodzaj = rodzaje[0];
    else 
        wypelniany_rodzaj = PL_ZENSKI + 1;
        
    if ((x = (sizeof(name) - size_rodzaje)) > 0)
        while (x--)
            rodzaje += ({ wypelniany_rodzaj });
        
    usun_stare_nazwy(name, przyp, 0, 0);
    obj_names[przyp] = name + obj_names[przyp];
    obj_rodzaje[przyp] = rodzaje + obj_rodzaje[przyp];
}

/*
 * Nazwa funkcji : add_name
 * Opis          : Dodaje na koniec tablicy nazw jedna lub wiecej nazwe w 
 *		   podanym przypadku, w liczbie pojedynczej. 
 *		   Mozna rowniez podac w jakim rodzaju gramatycznym 
 *		   bedzie kazda z nazw. W przypadku, gdy sie poda za malo 
 *		   rodzajow (albo nie poda sie ich wcale), funkcja sama 
 *		   dopelni tablice rodzajow tak, by kazda dodawana nazwa 
 *		   miala swoj rodzaj. Jesli zajdzie potrzeba 
 *		   dopelnienia, a bedzie podany chociaz jeden rodzaj, 
 *		   funkcja dopelni pierwszym podanym rodzajem. Gdy sie nie 
 *		   poda zadnego rodzaju, zostanie uzyty domyslny rodzaj - 
 *		   zenski. Rodzaje powinny miec specjalna, przetworzona 
 *		   wartosc - patrz naglowek funkcji query_rodzaj().
 * Argumenty     : name - nazwa, lub tablica nazw do dodania, w lp.
 *		   przyp - przypadek
 *		   rodzaje - rodzaj, lub tablica z prawdziwymi rodzajami
 *			     (w formie, w jakiej mudlib je zapamietuje).
 */
public varargs void
add_name(mixed name, int przyp = 0, mixed rodzaje = ({}))
{
    int x, wypelniany_rodzaj, size_rodzaje;
    
    if (!pointerp(name))
        name = ({ name });
        
    if (!pointerp(rodzaje))
       rodzaje = ({ rodzaje });

    size_rodzaje = sizeof(rodzaje);
    if (size_rodzaje)
        wypelniany_rodzaj = rodzaje[0];
    else 
        wypelniany_rodzaj = PL_ZENSKI + 1;
        
    if ((x = (sizeof(name) - size_rodzaje)) > 0)
        while (x--)
            rodzaje += ({ wypelniany_rodzaj });

    if (usun_stare_nazwy(name, przyp, 0, 1))
    {
	obj_names[przyp] += name;
	obj_rodzaje[przyp] += rodzaje;
    }
}

/*
 * Nazwa funkcji : remove_name
 * Opis          : Usuwa podana nazwe (lub liste nazw) z tablicy
 *		   nazw w podanym przypadku.
 * Argumenty     : mixed name - nazwa lub lista nazw do usuniecia.
 *		   int przyp - przypadek w ktorym sa nazwy do usuniecia.
 */
public varargs void
remove_name(mixed name, int przyp) 
{ 
    usun_stare_nazwy(name, przyp, 0, 0);
}

/*
 * Nazwa funkcji : query_name
 * Opis          : Zwraca pierwsza albo wszystkie nazwy z danego przypadku,
 *		   w liczbie pojedynczej.
 * Argumenty     : int arg - czy ma zwrocic tylko nazwe glowna, czy wszystki
 *			     w danym przypadku.
 *		   int przyp - przypadek, w ktorym maja byc nazwy
 * Funkcja zwraca: Nazwe lub tablice nazw w zadanym przypadku.
 */
varargs public mixed
query_name(int arg, int przyp) 
{ 
    return query_list(obj_names[przyp], arg);
}

/*
 * Nazwa funkcji  : query_nazwa
 * Opis           : Zwraca nazwe obiektu w okreslonym przypadku, o ile obiekt
 *                  zostal odmieniony za pomoca dodaj_nazwy() lub 
 *		    ustaw_nazwe(). W przeciwnym wypadku zwraca pierwsza
 *		    nazwe z mianownika.
 * Argumenty      : int przyp - przypadek.
 * Funkcja zwraca : string - nazwa obiektu.
 */
varargs public string
query_nazwa(int przyp)
{
    return obj_names[przyp][0];
}

/*
 * Nazwa funkcji : set_pname
 * Opis          : Ustawia na poczatku tablicy nazw jedna lub wiecej nazwe w 
 *		   podanym przypadku, w liczbie mnogiej. (nazwa glowna). 
 *		   Mozna rowniez podac w jakim rodzaju gramatycznym bedzie 
 *		   kazda z nazw. W przypadku, gdy sie poda za malo 
 *		   rodzajow (albo nie poda sie ich wcale), funkcja sama 
 *		   dopelni tablice rodzajow tak, by kazda dodawana nazwa miala
 *		   swoj rodzaj. Jesli zajdzie potrzeba dopelnienia, a bedzie 
 *		   podany chociaz jeden rodzaj, funkcja dopelni pierwszym
 *		   podanym rodzajem. Gdy sie nie poda zadnego rodzaju, 
 *		   zostanie uzyty domyslny rodzaj - zenski. Rodzaje powinny 
 *		   miec specjalna, przetworzona wartosc - patrz naglowek 
 *		   funkcji query_rodzaj().
 * Argumenty     : name - nazwa, lub tablica nazw do dodania, w lmn.
 *		   przyp - przypadek
 *		   rodzaje - rodzaj, lub tablica z prawdziwymi rodzajami
 *			     (w formie, w jakiej mudlib je zapamietuje).
 */
public varargs void
set_pname(mixed pname, int przyp, mixed rodzaje = ({})) 
{ 
    int x, wypelniany_rodzaj, size_rodzaje;
    
    if (!pointerp(pname))
        pname = ({ pname });

    if (!pointerp(rodzaje))
       rodzaje = ({ rodzaje });

    size_rodzaje = sizeof(rodzaje);
    if (size_rodzaje)
        wypelniany_rodzaj = rodzaje[0];
    else 
        wypelniany_rodzaj = PL_ZENSKI + 1;

    if ((x = (sizeof(pname) - size_rodzaje)) > 0)
        while (x--)
            rodzaje += ({ wypelniany_rodzaj });

    usun_stare_nazwy(pname, przyp, 1, 0);
    obj_pnames[przyp] = pname + obj_pnames[przyp];
    obj_prodzaje[przyp] = rodzaje + obj_prodzaje[przyp];
}

/*
 * Nazwa funkcji : add_pname
 * Opis          : Dodaje na koniec tablicy nazw jedna lub wiecej nazwe w 
 *		   podanym przypadku, w liczbie mnogiej. 
 *		   Mozna rowniez podac w jakim rodzaju gramatycznym 
 *		   bedzie kazda z nazw. W przypadku, gdy sie poda za malo 
 *		   rodzajow (albo nie poda sie ich wcale), funkcja sama 
 *		   dopelni tablice rodzajow tak, by kazda dodawana nazwa 
 *		   miala swoj rodzaj. Jesli zajdzie potrzeba 
 *		   dopelnienia, a bedzie podany chociaz jeden rodzaj, 
 *		   funkcja dopelni pierwszym podanym rodzajem. Gdy sie nie 
 *		   poda zadnego rodzaju, zostanie uzyty domyslny rodzaj - 
 *		   zenski. Rodzaje powinny miec specjalna, przetworzona 
 *		   wartosc - patrz naglowek funkcji query_rodzaj().
 * Argumenty     : name - nazwa, lub tablica nazw do dodania, w lmn.
 *		   przyp - przypadek
 *		   rodzaje - rodzaj, lub tablica z prawdziwymi rodzajami
 *			     (w formie, w jakiej mudlib je zapamietuje).
 */
public varargs void
add_pname(mixed pname, int przyp, mixed rodzaje = ({})) 
{ 
    int x, wypelniany_rodzaj, size_rodzaje;
    
    if (!pointerp(pname))
        pname = ({ pname });
        
    if (!pointerp(rodzaje))
       rodzaje = ({ rodzaje });
        
    size_rodzaje = sizeof(rodzaje);
    if (size_rodzaje)
        wypelniany_rodzaj = rodzaje[0];
    else 
        wypelniany_rodzaj = PL_ZENSKI + 1;

    if ((x = (sizeof(pname) - size_rodzaje)) > 0)
        while (x--)
            rodzaje += ({ wypelniany_rodzaj });

    if (usun_stare_nazwy(pname, przyp, 1, 1))
    {
	obj_pnames[przyp] += pname;
	obj_prodzaje[przyp] += rodzaje;
    }
}

/*
 * Nazwa funkcji : remove_pname
 * Opis          : Usuwa nazwe (lub liste nazw) w liczbie mnogiej
 *		   z tablicy nazw o podanym przypadku.
 * Argumenty     : mixed pname - nazwa lub lista nazw do usuniecia.
 *		   int przyp - przypadek w ktorym sa nazwy do usuniecia.
 */
public varargs void
remove_pname(mixed pname, int przyp) 
{ 
    usun_stare_nazwy(pname, przyp, 1, 0);
}

/*
 * Nazwa funkcji  : query_pname
 * Opis           : W zaleznosci od argumentow, zwraca liste wszystkich
 *                  nazw w lmn w podanym przypadku, albo tylko glowna.
 * Argumenty      : int arg - 1, gdy ma zwrocic liste wszystkich nazw
 *			      w danym przypadku; 0 gdy tylko glowna.
 *                  int przyp - dany przypadek.
 * Funkcja zwraca : mixed - tablice z nazwami w podanym przypadku, albo 
 *                          tylko nazwe glowna.
 */
varargs public mixed
query_pname(int arg, int przyp)
{
    return query_list(obj_pnames[przyp], arg);
} 

/*
 * Nazwa funkcji  : query_pnazwa
 * Opis           : Zwraca nazwe obiektu w liczbie mnogiej, w okreslonym 
 * 		    przypadku, o ile obiekt zostal odmieniony za pomoca 
 *		    dodaj_nazwy() lub ustaw_nazwe(). W przeciwnym wypadku
 *		    zwraca pierwsza nazwe z tablicy mianownika.
 * Argumenty      : int przyp - przypadek.
 * Funkcja zwraca : string - nazwa obiektu w liczbie mnogiej.
 */
varargs public string
query_pnazwa(int przyp)
{
    return obj_pnames[przyp][0];
}

#if 0
/*
 * Nazwa funkcji : set_adj
 * Opis          : Funkcja NIE istnieje. Uzyj dodaj_przym().
 */
public varargs void
set_adj(mixed adj, int przyp) 
{ 
    obj_adjs[przyp] = add_list(obj_adjs[przyp], adj, 1); 
}
#endif

#if 0
/*
 * Nazwa funkcji : add_adj
 * Opis          : Funkcja NIE istnieje. Uzyj dodaj_przym().
 */
public varargs void
add_adj(mixed adj, int przyp) 
{ 
    obj_adjs[przyp] = add_list(obj_adjs[przyp], adj, 0); 
}
#endif

/*
 * Nazwa funkcji : remove_adj
 * Opis          : Usuwa podane przymiotniki z tablicy z przymiotnikami
 * Argumenty     : mixed adj - string albo tablica z przymiotnikami
 *			       do usuniecia. Powinny one byc podane
 *			       w rodzaju meskim w liczbie pojedynczej.
 */
public void
remove_adj(mixed adj) 
{ 
    int i, pos;
    
    if (obj_no_change)
	return ;       		/* All changes has been locked out */
	
    if (!adj)
        return ;

    if (!pointerp(adj))
        adj = ({ adj });
    
    i = sizeof(adj);
    while (--i >= 0)
    {
        pos = member_array(adj[i], obj_przym[0]);
        if (pos >= 0)
        {
            obj_przym[0] = exclude_array(obj_przym[0], pos, pos);
            obj_przym[1] = exclude_array(obj_przym[1], pos, pos);
        }
    }
}
  
/*
 * Nazwa funkcji : query_adj
 * Opis          : Zwraca przymiotnik(i) obiektu w liczbie pojedynczej.
 * Argumenty     : int arg   - jesli prawdziwy, zostana zwrocone
 *			       wszystkie przymiotniki danego przypadku.
 *		   int przyp - numer przypadku.
 * Funkcja zwraca: mixed     - int 0    - nie ma zadnych przymiotnikow.
 *			       string   - jeden przymiotnik, jesli arg
 *					  jest falszywy (rowny 0).
 *			       string * - tablica z wszystkimi
 *					  przymiotnikami w danym przypadku.
 */
varargs public mixed
query_adj(int arg, int przyp)
{ 
     return query_przym(arg, przyp);
}

/*
 * Do uzytku przez /std/player/savevars_sec.c i /std/heap.c
 */
public mixed
query_przymiotniki()
{
    return obj_przym + ({ });
}

/*
 * Nazwa funkcji  : set_padj
 * Opis          : Funkcja istnieje tylko dla kompatybilnosci. Nie uzywac.
 */
public void
set_padj(mixed elem)
{
#if 0
    add_list(obj_padjs, elem, 1);
#endif
}

/*
 * Nazwa funkcji  : add_padj
 * Opis          : Funkcja istnieje tylko dla kompatybilnosci. Nie uzywac.
 */
public varargs void
add_padj(mixed elem, int przyp)
{
#if 0
    obj_padjs[przyp] = add_list(obj_padjs[przyp], elem, 0);
#endif
}

/*
 * Nazwa funkcji : query_padj
 * Opis          : Zwraca przymiotnik(i) obiektu w liczbie mnogiej.
 * Argumenty     : int arg   - jesli prawdziwy, zostana zwrocone
 *			       wszystkie przymiotniki danego przypadku.
 *		   int przyp - numer przypadku.
 * Funkcja zwraca: mixed     - int 0    - nie ma zadnych przymiotnikow.
 *			       string   - jeden przymiotnik, jesli arg
 *					  jest falszywy (rowny 0).
 *			       string * - tablica z wszystkimi
 *					  przymiotnikami w danym przypadku.
 */
public varargs mixed
query_padj(int arg, int przyp)
{
    return query_pprzym(arg, przyp);
}

#if 0
/*
 * Function name: set_short
 * Description:   Sets the string to return for short description.
 *                If not defined, the first name is used instead.
 * Arguments:     short: The short description
 * *UWAGA* Ta funkcja ZOSTALA USUNIETA. W zamian uzywaj
 *         funkcji ustaw_shorty().
 */
public void
set_short(string short)
{
    if (!obj_no_change)
	obj_short = short;
}
#endif

#if 0
/*
 * Function name: set_pshort
 * Description:   Sets the string to return for plural short description.
 *                If not defined, 0 is returned.
 * Arguments:     pshort: The plural short description
 * *UWAGA* Ta funkcja ZOSTALA USUNIETA. Wzamian uzywaj
 *         funkcji ustaw_shorty().
 */
public void
set_pshort(string pshort)
{
    if (!obj_no_change)
	obj_pshort = pshort;
}
#endif

/*
 * Function name: set_long
 * Description  : This function sets the long description of this object.
 *                It can be a string or VBFC in string or functionpointer
 *                form.
 * Arguments    : mixed long - the long description.
 */
public void
set_long(mixed long)
{
    if (!obj_no_change)
        obj_long = long;
}

/*
 * Function name: set_lock
 * Description:   Locks out all changes to this object through set_ functions.
 */
public void
set_lock()
{
    obj_no_change = 1;
}

/*
 * Function name: query_lock
 * Description:   Gives the lock status of this object
 * Returns:       True if changes to this object is locked out
 */
public int
query_lock()
{
    return obj_no_change;
}

/*
 * Function name: set_no_show
 * Description:   Don't show these objects.
 */
public void
set_no_show()
{
    obj_no_show = 1;
    set_no_show_composite(1);
}

/*
 * Function name: unset_no_show
 * Description:   Show it again. Note that if you want the object to appear
 *                in composite descriptions you have to set_no_show_composite(0)
 *                to since it is automatically set when setting no_show.
 */
public void
unset_no_show() 
{ 
    obj_no_show = 0; 
}

/*
 * Function name: query_no_show
 * Description:   Return no show status.
 */
public int
query_no_show()
{
    return obj_no_show;
}

/*
 * Function name: set_no_show_composite
 * Description:   Don't show this object in composite descriptions, otherwise
 *                this object is part of the game like any other.
 * Arguments:     1 - don't show, 0 - show it again
 */
void
set_no_show_composite(int i) 
{ 
    obj_no_show_c = i; 
}

/*
 * Function name: unset_no_show_composite
 * Description:   Show an object in compisite descriptions again.
 */
void
unset_no_show_composite() 
{ 
    obj_no_show_c = 0; 
}

/*
 * Function name: query_no_show_composite
 * Description:   Return status if to be shown in composite descriptions
 */
int
query_no_show_composite() 
{ 
    return obj_no_show_c; 
}

/*
 * Function name:  add_magic_effect
 * Description:    Notifies this object that it has been placed
 *                 a magical effect upon it.
 * Arguments:      The effect object, or the filename of
 *                 the code which handles the magical 
 *                 effect. (Filename for a shadow.)
 */
varargs void
add_magic_effect(mixed what)
{
    if (!what)
	what = previous_object();

    if (!pointerp(magic_effects))
	magic_effects = ({ what });
    else
	magic_effects += ({ what });
}

/*
 * Function name:  remove_magic_effect
 * Description:    Removes the magical effect from the
 *                 list of effects affecting this object.
 * Arguments:      What effect.
 * Returns:        If the effect was found.
 */
varargs int
remove_magic_effect(mixed what)
{
    int il;

    if (!what)
	what = previous_object();

    il = member_array(what, magic_effects);

    if (il == -1)
	return 0;

    magic_effects = exclude_array(magic_effects, il, il);
    return 1;
}

/*
 * Function name:  query_magic_effects
 * Description:    Returns the magical effects upon this
 *                 object.
 */
object *
query_magic_effects()
{
    if (!pointerp(magic_effects))
	magic_effects = ({});
    magic_effects -= ({ 0 });
    return magic_effects;
}

#if 0
/*
 * Function name: query_magic_res
 * Description:   Return the total resistance of worn objects
 * Arguments:     prop - The searched for property.
 * Returns:       How resistant this object is to that property
 */
public int
query_magic_res(string prop)
{
    int res;

    res = this_object()->query_prop(PRE_OBJ_MAGIC_RES + prop);
    return res > 100 ? 100 : res;
}
#endif

/*
 * Function name: query_magic_res
 * Description:   Return the total resistance for this object
 * Arguments:     prop - The searched for property.
 */
int
query_magic_res(string prop)
{
    int no_objs, max, max_add, max_stat, i;
    mixed value;
    object *list;

    list = this_object()->query_magic_effects();
    
    if (!sizeof(list))
	return (int)this_object()->query_prop(PRE_OBJ_MAGIC_RES + prop);

    max_add = 100;

    for (i = 0; i < sizeof(list); i++)
    {
	value = list[i]->query_magic_protection(prop, this_object());
	if (intp(value))
	    value = ({ value, 0 });
	if (pointerp(value))
	{
	    if ((sizeof(value) > 0) && !value[1])
		max_stat = max_stat > value[0] ? max_stat : value[0];
	    else
		max_add = max_add * (100 - value[0]) / 100;
	}
    }

    if (max_add > 0)
	max_add = 100 - max_add;

    max = max_stat > max_add ? max_stat : max_add;
    max += (int)this_object()->query_prop(PRE_OBJ_MAGIC_RES + prop);

    return max < 100 ? max : 100;
}

/*
 * Function name:  query_magic_protection
 * Description:    This function should return the
 *                 amount of protection versus an 
 *                 attack of 'prop' on 'obj'.
 * Arguments:      prop - The element property to defend.
 *                 protectee  - Magic protection for who or what? 
 */
varargs mixed
query_magic_protection(string prop, object protectee = previous_object())
{
    if (protectee == this_object())
	return query_prop(prop);
    else
	return 0;
}

/* 

   Items (pseudo look) 
   -------------------

*/
	

/*
 * Function name: item_id
 * Description  : Identify items in the object. This means that the function
 *                will return true if the argument has been added to this
 *                object using add_item().
 * Arguments    : string str - the name to test.
 * Returns      : int 1/0 - is added with add_item() or not.
 */
public int
item_id(string str)
{
    int size;
    
    if (!obj_items)
    {
	return 0;
    }

    size = sizeof(obj_items);
    while(--size >= 0)
    {
	if (member_array(str, obj_items[size][0]) >= 0)
	{
	    return 1;
	}
    }

    return 0;
}

/*
 * Function name: add_item
 * Description:   Adds an additional item to the object. The first 
 *		  argument is a single string or an array of 
 *      	  strings holding the possible name(s) of the item.
 *		  The second argument is the long description of 
 *		  the item. add_item can be repeatedly called with 
 *                new items. The second argument can be VBFC.
 * Arguments:	  names: Alternate names for the item, 
 *                mixed desc: desc of the item (string or VBFC)
 * Returns:	  True or false.
 */
public int
add_item(mixed names, mixed desc)
{
    if (query_prop(ROOM_I_NO_EXTRA_ITEM))
	return 0;

    if (!pointerp(names))
	names = ({ names });

    if (obj_items)
	obj_items = obj_items + ({ ({ names, desc }) });
    else
	obj_items = ({ ({ names, desc }) });
}

/*
 * Function name: set_add_item
 * Description:   Sets the 'pseudo items' of an object.
 * Arguments:     pseudo_items - a mixed array of pseudo items to be added.
 */
public void
set_add_item(mixed pseudo_items)
{
    obj_items = pseudo_items;
}

/*
 * Function name: query_item
 * Description:   Get the additional items array.
 * Returns:       Item array, see below:

  [0] = array
     [0] ({ "name1 of item1", "name2 of item1",... })
     [1] "This is the description of the item1."
  [1] = array
     [0] ({ "name1 of item2", "name2 of item2", ... })
     [1] "This is the description of the item2."
*/
public mixed
query_item() 
{ 
    return obj_items; 
}

/*
 * Function name: remove_item
 * Description:   Removes one additional item from the additional item list
 * Arguments:     name: name of item to remove.
 * Returns:       True or false. (True if removed successfully)
 */
public int
remove_item(string name)
{
    int i;
    
    if (!pointerp(obj_items))
	return 0;
    
    if (query_prop(ROOM_I_NO_EXTRA_ITEM))
	return 0;
    for (i = 0; i < sizeof(obj_items); i++)
	if (member_array(name, obj_items[i][0]) >= 0 )
	{
	    obj_items = exclude_array(obj_items, i, i);
	    return 1;
	}
    return 0;
}

/*
 * Function name: cmditem_action
 * Description:   Find and execute a command for a specific command item
 * Arguments:     str: The rest of the command
 */
public int
cmditem_action(string str)
{
    string verb = query_verb();
    int n = sizeof(obj_cmd_items);
    int cmd, arg;
    mixed ex;

    notify_fail(capitalize(verb) + " co?\n");

    if (!str)
        return 0;

    while (n--)
        if ((cmd = member_array(verb, obj_cmd_items[n][1])) != -1 &&
            (arg = member_array(str, obj_cmd_items[n][0])) != -1 &&
            (ex = check_call(obj_cmd_items[n][2][cmd <
                      sizeof(obj_cmd_items[n][2]) ? cmd : 0])))
        {
            if (stringp(ex))
                write(ex);

            return 1;
        }

    notify_fail(capitalize(verb) + " co?\n");
    return 0;
}

/*
 * Function name: add_cmd_item
 * Description:   Adds a specific item with associated commands to the
 *                object. These are similar to the normal items but
 *                they add commands which can then executed by players.
 *                The first argument is a single string or an array of 
 *                strings holding the possible name(s) of the item.
 *                The second argument is the command / description array of 
 *                the item. add_cmd_item can be repeatedly called with 
 *                new items.
 * Arguments:     names: Alternate names for the item, 
 *                cmd_arr:  Commands to give to get the desc
 *                desc_arr: descs of the item for each command
 * Returns:       True or false.
*/
public int
add_cmd_item(mixed names, string *cmd_arr, mixed desc_arr)
{
    if (query_prop(ROOM_I_NO_EXTRA_ITEM))
	return 0;

    if (!pointerp(names))
	names = ({ names });

    if (!pointerp(cmd_arr))
	cmd_arr = ({ cmd_arr });

    if (!pointerp(desc_arr))
	desc_arr = ({ desc_arr });

    if (!pointerp(obj_commands))
	obj_commands = ({ });

    if (obj_cmd_items) 
	obj_cmd_items = obj_cmd_items + ({ ({ names, cmd_arr, desc_arr }) });
    else
	obj_cmd_items = ({ ({ names, cmd_arr, desc_arr }) });

    if (sizeof(cmd_arr))
	obj_commands = obj_commands + (cmd_arr - obj_commands);
}

/*
 * Function name: query_cmd_item
 * Description:   Get the command items array.
 * Returns:       Item array, see below:

  [0] = array
     [0] ({ "name1 of item1", "name2 of item1",... })
     [1] ({ "command1", "command2", .... "commandN" })
     [2] ({ 
	    "string to print if command1 given",
	    "string to print if command2 given",
		   ......
	    "string to print if commandN given",
	 })

  Example:
	({ 
	    ({ "flower", "viola" }), 
	    ({ "smell", "taste", "get" }),
	    ({ "It smells nice", "It tastes awful!", "@@tryget" }),
	})

*/
public mixed
query_cmd_item() 
{  
    return obj_cmd_items; 
}
    
/*
 * Function name: remove_cmd_item
 * Description:   Removes one command item from the command item list
 * Arguments:     name: name of item to remove.
 * Returns:       True or false. (True if removed successfully)
 */
public int
remove_cmd_item(string name)
{
    int i, il;
    string *cmd_arr;
    
    if ( !pointerp(obj_cmd_items) ) return 0;
    
    if (query_prop(ROOM_I_NO_EXTRA_ITEM)) return 0;
    for ( i = 0; i<sizeof(obj_cmd_items); i++)
	if ( member_array(name, obj_cmd_items[i][0])>=0 )
	{
	    obj_cmd_items = exclude_array(obj_cmd_items,i,i);
	    obj_commands = ({});
	    for (il = 0; il < sizeof(obj_cmd_items); il++)
	    {
		cmd_arr = obj_cmd_items[il][1];
		if (sizeof(cmd_arr))
		    obj_commands = obj_commands + (cmd_arr - obj_commands);
	    }
	    return 1;
	}
    return 0;
}

/*
 * Function name: set_trusted
 * Description:   Sets the effuserid to the userid of this object. This is
 *                used by the 'trust' command mainly on wiztools.
 * Arguments:     arg - 1 = set the euid of this object.
 *                      0 = remove the euid.
 */
public void
set_trusted(int arg) 
{
    object cobj;

    if (!objectp(cobj = previous_object()))
	return;

    if ((geteuid(cobj) != getuid()) &&
	(geteuid(cobj) != SECURITY->query_domain_lord(getuid())) &&
	(SECURITY->query_wiz_rank(geteuid(cobj)) < WIZ_ARCH) &&
	(geteuid(cobj) != ROOT_UID))
    {
	return;
    }

    if (arg)
	seteuid(getuid(this_object())); 
    else
	seteuid(0);
}

/*
 * Function name: cut_sig_fig
 * Description:   Will reduce the number given to a new number with specified
 *                number of significant numbers.
 * Arguments:     fig - the number to correct.
 *		  num - number of significant numbers.
 * Returns:       the number with two significant numbers
 */
public int
cut_sig_fig(int fig, int num)
{
    int fac, lim;
    fac = 1;

    lim = ftoi(pow(10.0, itof(num)));
    while(fig > lim)
    {
	fac = fac * 10;
	fig = fig / 10;
    }

    return fig * fac;
}

/*
 * Function name: appraise_value
 * Description:   This function is called when someon tries to appraise value
 *                of this object.
 * Arguments:     num - use this number instead of skill if given.
 */
public string
appraise_value(int num)
{
    int value, skill, seed;

    if (!num)
	skill = this_player()->query_skill(SS_APPR_VAL);
    else
	skill = num;

/*
 * To nie mialo wiekszego sensu... /Alvin.
 *    skill = 1000 / (skill + 1);
 */
    skill = ((100 - skill) * 6 / 10) + 1;
    value = query_prop(OBJ_I_VALUE);
    sscanf(OB_NUM(this_object()), "%d", seed);
    skill = random(skill, seed);
    value = cut_sig_fig(value + ((skill % 2 ?: -1) * skill * value / 100), 2);

    return value + " miedziak" + (value == 1 ? "a" :
	(value%10 <= 4 && value%10 >= 2 && value%100 != 1 ? "i" : "ow"));
}

/*
 * Function name: appraise_weight
 * Description:   This function is called when someon tries to appraise weight
 *                of this object.
 * Arguments:     num - use this number instead of skill if given.
 */
public string
appraise_weight(int num)
{
    int value, skill, seed, obj;
    string str;

    if (!num)
	skill = this_player()->query_skill(SS_APPR_OBJ);
    else
	skill = num;

/*
 * To nie mialo wiekszego sensu... /Alvin.
 *    skill = 1000 / (skill + 1);
 */
    skill = ((100 - skill) * 6 / 10) + 1;
    value = query_prop(OBJ_I_WEIGHT);
    sscanf(OB_NUM(this_object()), "%d", seed);
    skill = random(skill, seed);
    value = cut_sig_fig(value + ((skill % 2 ?: -1) * skill * value / 100), 2);

    if (value > 10000)
    {
        obj = value / 1000;
        str = obj + " ";
        
        if (obj == 1)
            str += "kilogram";
        else if (obj % 10 > 1 && obj % 10 < 5 &&
            (obj % 100 < 10 || obj % 100 > 20))
            str += "kilogramy";
        else
            str += "kilogramow";
        
	return str;
    }
    else
    {
        str = value + " ";
        
        if (value == 1)
            str += "gram";
        else if (value % 10 > 1 && value % 10 < 5 &&
            (value % 100 < 10 || value % 100 > 20))
            str += "gramy";
        else
            str += "gramow";
        
	return str;
    }
}

/*
 * Function name: appraise_volume
 * Description:   This function is called when someon tries to appraise volume
 *                of this object.
 * Arguments:     num - use this number instead of skill if given.
 */
public string
appraise_volume(int num)
{
    int value, skill, seed, obj;
    string str;

    if (!num)
	skill = this_player()->query_skill(SS_APPR_OBJ);
    else
	skill = num;

/*
 * To nie mialo wiekszego sensu... /Alvin.
 *    skill = 1000 / (skill + 1);
 */
    skill = ((100 - skill) * 6 / 10) + 1;
    value = query_prop(OBJ_I_VOLUME);
    sscanf(OB_NUM(this_object()), "%d", seed);
    skill = random(skill, seed);
    value = cut_sig_fig(value + ((skill % 2 ?: -1) * skill * value / 100), 2);

    if (value > 10000)
    {
        obj = value / 1000;
        str = obj + " ";
        
        if (obj == 1)
            str += "litr";
        else if (obj % 10 > 1 && obj % 10 < 5 &&
            (obj % 100 < 10 || obj % 100 > 20))
            str += "litry";
        else
            str += "litrow";
        
	return str;
    }
    else
    {
        str = value + " ";
        
        if (value == 1)
            str += "mililitr";
        else if (value % 10 > 1 && value % 10 < 5 &&
            (value % 100 < 10 || value % 100 > 20))
            str += "mililitry";
        else
            str += "mililitrow";
        
	return str;
    }
}

/*
 * Function name: appraise_object
 * Description:   This function is called when someon tries to appraise this
 *                object.
 * Arguments:    num - use this number instead of skill if given.
 */
public void
appraise_object(int num)
{
    write(this_object()->long(0, this_player()) + "\n");
    write("Oceniasz, ze " + short(PL_MIA) + " waz" + 
	koncowka("y", "y", "y", "a", "a") + " " + appraise_weight(num) +
	", zas " + koncowka("jego", "jej", "jego", "ich", "ich") +
	" objetosc wynosi " + appraise_volume(num) + ".\n");
    write("Wydaje ci sie, ze " + (query_tylko_mn() ? "sa" : "jest") + 
	" war" + koncowka("t", "ta", "te", "ci", "te") + " " +
	appraise_value(num) + ".\n");
        
    if (this_object()->check_recoverable() == 1)
	write("Wyglada na to, ze mogl" + koncowka("by", "aby", "oby",
	    "iby", "yby") + " ci jeszcze dlugo sluzyc.\n");
}

/*
 * Function name: check_recoverable
 * Description:   This function checks if the object can be stored as
 *                recoverable or not.
 * Arguments    : flag - if this flag is true, then the may_not_recover
 *                       function is not called. This is done to allow
 *                       a 'fails to glow' message.
 * Returns:       1 / 0 depending on outcome.
 */
public nomask varargs int
check_recoverable(int flag)
{
    string str, path, arg;

    /* Armours and weapons have a chance to fail on recovery.
     */
    if (!flag && this_object()->may_not_recover())
    {
	return 0;
    }

    /* Check for recover string */
    str = (string)this_object()->query_recover();
    if (strlen(str) > 0)
    {
	if (sscanf(str, "%s:%s", path, arg) != 2)
	{
	    path = str;
	    arg = 0;
	}

	/* Check for arg to be <= 128 bytes long */
	if (strlen(arg) <= 128)
	    return 1;
    }

    return 0;
}


/*
 * Function namn: query_value
 * Description:   Does the same thing as query_prop(OBJ_I_VALUE)
 *                but is needed since unique_array() doesn't send an
 *                argument.
 * Returns:       The value
 */
int
query_value() 
{ 
    return query_prop(OBJ_I_VALUE); 
}

/*
 * Nazwa funkcji : search_fun
 * Opis          : Sprawdza, czy graczowi udalo sie, poza ukrytymi w obiekcie
 *                 obiektami, znalezc cos interesujacego.
 * Argumenty     : string str - argument komendy
 *                 int trail  - uzyta komenda: 0, gdy 'szukaj',
 *                                             1, gdy 'przeszukaj'.
 * Zwraca        : Komunikat, ktory ma byc wyswietlony graczowi, lub 0, jesli
 *                 nie udalo mu sie znalezc niczego interesujacego.
 * Uwaga         : Jesli maskujesz te funkcje, zamiast zwracac 0, wywoluj jej
 *                 oryginalna wersje. (return ::search_fun(str, trail);)
 */
static string
search_fun(string str, int trail)
{
    return 0;
}

/*
 * Nazwa funkcji : search_now
 * Opis          : Odpala wynik przeszukania.
 * Argumenty     : object searcher - przeszukujacy
 *                 string str      - argument komendy
 *                 int trail       - uzyta komenda: 0, gdy 'szukaj',
 *                                                  1, gdy 'przeszukaj'.
 */
public void
search_now(object searcher, string str, int trail)
{
    string hidden, fun;
    object *live, *dead, *found;

    if (!searcher)
        return;

    set_this_player(searcher);
    if (query_prop(ROOM_I_IS))
    {
        found = all_inventory(this_object()) - ({searcher});
        found = filter(found, &search_hidden(, searcher));
        if (sizeof(found))
        {
            dead = filter(found, &is_live_dead(, 0));
            live = filter(found, &is_live_dead(, 1));
            found = live + dead;
        }
        if (sizeof(live))
            hidden = COMPOSITE_LIVE(found, PL_BIE);
        else if (sizeof(dead))
            hidden = COMPOSITE_DEAD(found, PL_BIE);
        if (strlen(hidden))
        {
            write("Znajdujesz " + hidden + ".\n");
            say(QCIMIE(searcher, PL_MIA) + " znajduje " + QCOMPDEAD(PL_BIE)
              + ".\n", ({searcher}) + live);
            live->catch_msg(QCIMIE(searcher, PL_MIA) + " znajduje cie.\n");
	}
    }

    if (fun = search_fun(str, trail))
        write(fun);
    else
        if (!sizeof(found))
            write("Nie udalo ci sie znalezc niczego interesujacego.\n");
}

/*
 * Function name: is_live_dead
 * Description  : This function can be used to check whether an object
 *                is a living object or whether it is non-living. Basically
 *                it checks whether the outcome for living(obj) is equal to
 *                the second argument "what".
 * Arguments    : object obj - the object to check.
 *                int what   - when 1, return whether obj is living, when 0
 *                             return whether obj is dead. Note: other "true"
 *                             values for "what" will not work.
 * Returns      : int 1/0 - living/dead if "what" is 1, dead/living if
 *                          "what" is 0.
 */
int
is_live_dead(object obj, int what)
{
    return (living(obj) == what);
}

int
search_hidden(object obj, object who)
{
    if (!obj->query_prop(OBJ_I_HIDE))
	return 0;

    if (who && !who->query_wiz_level() &&
	    who->query_skill(SS_AWARENESS) < 
		obj->query_prop(OBJ_I_HIDE))
	return 0;
    obj->remove_prop(OBJ_I_HIDE);
    return 1;
}

/*
 * Nazwa funkcji : search_object
 * Opis          : Ktos usiluje przeszukac obiekt.
 * Argumenty     : string str - argument komendy
 *                 int trail  - uzyta komenda: 0, gdy 'szukaj',
 *                                             1, gdy 'przeszukaj'.
 */
public void
search_object(string str, int trail)
{
    int time;
    object obj;

    time = query_prop(OBJ_I_SEARCH_TIME) + 5;

    if (time < 1)
        search_now(this_player(), str, trail);
    else
    {
        remove_alarm(search_alarm);
        search_alarm = set_alarm(itof(time), 0.0,
                                 &search_now(this_player(), str, trail));

        seteuid(getuid(this_object()));
        obj = clone_object("/std/paralyze");
        obj->set_standard_paralyze(trail ? "przeszukiwac "
          + (query_prop(ROOM_I_IS) ? str :
                this_object()->short(this_player(), PL_BIE)) :
            "szukac" + (str ? " " + str : ""));
        obj->set_stop_fun("stop_search");
        obj->set_remove_time(time);
        obj->move(this_player(), 1);
    }
}

/*
 * Function name: stop_search
 * Description:   This function is called if the player decides to stop his
 *                search.
 * Arguments:     arg - If string extra string when player made a command
 *                if object, the time ran out on the paralyze.
 * Returns:       If this function should not allow the player to stop search
 *                it shall return 1.
 */
varargs int
stop_search(mixed arg)
{
    if (!objectp(arg))
	remove_alarm(search_alarm);
    return 0;
}

/*
 * Function name: stat_object
 * Description:   This function is called when a wizard wants to get more 
 *                information about an object.
 * Returns:       str - The string to write..
 */
string
stat_object()
{
    string str, tstr;
    mixed tmp;

    str = "Plik: " + file_name(this_object()) + ", Tworca: " + 
	creator(this_object()) + ", Uid: " + getuid(this_object()) + 
	", Euid: " + geteuid(this_object()) + "\n";
    if (tstr = query_prop(OBJ_S_WIZINFO))
	str += "Dodatkowe informacje:\n\t" + tstr;
    str += "Nazwa: " + query_name() + " \t";
    str += "Krotki opis: " + short() + "\n";
    str += "Dlugi opis: " + long();

    tstr = "";
    if (tmp = query_prop(OBJ_I_WEIGHT))
	tstr += "Waga obiektu: " + tmp + " \t";
    if (tmp = query_prop(OBJ_I_VOLUME))
	tstr += "Objetosc obiektu: " + tmp + " \t";
    if (tmp = query_prop(OBJ_I_VALUE))
	tstr += "Wartosc obiektu: " + tmp + "";
    if (strlen(tstr))
	str += tstr + "\n";

    tstr = "";
    if (tmp = query_prop(OBJ_I_HIDE))
	tstr += "ukryty\t";
    if (tmp = query_prop(OBJ_I_INVIS))
	tstr += "niewidzialny\t";
    if (tmp = query_prop(MAGIC_AM_MAGIC))
	tstr += "magiczny\t";
    if (query_no_show())
	tstr += "no_show\t";
    if (!query_no_show() && query_no_show_composite())
	tstr += "no_show_composite\t";
    if ((this_object()->query_recover()) &&
	(!(this_object()->may_not_recover())))
	tstr += "recoverable";
    if (strlen(tstr))
	str += tstr + "\n";

    return str;
}

/*
 * Function name: query_alarms
 * Description:   This function gives all alarms set in this object.
 * Returns:       The list as given by get_all_alarms.
 */
mixed
query_alarms()
{
    return get_all_alarms();
}

/*
 * Function name: init
 * Description  : Add the 'command items' of this object. Note that if
 *		  you redefine this function, the command items will not
 *		  work unless you do ::init(); in your code!
 */
public void
init()
{
    int index = sizeof(obj_commands);

    while(--index >= 0)
    {
	add_action(cmditem_action, obj_commands[index]);
    }
}


/*
 * Nazwa funkcji : 
 * Opis          : 
 * Argumenty     : 
 * Funkcja zwraca: 
 */
