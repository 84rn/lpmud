/*
 *  /cmd/std/soul_cmd.c
 *
 *  spolszczenie - Silvathraec 1997
 *
 *  Tego podstawowego soula dziedzicza wszystkie soule rasowe. Zawiera on
 *  emoty przeznaczone dla wszystkich graczy. Moze rowniez sluzyc jako
 *  przyklad soula gildiowego. Jesli zamierzasz robic w nim jakies zmiany lub
 *  rozszerzenia, trzymaj sie prosze pewnych standardow:
 *
 *  - Komendy emotow powinny byc czasownikami w formie rozkazujacej, wraz ze
 *  swoimi argumentami tworzacymi poprawne, jak najbardziej naturalne i
 *  sensowne zdanie. W pewnych przypadkach dopuszczalne jest uzywanie skrotow
 *  w formie rzeczownikow, bez zmiany skladni argumentow.
 *  - Warto zwrocic uwage na wlasciwe, w zaleznosci od kontekstu, uzywanie
 *  funkcji all/allbb, target/targetbb, all2act/all2actbb - nieuzasadnione
 *  ujawnianie obecnosci ukrytych/niewidzialnych osob nie jest pozadane.
 *  - Makra SOULDESC() nalezy uzywac wylacznie w wypadku dzialan, ktorych
 *  skutki trwaja przez pewien czas, takich jak usmiech (w przeciwienstwie
 *  do, na przyklad, uklonu).
 *  - Nie wszystkie emoty powinny moc dac sie wykonac na wiecej niz jednym
 *  celu jednoczesnie.
 *  - Nalezy sprawdzac takie rzeczy jak LIVE_M_MOUTH_BLOCKED, OPT_ECHO, albo
 *  (wazne!) czy gracz jest kobieta.
 *  - Kazda funkcja powinna generowac poprawne komunikaty bledow.
 *
 *  Uwaga:
 *  Skutki podejmowanych przez graczy dzialan MUSZA byc wyswietlane w
 *  nastepujacym porzadku: wykonawca, widzowie, cel. Dzieki temu, jesli cel
 *  zareaguje natychmiast, gracze otrzymaja teksty we wlasciwej kolejnosci.
 */

#pragma no_clone
#pragma strict_types
#pragma save_binary

inherit "/cmd/std/command_driver";

#include <adverbs.h>
#include <cmdparse.h>
#include <composite.h>
#include <filter_funs.h>
#include <macros.h>
// #include <ss_types.h>
#include <stdproperties.h>
#include <options.h>

#define SUBLOC_SOULEXTRA    "_soul_cmd_extra"
#define SOULDESC(x)         (this_player()->add_prop(LIVE_S_SOULEXTRA, (x)))
#define LANGUAGE_ALL_RSAY   55  /* Poziom, od ktorego rozumie sie wszystko. */
#define LANGUAGE_MIN_RSAY   15  /* Poziom, do ktorego nie rozumie sie nic. */
#define SHOUT_DEPTH         1   /* Przez ile pokoi niesie sie krzyk. */
#define MAX_FREE_EMOTE      60  /* Maksymalna dlugosc wolnego emota. */
#define DUMP_EMOTIONS_OUT   "/open/dump_emotions"

#define CHECK_MOUTH_BLOCKED; \
            if (Prop = this_player()->query_prop(LIVE_M_MOUTH_BLOCKED)) { \
            notify_fail(stringp(Prop) ? Prop : \
                "Nie jestes w stanie wydobyc z siebie zadnego dzwieku.\n"); \
            return 0; }

#define CHECK_MULTI_TARGETS(str, przyp) \
            if (sizeof(cele) > 1) { notify_fail("Nie mozesz " + str + " " + \
                COMPOSITE_LIVE(cele, przyp) + ".\n"); return 0; }

#define IS_BALD(living) \
            (member_array("lysy", living->query_przym(1, PL_MIA, 1)) != -1)

#define BUG_FAIL(str) \
            (log_file("bugi", ctime(time()) + ": [" + \
                file_name(this_object()) + "] " + str + "\n"), \
            "\nBLAD !!! Czarodzieje beda ci wdzieczni za zgloszenie go, " + \
                this_player()->query_wolacz() + ".\n\n")

private mixed Prop;             /* Uzywana przez CHECK_MOUTH_BLOCKED. */

private string *Kierunki = ({"polnoc", "poludnie", "wschod", "zachod",
                             "polnocny wschod", "poludniowy wschod",
                             "poludniowy zachod", "polnocny zachod",
                             "gore", "dol"});

private string *Skroty = ({"n", "s", "e", "w", "ne", "se", "sw", "nw", "u",
                           "d"});   /* Uzywane przez 'popatrz', 'wskaz'. */

/*
 *  Zwraca nazwe soula.
 */
public string
get_soul_id()
{
    return "emotions";
}

/*
 *  Lista komend. Prosze dodawac nowe w kolejnosci alfabetycznej.
 */
public mapping
query_cmdlist()
{
    return ([
             "beknij":"beknij",
             "blagaj":"blagaj",

             "chrzaknij":"chrzaknij",
             "czknij":"czknij",

             "dygnij":"dygnij",

             "hmm":"hmm",

             "jeknij":"jeknij",
             "jezyk":"jezyk",

             "kichnij":"kichnij",
             "kiwnij":"skin",			/* alias */
             "kopnij":"kopnij",
             "krzyknij":"krzyknij",

             "machnij":"machnij",
             "mrugnij":"mrugnij",

             "nadepnij":"nadepnij",
             "namysl":"namysl",

             "ob":"obejrzyj",			/* skrot */
             "obejrzyj":"obejrzyj",
             "obliz":"obliz",
             "odetchnij":"odetchnij",
             "opluj":"opluj",
             "otrzasnij":"otrzasnij",

             "parsknij":"parsknij",
             "pocaluj":"pocaluj",
             "pierdnij":"pierdnij",
             "pociesz":"pociesz",
             "podaj":"podaj",
             "podlub":"podlub",
             "podrap":"podrap",
             "podrepcz":"podrepcz",
             "podskocz":"podskocz",
             "podziekuj":"podziekuj",
             "pogladz":"poglaszcz",		/* alias */
             "poglaszcz":"poglaszcz",
             "pogratuluj":"pogratuluj",
             "pokiwaj":"pokiwaj",
             "poklon":"uklon",			/* alias */
             "pokrec":"pokrec",
          /* "polaskocz":"polaskocz", */
             "pomachaj":"pomachaj",
             "popatrz":"popatrz",
             "popukaj":"popukaj",
             "potrzasnij":"potrzasnij",
             "potrzyj":"potrzyj",
             "potwierdz":"potwierdz",
             "powitaj":"powitaj",
             "pozegnaj":"pozegnaj",
             "prychnij":"prychnij",
             "przebacz":"wybacz",		/* alias */
             "przebieraj":"przebieraj",
             "przeciagnij":"przeciagnij",
             "przelknij":"przelknij",
             "przepros":"przepros",
             "przestap":"przestap",
             "przewroc":"przewroc",
             "przymruz":"zmruz",		/* alias */
             "przytul":"przytul",
             "przywitaj":"powitaj",		/* alias */
          /* "pusc":"pusc", */              /* Do usuniecia. */

             "rozejrzyj":"rozejrzyj",
             "rozplacz":"rozplacz",

             "sapnij":"sapnij",
             "skin":"skin",
             "skrzyw":"skrzyw",
             "spanikuj":"spanikuj",
             "splun":"splun",
             "spoliczkuj":"spoliczkuj",
             "spojrzyj":"popatrz",		/* alias */
             "szepnij":"szepnij",
             "szturchnij":"szturchnij",

             "tupnij":"tupnij",

             "uklon":"uklon",
             "usciskaj":"usciskaj",
             "usmiech":"usmiechnij",		/* skrot */
             "usmiechnij":"usmiechnij",
          /* "uszczypnij":"uszczypnij", */

             "warknij":"warknij",
             "westchnij":"westchnij",
             "wskaz":"wskaz",
             "wybacz":"wybacz",
             "wybalusz":"wytrzeszcz",		/* alias */
             "wykrzyw":"skrzyw",		/* alias */
             "wyplacz":"wyplacz",
             "wytrzeszcz":"wytrzeszcz",
             "wzdrygnij":"wzdrygnij",
             "wzrusz":"wzrusz",

             "zachichocz":"zachichocz",
             "zachlip":"zachlip",
             "zachrumkaj":"zachrumkaj",
             "zaczerwien":"zaczerwien",
             "zajecz":"jeknij",			/* alias */
             "zagryz":"zagryz",
             "zagwizdz":"zagwizdz",
             "zakrztus":"zakrztus",
             "zalam":"zalam",
             "zalkaj":"zalkaj",
             "zamrucz":"zamrucz",
             "zaprzecz":"zaprzecz",
             "zarechocz":"zarechocz",
             "zarumien":"zaczerwien",
             "zasmiej":"zasmiej",
             "zatancz":"zatancz",
             "zatrzyj":"zatrzyj",
             "zazgrzytaj":"zazgrzytaj",
             "zbeltaj":"zbeltaj",
             "zblednij":"zblednij",
             "zdziw":"zdziw",
             "ziewnij":"ziewnij",
             "zignoruj":"zignoruj",
             "zmarszcz":"zmarszcz",
             "zmruz":"zmruz",
             "zwymiotuj":"zbeltaj"		/* alias */
           ]);
}

/*
 * Function name: using_soul
 * Description  : Called once by the living object using this soul. Adds
 *                sublocations responsible for extra descriptions of the
 *                living object.
 * Arguments    : object live - the living using the soul.
 */
public void
using_soul(object live)
{
    live->add_subloc(SUBLOC_SOULEXTRA, file_name(this_object()));
}

/*
 * Function name: show_subloc
 * Description  : Shows the specific sublocation description for a living.
 * Arguments    : string subloc  - the subloc to display.
 *                object on      - the object to which the subloc is linked.
 *                object for_obj - the object that wants to see the subloc.
 * Returns      : string - the subloc description.
 */
public string
show_subloc(string subloc, object on_obj, object for_obj)
{
    if ((subloc != SUBLOC_SOULEXTRA) ||
        on_obj->query_prop(TEMP_SUBLOC_SHOW_ONLY_THINGS) ||
	!(subloc = on_obj->query_prop(LIVE_S_SOULEXTRA)))

	return "";

    if (for_obj == on_obj)
    {
        string *wyrazy = explode(subloc, " ");
        string pierwszy = wyrazy[0];

        switch (pierwszy)
        {
            case "jest":
                wyrazy[0] = "jestes";
                break;
            case "jestes,":
                wyrazy[0] = "jestes,";
                break;
            default:
                if (pierwszy[-1..] == ",")
                    wyrazy[0] = pierwszy[..-2] + "sz,";
                else
                    wyrazy[0] = pierwszy + "sz";
        }
        subloc = implode(wyrazy, " ");
    }
    return capitalize(subloc + ".\n");
}

/*
 * Function name: query_cmd_soul
 * Description  : This is a command soul. This defines it as such.
 * Returns      : int 1 - always.
 */
public int
query_cmd_soul()
{
    return 1;
}

/*
 * Function name: dump_emotions
 * Description  : This function can be used to dump all emotions to a file
 *                in a sorted and formatted way. This dump can be used for
 *                the 'help emotions' document. The output of this function
 *                will be written to the file DUMP_EMOTIONS_OUT.
 * Returns      : int 1 - always.
 */
public nomask int
dump_emotions()
{
    int index = -1;
    int size = strlen(ALPHABET);
    string *words;

    setuid();
    seteuid(getuid());

    rm(DUMP_EMOTIONS_OUT);

    while(++index < size)
    {
        words = filter(m_indices(query_cmdlist()),
                       &wildmatch((ALPHABET[index..index] + "*")));

        if (!sizeof(words))
            continue;

        if (strlen(words[0]) < 14)
            words[0] = (words[0] + "                ")[..13];

        write_file(DUMP_EMOTIONS_OUT,
            sprintf("%-80#s\n\n", implode(sort_array(words), "\n")));
    }
    return 1;
}

/*
 * Function name: find_neighbour
 * Description  : This function will recursively search through the
 *                neighbouring rooms to a particular room to find the
 *                rooms a shout or scream will be heard in.
 * Arguments    : object *found  - the rooms already found.
 *                object *search - the rooms still to search.
 *                int    depth   - the depth still to search.
 * Returns      : object * - the neighbouring rooms.
 */
static object *
find_neighbour(object *found, object *search, int depth)
{
    int index;
    int size;
    int index2;
    int size2;
    mixed *exit_arr;
    object *new_search, *rooms, troom;

    if (!depth)
        return found;

    rooms = found;
    new_search = ({});

    index = -1;
    size = sizeof(search);
    while(++index < size)
    {
        exit_arr = (mixed *) search[index]->query_exit();

        index2 = -3;
        size2 = sizeof(exit_arr);
        while((index2 += 3) < size2)
        {
            if (objectp(exit_arr[index2]))
                troom = exit_arr[index2];
            else
                troom = find_object(exit_arr[index2]);
            if (objectp(troom) &&
                (member_array(troom, rooms) < 0))
            {
                rooms += ({ troom });
                new_search += ({ troom });
            }
        }
    }

    return find_neighbour(rooms, new_search, depth - 1);
}

string
shout_name()
{
    /* Odbiorca. Zauwazmy, ze previous_object() to VBFC_OBJECT. */
    object pobj = previous_object(-1);

    if (environment(pobj) == environment(this_player()) &&
        CAN_SEE_IN_ROOM(pobj) && CAN_SEE(pobj, this_player()))
        return this_player()->query_Imie(pobj, PL_MIA);
    if (pobj->query_met(this_player()))
        return this_player()->query_name(PL_MIA);
    return this_player()->query_osobno() ?
           this_player()->koncowka("Jakis mezczyzna", "Jakas kobieta") :
           this_player()->koncowka("Jakis ", "Jakas ", "Jakies ")
         + this_player()->query_rasa(PL_MIA);
}

string
shout_string(string shout_what)
{
    /* Odbiorca. Zauwazmy, ze previous_object() to VBFC_OBJECT. */
    object pobj = previous_object(-1);
    int empty_string = shout_what == "0";

    if (environment(pobj) == environment(this_player()))
        return empty_string ? "krzyczy glosno." : "krzyczy: " + shout_what;

    return empty_string ? "krzyczy glosno z oddali." :
                          "krzyczy z oddali: " + shout_what;
}

/*
 *  Funkcje obslugujace emoty. Prosze dodawac nowe w kolejnosci alfabetycznej.
 */

int
beknij(string str)
{
    string jak;

    CHECK_MOUTH_BLOCKED;

    jak = check_adverb_with_space(str, "prostacko");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        write("Bekasz" + jak + ".\n");
        all("beka" + jak + ".", jak);
        return 1;
    }
    notify_fail("Beknij [jak] ?\n");
    return 0;
}

int
blagaj(string str)
{
    object *cele;

    CHECK_MOUTH_BLOCKED;

    cele = parse_this(str, "%l:" + PL_BIE + " 'o' 'wybaczenie' / "
                    + "'przebaczenie'");
    if (sizeof(cele))
    {
        actor("Blagasz", cele, PL_BIE, " o wybaczenie.");
        all2actbb("blaga", cele, PL_BIE, " o wybaczenie.");
        target("blaga cie o wybaczenie.", cele);
        return 1;
    }
    cele = parse_this(str, "%l:" + PL_BIE + " [o] [litosc] / [laske]");
    if (sizeof(cele))
    {
        actor("Padasz na kolana, blagajac", cele, PL_BIE, " o litosc.");
        all2actbb("pada na kolana, blagajac", cele, PL_BIE, " o litosc.");
        target("pada na kolana, blagajac cie o litosc.", cele);
        return 1;
    }
    notify_fail("Blagaj kogo [o co] ?\n");
    return 0;
}

int
chrzaknij(string str)
{
    string jak;

    CHECK_MOUTH_BLOCKED;

    jak = check_adverb_with_space(str, "znaczaco");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        write("Chrzakasz" + jak + ".\n");
        all("chrzaka" + jak + ".", jak);
        return 1;
    }
    notify_fail("Chrzaknij [jak] ?\n");
    return 0;
}

int
czknij(string str)
{
    string jak;

    CHECK_MOUTH_BLOCKED;

    jak = check_adverb_with_space(str, "prostacko");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        SOULDESC("ma czkawke");
        write("Czkasz" + jak + ".\n");
        all("czka" + jak + ".", jak);
        return 1;
    }
    notify_fail("Czknij [jak] ?\n");
    return 0;
}

int
dygnij(string str)
{
    object *cele;
    string *jak;

    /* Zabezpieczenie przed bledna interpretacja 'przed' jako przyslowka. */
    if (str && wildmatch("przed *", str))
        jak = ({str, ADD_SPACE_TO_ADVERB("z gracja")});
    else
        jak = parse_adverb_with_space(str, "z gracja", 0);

    if (this_player()->query_gender() != G_FEMALE)
        jak[1] = ADD_SPACE_TO_ADVERB("niezdarnie");

    if (!jak[0])
    {
        write("Dygasz" + jak[1] + ".\n");
        allbb("dyga" + jak[1] + ".", jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'przed' %l:" + PL_NAR);
    if (sizeof(cele))
    {
        actor("Dygasz przed", cele, PL_NAR, jak[1] + ".");
        all2actbb("dyga przed", cele, PL_NAR, jak[1] + ".", jak[1]);
        targetbb("dyga przed toba" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Dygnij [jak] [przed kim] ?\n");
    return 0;
}

int
hmm(string str)
{
    CHECK_MOUTH_BLOCKED;

    if (!str)
    {
        write("Hmmmmm...\n");
        allbb("wydaje z siebie dlugie hmmmmm...");
        return 1;
    }
    notify_fail("Hmm?\n");
    return 0;
}

int
jeknij(string str)
{
    string jak;

    CHECK_MOUTH_BLOCKED;

    jak = check_adverb_with_space(str, "bolesnie");

    if (jak != NO_ADVERB_WITH_SPACE)
    {
        write("Jeczysz" + jak + ".\n");
        all("jeczy" + jak + ".", jak);
        return 1;
    }

    notify_fail(capitalize(query_verb()) + " [jak] ?\n");
    return 0;
}

int
jezyk(string str)
{
    object *cele;

    if (!str)
    {
        write("Pokazujesz jezyk.\n");
        allbb("pokazuje jezyk.");
        return 1;
    }
    cele = parse_this(str, "%l:" + PL_CEL);
    if (sizeof(cele))
    {
        actor("Pokazujesz", cele, PL_CEL, " jezyk.");
        all2actbb("pokazuje", cele, PL_CEL, " jezyk.");
        target("pokazuje ci jezyk.", cele);
        return 1;
    }
    notify_fail("(Pokaz) jezyk [komu] ?\n");
    return 0;
}

int
kichnij(string str)
{
    string jak;

    jak = check_adverb_with_space(str, "glosno");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        write("Na wpol przymykasz oczy, odchylasz glowe i kichasz" + jak
            + ".\n");
        all("na wpol przymyka oczy, odchyla glowe i kicha" + jak + ".");
        return 1;
    }
    notify_fail("Kichnij [jak] ?\n");
    return 0;
}

int
kopnij(string str)
{
    object *cele;

    cele = parse_this(str, "%l:" + PL_BIE);
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("kopac jednoczesnie", PL_DOP);

        actor("Kopiesz brutalnie", cele, PL_BIE);
        all2actbb("kopie brutalnie", cele, PL_BIE);
        target("kopie cie brutalnie.", cele);
        return 1;
    }
/*
    cele = parse_this(str, "%l:" + PL_BIE + " 'w' 'jaja'");
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("kopac jednoczesnie", PL_DOP);

        actor("Kopiesz brutalnie", cele, PL_BIE);
        all2actbb("kopie brutalnie", cele, PL_BIE);
        target("kopie cie brutalnie.", cele);
        return 1;
    }
*/
    cele = parse_this(str, "%l:" + PL_BIE + " 'w' 'kostke'");
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("kopac jednoczesnie", PL_DOP);

        actor("Kopiesz", cele, PL_BIE, " po kostkach.");
        all2actbb("kopie", cele, PL_BIE, " po kostkach.");
        target("kopie cie po kostkach.", cele);
        cele->add_prop(LIVE_S_SOULEXTRA,
                       "podskakuje, rozcierajac obolale kostki");
        return 1;
    }
    notify_fail("Kopnij kogo [gdzie] ?\n");
    return 0;
}

int
krzyknij(string str)
{
    object env;
    object *lokacje;
    int ile;

    CHECK_MOUTH_BLOCKED;

    if (env = environment(this_player()))
        lokacje = find_neighbour(({env}), ({env}), SHOUT_DEPTH);
    else
        lokacje = ({});

    if (this_player()->query_option(OPT_ECHO))
        write("Krzyczysz" + (str ? ": " + str : " glosno.") + "\n");
    else
        write("Ok.\n");

    ile = sizeof(lokacje);
    while(ile--)
        tell_room(lokacje[ile], "@@shout_name:" + file_name(this_object())
                + "@@ @@shout_string:" + file_name(this_object()) + "|" + str
                + "@@\n", ({this_player()}));

    return 1;
}

int
machnij(string str)
{
    object *cele;
    string *jak;

    if (str == "reka")
        str = 0;
    else if (str)
        sscanf(str, "reka %s", str);

    jak = parse_adverb_with_space(str, "z rezygnacja", 0);
    if (!jak[0])
    {
        write("Machasz reka" + jak[1] + ".\n");
        allbb("macha reka" + jak[1] + ".", jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'na' %l:" + PL_DOP);
    if (sizeof(cele))
    {
        actor("Machasz na", cele, PL_DOP, jak[1] + ".");
        all2actbb("macha na", cele, PL_DOP, jak[1] + ".", jak[1]);
        targetbb("macha na ciebie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Machnij [reka] [jak] [na kogo] ?\n");
    return 0;
}

int
mrugnij(string str)
{
    object *cele;
    string *jak;

    jak = parse_adverb_with_space(str, "porozumiewawczo", 0);
    if (!jak[0])
    {
        write("Mrugasz" + jak[1] + ".\n");
        allbb("mruga" + jak[1] + ".", jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'do' %l:" + PL_DOP);
    if (sizeof(cele))
    {
        actor("Mrugasz do", cele, PL_DOP, jak[1] + ".");
        all2actbb("mruga do", cele, PL_DOP, jak[1] + ".", jak[1]);
        targetbb("mruga do ciebie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Mrugnij [jak] [do kogo] ?\n");
    return 0;
}

int
nadepnij(string str)
{
    object *cele;

    if (sizeof(cele = parse_this(str, "'na' %l:" + PL_BIE)))
    {
        CHECK_MULTI_TARGETS("deptac jednoczesnie po", PL_MIE);

        actor("Z msciwa satysfakcja nadeptujesz", cele, PL_CEL, " na stope.");
        all2actbb("nadeptuje", cele, PL_CEL, " na stope.");
        target("nadeptuje ci na stope. Auhuhu... ! Co za bol...", cele);
        cele->add_prop(LIVE_S_SOULEXTRA,
                       "podskakuje, rozcierajac obolala stope");
        return 1;
    }
    notify_fail("Nadepnij na kogo?\n");
    return 0;
}

int
namysl(string str)
{
    if (str == "sie" || (str && wildmatch("sie nad *", str)))
    {
        if (strlen(str) > MAX_FREE_EMOTE)
            str = "sie nad czyms niezwykle skomplikowanym";

        SOULDESC("namysla sie nad czyms");
        write("Namyslasz " + str + ". Tak... \n");
        allbb("namysla " + str + ".");
        return 1;
    }

    notify_fail("Namysl sie [nad czym] ?\n");
    return 0;
}

int
obejrzyj(string str)
{
    if (str == "sie" || str == "sie za siebie")
    {
        SOULDESC("lypie nerwowo za siebie");
        write("Ogladasz sie nerwowo za siebie i...\n"
            + "Na szczescie nikt za toba nie stal.\n");
        allbb("oglada sie nerwowo za siebie.");
        return 1;
    }
    /* Notify_fail jest zdefiniowany w /cmd/live/things. */
    return 0;
}

int
obliz(string str)
{
    string *jak;

    jak = parse_adverb_with_space(str, "ze smakiem", 1);
    if (jak[0] == "sie")
    {
        SOULDESC("oblizuje sie" + jak[1]);
        write("Oblizujesz sie" + jak[1] + ".\n");
        allbb("oblizuje sie" + jak[1] + ".", jak[1]);
        return 1;
    }
    notify_fail("Obliz sie [jak] ?\n");
    return 0;
}

int
odetchnij(string str)
{
    string jak;

    CHECK_MOUTH_BLOCKED;

    jak = check_adverb_with_space(str, "pelna piersia");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        write("Oddychasz" + jak + ".\n");
        all("oddycha" + jak + ".", jak);
        return 1;
    }
    notify_fail("Odetchnij [jak] ?\n");
    return 0;
}

int
opluj(string str)
{
    object *cele;

    cele = parse_this(str, "%l:" + PL_BIE);
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("pluc jednoczesnie na", PL_BIE);

        actor("Opluwasz", cele, PL_BIE);
        all2actbb("opluwa", cele, PL_BIE, ".");
        target("opluwa cie.", cele);
        return 1;
    }
    notify_fail("Opluj kogo?\n");
    return 0;
}

int
otrzasnij(string str)
{
    if (str == "sie" || (str && wildmatch("sie *mysl*", str)))
    {
        if (strlen(str) > MAX_FREE_EMOTE)
            str = "sie na mysl o czyms szczegolnie potwornym";

        write("Otrzasasz " + str + ". Brrr... !\n");
        allbb("otrzasa " + str + ".");
        return 1;
    }

    notify_fail("Otrzasnij sie [na mysl o czym] ?\n");
    return 0;
}

int
parsknij(string str)
{
    object *cele;
    string *jak;

    jak = parse_adverb_with_space(str, "kpiaco", 0);
    if (!jak[0])
    {
        write("Parskasz" + jak[1] + ".\n");
        all("parska" + jak[1] + ".", jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'na' %l:" + PL_BIE);
    if (sizeof(cele))
    {
        actor("Parskasz na", cele, PL_BIE, jak[1] + ".");
        all2act("parska na", cele, PL_BIE, jak[1] + ".", jak[1]);
        target("parska na ciebie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Parsknij [jak] [na kogo] ?\n");
    return 0;
}

int
pierdnij(string str)
{
    if (!str)
    {
        write("Koncentrujesz sie przez chwile... Achh ! Co za ulga...\n");
        allbb("zamyka oczy i koncentruje sie.");
        tell_room(environment(this_player()), "Czujesz nagle potworny wrecz "
                + "smrod. Chyba ktos puscil tu baka...\n");
        return 1;
    }
    notify_fail("Pierdnij?\n");
    return 0;
}

int
pocaluj(string str)
{
    string *wyrazy = str ? explode(str, " ") : ({});
    int ile = sizeof(wyrazy);
    object *cele;
    string *jak;

// w reke, policzek, usta

/*    if (ile > 2 && wyrazy[ile - 2] == "w")
        switch (wyrazy[ile - 1])
        {
            case "czolo":
            case "czolko":
                jak = parse_adverb_with_space(str, "czule", 1);
                cele = parse_this(jak[0], "%l:" + PL_BIE);
                if (sizeof(cele))
                {
                   actor("Calujesz" + jak[1], cele, PL_BIE, " w czolko.");
                       all2actbb("caluje" + jak[1], cele, PL_BIE,
                                 " w czolko.", jak[1]);
                       target("caluje cie" + jak[1] + " w czolko.", cele,
                              jak[1]);
                       return 1;
        }
    }
*/    jak = parse_adverb_with_space(str, "goraco", 1);
    cele = parse_this(jak[0], "%l:" + PL_BIE);
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("calowac jednoczesnie", PL_DOP);

        actor("Calujesz" + jak[1], cele, PL_BIE);
        all2actbb("caluje" + jak[1], cele, PL_BIE, ".", jak[1]);
        target("caluje cie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Pocaluj kogo [jak] [gdzie] ?\n");
    return 0;
}

int
pociesz(string str)
{
    object *cele;

    CHECK_MOUTH_BLOCKED;

    cele = parse_this(str, "%l:" + PL_BIE);
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("pocieszac jednoczesnie", PL_DOP);

#if 0	/* Nie skonczone jeszcze */
        if (this_player()->query_gender() == G_FEMALE ||
            cele[0]->query_gender() == G_FEMALE)
        {
            actor("Przytulasz", cele, PL_BIE, " pocieszajac "
                + cele[0]->koncowka("go", "ja") + " w strapieniu.\n");
        }
#endif 0
        actor("Pocieszasz", cele, PL_BIE, " na miare swych skromnych "
            + "mozliwosci.");
        all2actbb("pociesza", cele, PL_BIE, " na miare swych skromnych "
                + "mozliwosci.");
        target("pociesza cie na miare swych skromnych mozliwosci.", cele);
        return 1;
    }
    notify_fail("Pociesz kogo?\n");
    return 0;
}

int
podaj(string str)
{
    object *cele;

    cele = parse_this(str, "'reke' / 'dlon' %l:" + PL_CEL);
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("wymieniac jednoczesnie uscisku dloni z", PL_NAR);

        actor("Wymieniasz uscisk dloni z", cele, PL_NAR);
        all2actbb("wymienia uscisk dloni z", cele, PL_NAR);
        target("wymienia z toba uscisk dloni.", cele);
        return 1;
    }
    notify_fail("Podaj reke komu?\n");
    return 0;
}

int
podlub(string str)
{
    if (!str)
        str = "w nosie";

    switch (str)
    {
        case "w nosie":
            SOULDESC("dlubie w nosie");
            write("Dlubiesz w nosie.\n");
            allbb("dlubie w nosie.");
            return 1;
        case "w uchu":
            SOULDESC("dlubie sobie w uchu");
            write("Dlubiesz sobie w uchu.\n");
            allbb("dlubie sobie w uchu.");
            return 1;
    }
    notify_fail("Podlub [w czym] ?\n");
    return 0;
}

int
podrap(string str)
{
    switch (str)
    {
        case "sie":
        case "sie po glowie":
        case "sie w glowe":
            write("Drapiesz sie po glowie.\n");
            allbb("drapie sie po glowie.");
            return 1;
        case "sie po nosie":
        case "sie w nos":
            write("Drapiesz sie w nos.\n");
            allbb("drapie sie w nos.");
            return 1;
        case "sie w ucho":
            write("Drapiesz sie w ucho.\n");
            allbb("drapie sie w ucho.");
            return 1;
        case "sie po brzuchu":
        case "sie w brzuch":
            write("Drapiesz sie po brzuchu.\n");
            allbb("drapie sie po brzuchu.");
            return 1;
        case "sie po zadku":
        case "sie w zadek":
        case "sie po tylku":
        case "sie w tylek":
        case "sie po posladku":
        case "sie w posladek":
        case "sie po posladkach":
        case "sie w posladki":
        case "sie po dupie":
        case "sie w dupe":
            write("Drapiesz sie w zadek.\n");
            allbb("drapie sie w zadek.");
            return 1;
        case "sie po jajach":
        case "sie w jaja":
            if (this_player()->query_gender() == G_MALE)
            {
                write("Drapiesz sie po jajach.\n");
                allbb("przysiada lekko, rozkracza sie i drapie pomiedzy "
                    + "nogami, nie baczac na twoja obecnosc.");
                return 1;
            }
            write("Ech, chcialo by sie... Ale ich nie masz!\n");
            return 1;
        case "sie po plecach":
        case "sie w plecy":
            switch (this_player()->query_race())
            {
                case "czlowiek":
                case "elf":
                    write("Drapiesz sie po plecach.\n");
                    allbb("drapie sie po plecach.");
                    return 1;
                case "krasnolud":
                case "halfling":
                case "gnom":
                case "ogr":
                    write("Usilujesz podrapac sie po plecach, ale masz chyba "
                        + "za krotkie rece.\n");
                    allbb("usiluje podrapac sie po plecach, ale ma chyba za "
                        + "krotkie rece.");
                    return 1;
                default:
                    if (this_player()->query_race())
                    {
                        write(BUG_FAIL("query_race() = "
                                     + this_player()->query_race()));
                        return 1;
                    }
                    write("Drapiesz sie po plecach.\n");
                    allbb("drapie sie po plecach.");
                    return 1;
            }
    }
    notify_fail("Podrap sie [gdzie] ?\n");
    return 0;
}

int
podrepcz(string str)
{
    if (!str || str == "w miejscu")
    {
        write("Drepczesz sobie w miejscu.\n");
        allbb("drepcze sobie w miejscu.");
        return 1;
    }
    notify_fail("Podrepcz [w miejscu] ?\n");
    return 0;
}

int
podskocz(string str)
{
    string jak;

    jak = check_adverb_with_space(str, "wesolo");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        SOULDESC("podskakuje" + jak);
        write("Podskakujesz" + jak + ".\n");
        allbb("podskakuje" + jak + ".", jak);
        return 1;
    }
    notify_fail("Podskocz [jak] ?\n");
    return 0;
}

int
podziekuj(string str)
{
    object *cele;
    string *jak;

    CHECK_MOUTH_BLOCKED;

    jak = parse_adverb_with_space(str, "wylewnie", 1);
    cele = parse_this(jak[0], "%l:" + PL_CEL);
    if (sizeof(cele))
    {
        actor("Dziekujesz" + jak[1], cele, PL_CEL);
        all2actbb("dziekuje" + jak[1], cele, PL_CEL, ".", jak[1]);
        target("dziekuje ci" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Podziekuj komu [jak] ?\n");
    return 0;
}

int
poglaszcz(string str)
{
    object *cele;
    string *jak;
    string *words;
    string gdzie = "";

    if (!str)
    {
        notify_fail(capitalize(query_verb()) + " kogo [jak] [gdzie] ?\n");
        return 0;
    }

    words = explode(str, " ");
    if (sizeof(words) > 2)
        switch (implode(words[-2..], " "))
        {
            case "po twarzy":
            case "po policzku":
            case "po policzkach":
            case "w policzek":
                gdzie = " po policzku";
                str = implode(words[..-3], " ");
                break;
            case "po wlosach":
            case "po glowie":
            case "po czaszce":
            case "po lysinie":
                gdzie = 0;
                str = implode(words[..-3], " ");
                break;
            case "po plecach":
                gdzie = " po plecach";
                str = implode(words[..-3], " ");
                break;
            case "po ramieniu":
            case "po ramionach":
            case "w ramie":
                gdzie = " po ramieniu";
                str = implode(words[..-3], " ");
                break;
            case "po dloni":
            case "po dloniach":
            case "po rece":
            case "po rekach":
                gdzie = " po dloni";
                str = implode(words[..-3], " ");
                break;
        }

    jak = parse_adverb_with_space(str, NO_DEFAULT_ADVERB, 1);
    cele = parse_this(jak[0], "%l:" + PL_BIE);
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("glaskac jednoczesnie", PL_DOP);

        if ((cele[0]->query_humanoid()))
        {
            if (jak[1] == NO_DEFAULT_ADVERB_WITH_SPACE)
                jak[1] = ADD_SPACE_TO_ADVERB("delikatnie");
            if (!gdzie)
                gdzie = IS_BALD(cele[0]) ? " po lysej czaszce" :
                                           " po wlosach";
        }
        else
        {
            if (jak[1] == NO_DEFAULT_ADVERB_WITH_SPACE)
                jak[1] = ADD_SPACE_TO_ADVERB("powoli");
            gdzie = "";
        }
        actor("Glaszczesz" + jak[1], cele, PL_BIE, gdzie + ".");
        all2actbb("glaszcze" + jak[1], cele, PL_BIE, gdzie + ".", jak[1]);
        target("glaszcze cie" + jak[1] + gdzie + ".", cele, jak[1]);
        return 1;
    }
    notify_fail(capitalize(query_verb()) + " kogo [jak] [gdzie] ?\n");
    return 0;
}

int
pogratuluj(string str)
{
    object *cele;
    string *jak;

    CHECK_MOUTH_BLOCKED;

    jak = parse_adverb_with_space(str, "z uznaniem", 1);
    cele = parse_this(jak[0], "%l:" + PL_CEL);
    if (sizeof(cele))
    {
        actor("Gratulujesz" + jak[1], cele, PL_CEL);
        all2actbb("gratuluje" + jak[1], cele, PL_CEL, ".", jak[1]);
        target("gratuluje ci" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Pogratuluj komu [jak] ?\n");
    return 0;
}

int
pokiwaj(string str)
{
    object *cele;
    string *jak;

    if (str == "glowa")
        str = 0;
    else if (str)
        sscanf(str, "glowa %s", str);

    jak = parse_adverb_with_space(str, "ze zrozumieniem", 0);
    if (!jak[0])
    {
        SOULDESC("kiwa glowa" + jak[1]);
        write("Kiwasz glowa" + jak[1] + ".\n");
        allbb("kiwa glowa" + jak[1] + ".", jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'do' %l:" + PL_DOP);
    if (sizeof(cele))
    {
        SOULDESC("kiwa glowa" + jak[1]);
        actor("Kiwasz do", cele, PL_DOP, " glowa" + jak[1] + ".");
        all2actbb("kiwa do", cele, PL_DOP, " glowa" + jak[1] + ".", jak[1]);
        targetbb("kiwa do ciebie glowa" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Pokiwaj [glowa] [jak] [do kogo] ?\n");
    return 0;
}

int
pokrec(string str)
{
    if (str == "sie" || str == "sie w kolko")
    {
        write("Krecisz sie w kolko.\n");
        allbb("kreci sie w kolko.");
        return 1;
    }
    notify_fail("Pokrec sie [w kolko] ?\n");
    return 0;
}

int
polaskocz(string str)
{ /*
    object *cele;
    string *jak;
    string gdzie;
    int    i;
			Eee... nie zadziala. ('word', [word])
    cele = parse_this(str, "%l 'w stope' / 'w stopy' / 'pod stopami' / "
                    + "'w piete' / 'w piety' / 'pod pacha' / 'pod pachami' / "
                    + "'w szyje' / 'w podbrodek' / 'w policzek' / "
                    + "'w brzuch' / 'po brzuchu' / 'w bok' / 'po boku'");
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("laskotac jednoczesnie", PL_DOP);

        jak = explode(str, " ");
        switch(gdzie = jak[sizeof(jak) - 1])
        {
            case "szyje":
            case "podbrodek":
            case "policzek":
                cele->add_prop(LIVE_S_SOULEXTRA,
                               "usmiecha sie z rozbawieniem");
                if (sizeof(cele) == 1)
                    actor("Laskoczesz", cele, " w " + gdzie + ", a "
                        + query_zaimek(cele[0]) + "usmiecha sie z ((sizeof(cele) == 1) ?
                    (capitalize(oblist[0]->query_pronoun()) + " smiles") :
                    "They smile") + " sweetly in return.");
            all2act("tickles", oblist, " under the chin. " +
                ((sizeof(oblist) == 1) ? "The latter smiles" :
                    "They smile") + " sweetly in return.");
            return 1;

        case "feet":
        case "foot":
            oblist->add_prop(LIVE_S_SOULEXTRA, "laughing uncontrollably");
            target(" tickles you under your " + location + ". " +
                "You fall down, laughing uncontrollably.", oblist);
            actor("You tickle", oblist, " under the " + location + ". " +
                ((sizeof(oblist) == 1) ?
                    (capitalize(oblist[0]->query_pronoun()) + " falls") :
        	    "They fall") + " down, laughing uncontrollably.");
	    all2act("tickles", oblist, " under the " + location + ". " +
		((sizeof(oblist) == 1) ? "The latter falls" :
		    "They fall") + " down, laughing uncontrollably.");
	    return 1;

	case "abdomen":
	case "belly":
	case "side":
	    oblist->add_prop(LIVE_S_SOULEXTRA, "giggling merrily");
	    target(" tickles you in your " + location + ". " +
		"You start giggling merrily.", oblist);
	    actor("You tickle", oblist, " in the " + location + ". " +
		((sizeof(oblist) == 1) ?
		    (capitalize(oblist[0]->query_pronoun()) + " starts") :
		    "They start") + " giggling merrily.");
	    all2act("tickles", oblist, " in the " + location + ". " +
		((sizeof(oblist) == 1) ? "The latter starts" :
		    "They start") + " giggling merrily.");
	    return 1;

	default:
	    notify_fail("Tickle whom [where / how]? Rather... this should " +
		"not happen. Please make a sysbugreport about this.\n");
	    return 0;
        }

        return 1;
    }

    how = parse_adverb_with_space(str, "playfully", 1);

    oblist = parse_this(how[0], "[the] %l");

    if (!sizeof(oblist))
    {
        notify_fail("Tickle whom [how / where]?\n");
        return 0;
    }

    oblist->add_prop(LIVE_S_SOULEXTRA, "laughing");
    actor("You tickle", oblist, how[1] + ". " +
        ((sizeof(oblist) == 1) ?
	    (capitalize(oblist[0]->query_pronoun()) + " falls") :
	    "They fall") + " down laughing, rolling over in an attempt to " +
	    "evade your tickling fingers.");
    all2act("tickles", oblist, how[1] + ". " +
	((sizeof(oblist) == 1) ? "The latter falls" :
	    "They fall") + " down laughing, rolling over in an attempt to " +
	    "evade the tickling fingers.", how[1]);
    target(" tickles you" + how[1] + ". You fall down laughing and roll " +
	"over in an attempt to evade those tickling fingers.", oblist,
	how[1]);

    return 1; */
}

int
pomachaj(string str)
{
    string czym = " rekami.";
    object *cele;
    string *jak;

    if (str)
    {
        string *words = explode(str, " ");

        switch (words[0])
        {
            case "reka":
            case "dlonia":
                czym = " reka.";
            case "rekami":
            case "dlonmi":
                str = (sizeof(words) == 1 ? 0 : implode(words[1..], " "));
        }
    }

    jak = parse_adverb_with_space(str, NO_DEFAULT_ADVERB, 0);
    if (!jak[0])
    {
        if (jak[1] == NO_DEFAULT_ADVERB_WITH_SPACE)
            jak[1] = ADD_SPACE_TO_ADVERB("wesolo");
        write("Machasz" + jak[1] + czym + "\n");
        allbb("macha" + jak[1] + czym, jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'do' %l:" + PL_DOP);
    if (sizeof(cele))
    {
        if (jak[1] == NO_DEFAULT_ADVERB_WITH_SPACE)
            jak[1] = ADD_SPACE_TO_ADVERB("wesolo");
        actor("Machasz do", cele, PL_DOP, jak[1] + ".");
        all2actbb("macha do", cele, PL_DOP, jak[1] + ".", jak[1]);
        targetbb("macha do ciebie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'na' %l:" + PL_DOP);
    if (sizeof(cele))
    {
        if (jak[1] == NO_DEFAULT_ADVERB_WITH_SPACE)
            jak[1] = ADD_SPACE_TO_ADVERB("zachecajaco");
        actor("Machasz na", cele, PL_DOP, jak[1] + ".");
        all2actbb("macha na", cele, PL_DOP, jak[1] + ".", jak[1]);
        targetbb("macha na ciebie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Pomachaj [czym] [jak] [do kogo / na kogo] ?\n");
    return 0;
}

int
popatrz(string str)
{
    object *cele;
    string *jak;
    int i;

    jak = parse_adverb_with_space(str, NO_DEFAULT_ADVERB, 0);
    if (!jak[0])
    {
        if (jak[1] == NO_DEFAULT_ADVERB_WITH_SPACE)
            jak[1] = ADD_SPACE_TO_ADVERB("nieobecnym wzrokiem");
        write("Spogladasz gdzies" + jak[1] + ".\n");
        allbb("spoglada gdzies" + jak[1] + ".", jak[1]);
        return 1;
    }
    if (jak[0] == "na siebie")
    {
        if (jak[1] == NO_DEFAULT_ADVERB_WITH_SPACE)
            jak[1] = ADD_SPACE_TO_ADVERB("uwaznie");
        write("Spogladasz na siebie" + jak[1] + ".\n");
        allbb("spoglada na siebie" + jak[1] + ".", jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'na' %l:" + PL_BIE);
    if (sizeof(cele))
    {
        if (jak[1] == NO_DEFAULT_ADVERB_WITH_SPACE)
            jak[1] = ADD_SPACE_TO_ADVERB("z zainteresowaniem");
        actor("Spogladasz na", cele, PL_BIE, jak[1] + ".");
        all2actbb("spoglada na", cele, PL_BIE, jak[1] + ".", jak[1]);
        targetbb("spoglada na ciebie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'na' %i:" + PL_BIE);
    if (sizeof(cele))
    {
        if (jak[1] == NO_DEFAULT_ADVERB_WITH_SPACE)
            jak[1] = ADD_SPACE_TO_ADVERB("z zainteresowaniem");
        write("Spogladasz na " + COMPOSITE_DEAD(cele, PL_BIE) + jak[1]
            + ".\n");
        saybb(QCIMIE(this_player(), PL_MIA) + " spoglada na "
            + QCOMPDEAD(PL_BIE) + jak[1] + ".\n");
        return 1;
    }
    if (sscanf(jak[0], "na %s", str))
    {
        i = member_array(str, Skroty) + member_array(str, Kierunki);
        if (i++ != -2)
        {
            if (jak[1] == NO_DEFAULT_ADVERB_WITH_SPACE)
                jak[1] = ADD_SPACE_TO_ADVERB("nieobecnym wzrokiem");
            write("Spogladasz na " + Kierunki[i] + jak[1] + ".\n");
            allbb("spoglada na " + Kierunki[i] + jak[1] + ".", jak[1]);
            return 1;
        }
        if (environment(this_player())->item_id(str) &&
            CAN_SEE_IN_ROOM(this_player()))
        {
            if (jak[1] == NO_DEFAULT_ADVERB_WITH_SPACE)
                jak[1] = ADD_SPACE_TO_ADVERB("z zainteresowaniem");
            write("Spogladasz na " + str + jak[1] + ".\n");
            allbb("spoglada na " + str + jak[1] + ".", jak[1]);
            return 1;
        }
    }
    notify_fail("Popatrz [jak] [na kogo / na co] ?\n");
    return 0;
}

int
popukaj(string str)
{
    if (str == "sie" || str == "sie w czolo" || str == "sie w glowe")
    {
        write("Pukasz sie niedwuznacznym gestem w czolo.\n");
        allbb("puka sie niedwuznacznym gestem w czolo.");
        return 1;
    }
    notify_fail("Popukaj sie [gdzie] ?\n");
    return 0;
}

int
potrzasnij(string str)
{
    object *cele;
    string *jak;

    if (str)
        sscanf(str, "glowa %s", str);

    jak = parse_adverb_with_space(str, "przeczaco", 0);
    if (!jak[0])
    {
        SOULDESC("potrzasa glowa" + jak[1]);
        write("Potrzasasz glowa" + jak[1] + ".\n");
        allbb("potrzasa glowa" + jak[1] + ".", jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'do' %l:" + PL_DOP);
    if (sizeof(cele))
    {
        SOULDESC("potrzasa glowa" + jak[1]);
        actor("Potrzasasz do", cele, PL_DOP, " glowa" + jak[1] + ".");
        all2actbb("potrzasa do", cele, PL_DOP, " glowa" + jak[1] + ".",
                  jak[1]);
        targetbb("potrzasa do ciebie glowa" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Potrzasnij [glowa] [jak] [do kogo] ?\n");
    return 0;
}

int
potrzyj(string str)
{
    if (!str || str == "czolo")
    {
        SOULDESC("pociera czolo z namyslem");
        write("Pocierasz czolo z namyslem.\n");
        allbb("pociera czolo z namyslem.");
        return 1;
    }
    notify_fail("Potrzyj [co] ?\n");
    return 0;
}

int
potwierdz(string str)
{
    object *cele;

    CHECK_MOUTH_BLOCKED;

    if (!str)
    {
        write("Potwierdzasz.\n");
        all("potwierdza.");
        return 1;
    }
    cele = parse_this(str, "'slowa' %l:" + PL_DOP);
    if (sizeof(cele))
    {
        actor("Potwierdzasz slowa", cele, PL_DOP);
        all2act("potwierdza slowa", cele, PL_DOP);
        target("potwierdza twoje slowa.", cele);
        return 1;
    }
    notify_fail("Potwierdz [czyje slowa] ?\n");
    return 0;
}

int
powitaj(string str)
{
    object *cele;
    string *jak;

    jak = parse_adverb_with_space(str, "serdecznie", 1);
    cele = parse_this(jak[0], "%l:" + PL_BIE);
    if (sizeof(cele))
    {
        actor("Witasz" + jak[1], cele, PL_BIE);
        all2actbb("wita" + jak[1], cele, PL_BIE, ".", jak[1]);
        target("wita cie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail(capitalize(query_verb()) + " kogo [jak] ?\n");
    return 0;
}

int
pozegnaj(string str)
{
    object *cele;
    string *jak;

    jak = parse_adverb_with_space(str, BLANK_ADVERB, 1);
    cele = parse_this(jak[0], "%l:" + PL_BIE);
    if (sizeof(cele))
    {
        actor("Zegnasz" + jak[1], cele, PL_BIE);
        all2actbb("zegna" + jak[1], cele, PL_BIE, ".", jak[1]);
        target("zegna cie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Pozegnaj kogo [jak] ?\n");
    return 0;
}

int
prychnij(string str)
{
    string *jak;
    object *cele;

    CHECK_MOUTH_BLOCKED;

    jak = parse_adverb_with_space(str, "z rozdraznieniem", 0);
    if (!jak[0])
    {
        write("Prychasz" + jak[1] + ".\n");
        all("prycha" + jak[1] + ".", jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'na' %l:" + PL_BIE);
    if (sizeof(cele))
    {
        actor("Prychasz na", cele, PL_BIE, jak[1] + ".");
        all2act("prycha na", cele, PL_BIE, jak[1] + ".", jak[1]);
        target("prycha na ciebie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Prychnij [jak] [na kogo] ?\n");
    return 0;
}

int
przebieraj(string str)
{
    string *jak;

    jak = parse_adverb_with_space(str, "nerwowo", 1);
    if (jak[0] == "nogami")
    {
        SOULDESC("przebiera" + jak[1] + " nogami");
        write("Przebierasz" + jak[1] + " nogami.\n");
        allbb("przebiera" + jak[1] + " nogami.", jak[1]);
        return 1;
    }
    notify_fail("Przebieraj nogami [jak] ?\n");
    return 0;
}

int
przeciagnij(string str)
{
    string *jak;

    if (str == "sie")
    {
        if (this_player()->query_gender() == G_FEMALE)
        {
            write("Przeciagasz sie rozkosznie.\n");
            allbb("przeciaga sie rozkosznie.",
                  ADD_SPACE_TO_ADVERB("rozkosznie"));
            return 1;
        }
        write("Przeciagasz sie, az cos chrupie ci w kosciach.\n");
        allbb("przeciaga sie, az cos chrupie mu w kosciach.");
        return 1;
    }
    jak = parse_adverb_with_space(str, NO_DEFAULT_ADVERB, 1);
    if (jak[0] == "sie")
    {
        write("Przeciagasz sie" + jak[1] + ".\n");
        allbb("przeciaga sie" + jak[1] + ".", jak[1]);
        return 1;
    }
    notify_fail("Przeciagnij sie [jak] ?\n");
    return 0;
}

int
przelknij(string str)
{
    if (!str || str == "sline" || str == "glosno sline")
    {
        write("Przelykasz glosno sline... Glurp!\n");
        all("glosno przelyka sline.");
        return 1;
    }
    notify_fail("Przelknij [co] ?\n");
    return 0;
}

int
przepros(string str)
{
    object *cele;
    string *jak;

    CHECK_MOUTH_BLOCKED;

    jak = parse_adverb_with_space(str, "ze skrucha", 1);
    cele = parse_this(jak[0], "%l:" + PL_BIE);
    if (sizeof(cele))
    {
        actor("Przepraszasz" + jak[1], cele, PL_BIE);
        all2actbb("przeprasza" + jak[1], cele, PL_BIE, ".", jak[1]);
        target("przeprasza cie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Przepros kogo [jak] ?\n");
    return 0;
}

int
przestap(string str)
{
    if (!str || str == "z nogi na noge")
    {
        write("Przestepujesz z nogi na noge.\n");
        allbb("przestepuje z nogi na noge.");
        return 1;
    }
    notify_fail("Przestap [z nogi na noge] ?\n");
    return 0;
}

int
przewroc(string str)
{
    if (str == "oczami" || str == "oczyma")
    {
        write("Przewracasz oczami.\n");
        allbb("przewraca oczami.");
        return 1;
    }
    notify_fail("Przewroc czym ?\n");
    return 0;
}

int
przytul(string str)
{
    object *cele;
    string *jak;

    jak = parse_adverb_with_space(str, "czule", 1);
    cele = parse_this(jak[0], "%l:" + PL_BIE);
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("przytulac jednoczesnie", PL_DOP);

        actor("Przytulasz" + jak[1], cele, PL_BIE);
        all2actbb("przytula" + jak[1], cele, PL_BIE, ".", jak[1]);
        target("przytula cie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'sie' 'do' %l:" + PL_DOP);
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("przytulac sie jednoczesnie do", PL_DOP);

        actor("Przytulasz sie" + jak[1] + " do", cele, PL_DOP);
        all2actbb("przytula sie" + jak[1] + " do", cele, PL_DOP, ".", jak[1]);
        target("przytula sie do ciebie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Przytul [sie do] kogo [jak] ?\n");
    return 0;
}

int zbeltaj(string str); /* Prototyp. */

int
pusc(string str)		/* I tak sie wypieprzy */
{
    object *cele;

    if (str == "baka")
    {
        write("Koncentrujesz sie przez chwile... Achh ! Co za ulga...\n");
        allbb("zamyka oczy i koncentruje sie.");
        tell_room(environment(this_player()), "Nagle poczules potworny wrecz "
                + "smrod. Chyba ktos puscil tu baka...\n");
        return 1;
    }
    if (str == "belta" || str == "pawia")
        return zbeltaj("sie");
    if (str == "oczko" || str == "oko")
    {
        write("Puszczasz dyskretnie oczko.\n");
        allbb("puszcza dyskretnie oczko.");
        return 1;
    }
    if (str && (sscanf(str, "oczko %s", str) || sscanf(str, "oko %s", str)))
    {
        cele = parse_this(str, "'do' %l:" + PL_DOP);
        if (sizeof(cele))
        {
            actor("Puszczasz dyskretnie oczko do", cele, PL_DOP, ".");
            all2actbb("puszcza dyskretnie oczko do", cele, PL_DOP, ".");
            targetbb("puszcza do ciebie dyskretnie oczko.", cele);
            return 1;
        }
        notify_fail("Pusc oczko [do kogo] ?\n");
        return 0;
    }
    notify_fail("Pusc co?\n");
    return 0;
}

int
rozejrzyj(string str)
{
    string *jak;

    jak = parse_adverb_with_space(str, "dookola z wyrazem zagubienia na "
                                + "twarzy", 1);
    if (jak[0] == "sie" || jak[0] == "sie dokola" || jak[0] == "sie dookola")
    {
        SOULDESC("rozglada sie" + jak[1]);
        write("Rozgladasz sie" + jak[1] + ".\n");
        allbb("rozglada sie" + jak[1] + ".", jak[1]);
        return 1;
    }
    notify_fail("Rozejrzyj sie [dookola] [jak] ?\n");
    return 0;
}

int
rozplacz(string str)
{
    if (str == "sie")
    {
        SOULDESC("tonie we lzach");
        write("Wybuchasz placzem.\n");
        allbb("wybucha placzem.");
        return 1;
    }
    notify_fail("Rozplacz sie?\n");
    return 0;
}

int
sapnij(string str)
{
    string jak;

    CHECK_MOUTH_BLOCKED;

    jak = check_adverb_with_space(str, "desperacko");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        write("Sapiesz" + jak + ".\n");
        all("sapie" + jak + ".", jak);
        return 1;
    }
    notify_fail("Sapnij [jak] ?\n");
    return 0;
}

int
skin(string str)
{
    object *cele;

    if (!str || str == "glowa")
        return pokiwaj("glowa .");

    if (str == "dlonia")
    {
        notify_fail(capitalize(query_verb()) + " dlonia na kogo?");
        return 0;
    }
    if (str == "reka")
    {
        notify_fail(capitalize(query_verb()) + " reka na kogo?");
        return 0;
    }
    cele = parse_this(str, "'glowa' 'na' %l:" + PL_BIE);
    if (sizeof(cele))
    {
        actor("Kiwasz glowa na", cele, PL_BIE);
        all2actbb("kiwa glowa na", cele, PL_BIE);
        targetbb("kiwa na ciebie glowa.", cele);
        return 1;
    }
    cele = parse_this(str, "[dlonia] / [reka] 'na' %l:" + PL_BIE);
    if (sizeof(cele))
    {
        actor("Kiwasz dlonia na", cele, PL_BIE);
        all2actbb("kiwa dlonia na", cele, PL_BIE);
        targetbb("kiwa na ciebie dlonia.", cele);
        return 1;
    }
    notify_fail(capitalize(query_verb()) + " [czym] [na kogo] ?\n");
    return 0;
}

int
skrzyw(string str)
{
    object *cele;
    string *jak;

    if (str == "sie")
    {
        SOULDESC("krzywi sie z niesmakiem");
        write("Krzywisz sie z niesmakiem.\n");
        allbb("krzywi sie z niesmakiem.",
              ADD_SPACE_TO_ADVERB("z niesmakiem"));
        return 1;
    }
    if (str && sscanf(str, "sie %s", str))
    {
        jak = parse_adverb_with_space(str, NO_DEFAULT_ADVERB, 0);
        if (!jak[0])
        {
            SOULDESC("krzywi sie" + jak[1]);
            write("Krzywisz sie" + jak[1] + ".\n");
            allbb("krzywi sie" + jak[1] + ".", jak[1]);
            return 1;
        }
        cele = parse_this(jak[0], "'do' %l:" + PL_DOP);
        if (sizeof(cele))
        {
            if (jak[1] == NO_DEFAULT_ADVERB_WITH_SPACE)
                jak[1] = ADD_SPACE_TO_ADVERB("zlosliwie");
            SOULDESC("krzywi sie" + jak[1]);
            actor("Krzywisz sie do", cele, PL_DOP, jak[1] + ".");
            all2actbb("krzywi sie do", cele, PL_DOP, jak[1] + ".", jak[1]);
            targetbb("krzywi sie do ciebie" + jak[1] + ".", cele, jak[1]);
            return 1;
        }
        cele = parse_this(jak[0], "'na' %l:" + PL_BIE);
        if (sizeof(cele))
        {
            if (jak[1] == NO_DEFAULT_ADVERB_WITH_SPACE)
                jak[1] = ADD_SPACE_TO_ADVERB("oblesnie");
            SOULDESC("krzywi sie" + jak[1]);
            actor("Krzywisz sie na", cele, PL_BIE, jak[1] + ".");
            all2actbb("krzywi sie na", cele, PL_BIE, jak[1] + ".", jak[1]);
            targetbb("krzywi sie na ciebie" + jak[1] + ".", cele, jak[1]);
            return 1;
        }
    }
    notify_fail(capitalize(query_verb())
              + " sie [jak] [do kogo] / [na kogo] ?\n");
    return 0;
}

int
spanikuj(string str)
{
    object *cele;

    if (!str)
    {
        write("Wpadasz w panike!\n");
        allbb("wpada w panike!");
        return 1;
    }
    cele = parse_this(str, "'przed' %l:" + PL_NAR);
    if (sizeof(cele))
    {
        actor("Wpadasz w panike na widok", cele, PL_DOP, "!");
        all2actbb("wpada w panike na widok", cele, PL_DOP, "!");
        targetbb("wpada w panike na twoj widok!", cele);
        return 1;
    }
    notify_fail("Spanikuj [przed kim] ?\n");
    return 0;
}

int
splun(string str)
{
    object *cele;

    if (!str || str == "na ziemie" || str == "na podloge")
    {
        write("Spluwasz z niesmakiem na ziemie.\n");
        allbb("spluwa z niesmakiem na ziemie.");
        return 1;
    }
    if (wildmatch("pod *", str))
    {
        cele = parse_this(str, "'pod' 'nogi' %l:" + PL_CEL);
        if (sizeof(cele))
        {
        CHECK_MULTI_TARGETS("spluwac jednoczesnie pod nogi", PL_CEL);

            actor("Spluwasz z pogarda pod nogi", cele, PL_CEL);
            all2actbb("spluwa z pogarda pod nogi", cele, PL_CEL, ".");
            targetbb("spluwa ci z pogarda pod nogi.", cele);
            return 1;
        }
        notify_fail("Splun pod nogi komu?\n");
        return 0;
    }
    notify_fail("Splun [gdzie] ?\n");
    return 0;
}

int
spoliczkuj(string str)
{
    object *cele;

    cele = parse_this(str, "%l:" + PL_BIE);
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("policzkowac jednoczesnie", PL_DOP);

        actor("Bierzesz rozmach i wymierzasz siarczysty policzek", cele,
              PL_CEL);
        all2act("bierze rozmach i wymierza siarczysty policzek", cele,
                PL_CEL);
        target("wymierza ci siarczysty policzek. Auuu !", cele);
        cele->add_prop(LIVE_S_SOULEXTRA, "rozciera obolaly policzek");
        return 1;
    }
    notify_fail("Spoliczkuj kogo?\n");
    return 0;
}

int
szepnij(string str)
{
    object *cele;

    CHECK_MOUTH_BLOCKED;

    if (str && CAN_SEE_IN_ROOM(this_player()) &&
        parse_command(str, environment(this_player()),
                      "%l:" + PL_CEL + " %s", cele, str) &&
        sizeof(cele = NORMAL_ACCESS(cele, 0, 0)))
    {
        this_player()->set_obiekty_zaimkow(cele);
        CHECK_MULTI_TARGETS("szeptac jednoczesnie do", PL_DOP);

        if (strlen(str))
        {
            if (this_player()->query_option(OPT_ECHO))
                actor("Szepczesz do", cele, PL_DOP, ": " + str);
            else
                write("Ok.\n");
            all2actbb("szepcze cos do", cele, PL_DOP);
            target("szepcze do ciebie: " + str, cele);
            return 1;
        }
        notify_fail("Szepnij " + COMPOSITE_LIVE(cele, PL_CEL) + " co?\n");
        return 0;
    }
    notify_fail("Szepnij komu co?\n");
    return 0;
}

int
szturchnij(string str)
{
    object *cele;

    cele = parse_this(str, "%l:" + PL_BIE);
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("szturchac jednoczesnie", PL_DOP);

        actor("Szturchasz", cele, PL_BIE);
        all2actbb("szturcha", cele, PL_BIE, ".");
        target("szturcha cie.", cele);
        return 1;
    }
    notify_fail("Szturchnij kogo?\n");
    return 0;
}

int
tupnij(string str)
{
    object *cele;
    string *jak;

    jak = parse_adverb_with_space(str, "niecierpliwie", 0);
    if (!jak[0])
    {
        write("Tupiesz" + jak[1] + ".\n");
        allbb("tupie" + jak[1] + ".", jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'na' %l:" + PL_BIE);
    if (sizeof(cele))
    {
        actor("Tupiesz na", cele, PL_BIE, jak[1] + ".");
        all2actbb("tupie na", cele, PL_BIE, jak[1] + ".", jak[1]);
        targetbb("tupie na ciebie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Tupnij [jak] [na kogo] ?\n");
    return 0;
}

int
uklon(string str)
{
    object *cele;
    string *jak;

    if (str == "sie")
        str = 0;
    else if (str)
        sscanf(str, "sie %s", str);

    jak = parse_adverb_with_space(str, "z gracja", 1);
    if (!jak[0])
    {
        write("Klaniasz sie" + jak[1] + ".\n");
        allbb("klania sie" + jak[1] + ".", jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "%l:" + PL_CEL);
    if (sizeof(cele))
    {
        actor("Klaniasz sie" + jak[1], cele, PL_CEL);
        all2actbb("klania sie" + jak[1], cele, PL_CEL, ".", jak[1]);
        targetbb("klania sie ci" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail(capitalize(query_verb()) + " [sie] [komu] [jak] ?\n");
    return 0;
}

int
usciskaj(string str)
{
    object *cele;
    string *jak;

    jak = parse_adverb_with_space(str, "kordialnie", 1);
    cele = parse_this(jak[0], "%l:" + PL_BIE);
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("sciskac jednoczesnie", PL_DOP);

        actor("Sciskasz" + jak[1], cele, PL_BIE);
        all2actbb("sciska" + jak[1], cele, PL_BIE, ".", jak[1]);
        target("sciska cie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Usciskaj kogo [jak] ?\n");
    return 0;
}

int
usmiechnij(string str)
{
    object *cele;
    string *jak;

    if (query_verb() == "usmiech")
        str = str ? "sie " + str : "sie";

    if (str == "sie")
    {
        SOULDESC("usmiecha sie wesolo");
        write("Usmiechasz sie wesolo.\n");
        allbb("usmiecha sie wesolo.",
              ADD_SPACE_TO_ADVERB("wesolo"));
        return 1;
    }
    if (str && sscanf(str, "sie %s", str))
    {
        jak = parse_adverb_with_space(str, BLANK_ADVERB, 0);
        if (!jak[0])
        {
            SOULDESC("usmiecha sie" + jak[1]);
            write("Usmiechasz sie" + jak[1] + ".\n");
            allbb("usmiecha sie" + jak[1] + ".", jak[1]);
            return 1;
        }
        cele = parse_this(jak[0], "'do' %l:" + PL_DOP);
        if (sizeof(cele))
        {
            SOULDESC("usmiecha sie" + jak[1]);
            actor("Usmiechasz sie do", cele, PL_DOP, jak[1] + ".");
            all2actbb("usmiecha sie do", cele, PL_DOP, jak[1] + ".", jak[1]);
            targetbb("usmiecha sie do ciebie" + jak[1] + ".", cele, jak[1]);
            return 1;
        }
    }
    notify_fail("Usmiech[nij sie] [jak] [do kogo] ?\n");
    return 0;
}

int
warknij(string str)
{
    object *cele;
    string *jak;

    CHECK_MOUTH_BLOCKED;

    jak = parse_adverb_with_space(str, "groznie", 0);
    if (!jak[0])
    {
        write("Warczysz" + jak[1] + ".\n");
        all("warczy" + jak[1] + ".", jak[1]);
        return 1;
    }
    cele = parse_this(jak[0], "'na' %l:" + PL_BIE);
    if (sizeof(cele))
    {
        actor("Warczysz na", cele, PL_BIE, jak[1] + ".");
        all2act("warczy na", cele, PL_BIE, jak[1] + ".", jak[1]);
        target("warczy na ciebie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Warknij [jak] [na kogo] ?\n");
    return 0;
}

int
westchnij(string str)
{
    string jak;

    CHECK_MOUTH_BLOCKED;

    jak = check_adverb_with_space(str, "ze smutkiem");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        write("Wzdychasz" + jak + ".\n");
        all("wzdycha" + jak + ".", jak);
        return 1;
    }
    notify_fail("Westchnij [jak] ?\n");
    return 0;
}

int
wskaz(string str)
{
    object *cele;
    int i;

    if (!str)
    {
        write("Wskazujesz nieokreslony kierunek.\n");
        allbb("wskazuje nieokreslony kierunek.");
        return 1;
    }
    if (str == "siebie" || str == "na siebie")
    {
        write("Wskazujesz na siebie.\n");
        allbb("wskazuje na siebie.");
        return 1;
    }
    cele = parse_this(str, "[na] %l:" + PL_BIE);
    if (sizeof(cele))
    {
        actor("Wskazujesz", cele, PL_BIE);
        all2actbb("wskazuje", cele, PL_BIE);
        targetbb("wskazuje na ciebie.", cele);
        return 1;
    }
    cele = parse_this(str, "[na] %i:" + PL_BIE);
    if (sizeof(cele))
    {
        write("Wskazujesz " + COMPOSITE_DEAD(cele, PL_BIE) + ".\n");
        saybb(QCIMIE(this_player(), PL_MIA) + " wskazuje "
            + QCOMPDEAD(PL_BIE) + ".\n");
        return 1;
    }
    sscanf(str, "na %s", str);
    i = member_array(str, Skroty) + member_array(str, Kierunki);
    if (i++ != -2)
    {
        write("Wskazujesz na " + Kierunki[i] + ".\n");
        allbb("wskazuje na " + Kierunki[i] + ".");
        return 1;
    }
    if (environment(this_player())->item_id(str) &&
        CAN_SEE_IN_ROOM(this_player()))
    {
         write("Wskazujesz na " + str + ".\n");
         allbb("wskazuje na " + str + ".");
         return 1;
    }
    notify_fail("Wskaz [[na] kogo / co] ?\n");
    return 0;
}

int
wybacz(string str)
{
    object *cele;
    string *jak;

    CHECK_MOUTH_BLOCKED;

    jak = parse_adverb_with_space(str, "wielkodusznie", 1);
    cele = parse_this(jak[0], "%l:" + PL_CEL);
    if (sizeof(cele))
    {
        actor("Wybaczasz" + jak[1], cele, PL_CEL);
        all2actbb("wybacza" + jak[1], cele, PL_CEL, ".", jak[1]);
        targetbb("wybacza ci" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Wybacz komu [jak] ?\n");
    return 0;
}

int
wyplacz(string str)
{
    object *cele;

    if (sizeof(cele = parse_this(str, "'sie' 'na' %l:" + PL_MIE)) ||
        sizeof(cele = parse_this(str, "'sie' 'na' 'ramieniu' %l:" + PL_DOP)))
    {
        CHECK_MULTI_TARGETS("wyplakiwac sie jednoczesnie na ramieniu",
                            PL_DOP);

        SOULDESC("tonie we lzach");
        actor("Wyplakujesz sie na ramieniu", cele, PL_DOP);
        all2actbb("wyplakuje sie na ramieniu", cele, PL_DOP, ".");
        target("wyplakuje sie na twoim ramieniu.", cele);
        return 1;
    }
    notify_fail("Wyplacz sie na kim / na czyim ramieniu?\n");
    return 0;
}

int
wytrzeszcz(string str)
{
    object *cele;

    if (!str || str == "sie" || str == "oczy" || str == "slepia" ||
        str == "galy")
    {
        SOULDESC("ma wytrzeszczone oczy");
        write("Wytrzeszczasz oczy w niemym zdumieniu.\n");
        allbb("wytrzeszcza oczy w niemym zdumieniu.");
        return 1;
    }
    cele = parse_this(str, "[sie] / [oczy] / [slepia] / [galy] 'na' %l:"
                    + PL_BIE);
    if (sizeof(cele))
    {
        SOULDESC("ma wytrzeszczone oczy");
        actor("Wytrzeszczasz oczy na", cele, PL_BIE, " w niemym "
            + "zdumieniu.");
        all2actbb("wytrzeszcza oczy na", cele, PL_BIE, " w niemym "
                + "zdumieniu.");
        targetbb("wytrzeszcza na ciebie oczy w niemym zdumieniu.", cele);
        return 1;
    }
    notify_fail(capitalize(query_verb()) + " [co] [na kogo] ?\n");
    return 0;
}

int
wzdrygnij(string str)
{
    if (str == "sie" || (str && wildmatch("sie *mysl*", str)))
    {
        if (strlen(str) > MAX_FREE_EMOTE)
            str = "sie na mysl o czyms szczegolnie potwornym";

        write("Wzdrygasz " + str + ". Brrr... !\n");
        allbb("wzdryga " + str + ".");
        return 1;
    }

    notify_fail("Wzdrygnij sie [na mysl o czym] ?\n");
    return 0;
}

int
wzrusz(string str)
{
    switch (str)
    {
        case "sie":
            SOULDESC("jest czyms bardzo wzruszon"
                     + this_player()->koncowka("y", "a", "e"));
            write("Wzruszasz sie.\n");
            allbb("jest czyms bardzo wzruszon"
                  + this_player()->koncowka("y", "a", "e") + ".");
            return 1;
        case "ramionami":
            write("Wzruszasz ramionami.\n");
            allbb("wzrusza ramionami.");
            return 1;
    }
    notify_fail("Wzrusz sie? / Wzrusz czym?\n");
    return 0;
}

int
zachichocz(string str)
{
    string jak;

    CHECK_MOUTH_BLOCKED;

    jak = check_adverb_with_space(str, "radosnie");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        SOULDESC("chichocze" + jak);
        write("Chichoczesz" + jak + ".\n");
        all("chichocze" + jak + ".", jak);
        return 1;
    }
    notify_fail("Zachichocz [jak] ?\n");
    return 0;
}

int
zachlip(string str)
{
    string jak;

    CHECK_MOUTH_BLOCKED;

    jak = check_adverb_with_space(str, "zalosnie");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        SOULDESC("chlipie" + jak);
        write("Chlipiesz" + jak + ".\n");
        all("chlipie" + jak + ".", jak);
        return 1;
    }
    notify_fail("Zachlip [jak] ?\n");
    return 0;
}

int
zachrumkaj(string str)
{
    string jak;

    CHECK_MOUTH_BLOCKED;

    jak = check_adverb_with_space(str, "jak swinka");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        write("Chrumkasz" + jak + ".\n");
        all("zadziera ryjek i chrumka" + jak + ".", jak);
        return 1;
    }
    notify_fail("Zachrumkaj [jak] ?\n");
    return 0;
}

int
zaczerwien(string str)
{
    if (str == "sie")
    {
        SOULDESC("jest czerwon" + this_player()->koncowka("y", "a", "e")
               + " po czubki uszu");
        write("Czerwienisz sie po czubki uszu.\n");
        allbb("czerwieni sie po czubki uszu.");
        return 1;
    }
    notify_fail("Zaczerwien sie?\n");
    return 0;
}

int
zagryz(string str)
{
    string *jak;

    jak = parse_adverb_with_space(str, BLANK_ADVERB, 1);
    if (jak[0] == "usta" || jak[0] == "wargi")
    {
        write("Zagryzasz" + jak[1] + " wargi.\n");
        allbb("zagryza" + jak[1] + " wargi.", jak[1]);
        return 1;
    }
    notify_fail("Zagryz wargi [jak] ?\n");
    return 0;
}

int
zagwizdz(string str)
{
    object *cele;
    string *jak;

    CHECK_MOUTH_BLOCKED;

    jak = parse_adverb_with_space(str, "z podziwem", 0);
    if (!jak[0])
    {
        write("Gwizdzesz" + jak[1] + ".\n");
        all("gwizdze" + jak[1] + ".");
        return 1;
    }
    cele = parse_this(jak[0], "'na' %l:" + PL_BIE);
    if (sizeof(cele))
    {
        actor("Gwizdzesz na", cele, PL_BIE, jak[1] + ".");
        all2act("gwizdze na", cele, PL_BIE, jak[1] + ".", jak[1]);
        target("gwizdze na ciebie" + jak[1] + ".", cele, jak[1]);
        return 1;
    }
    notify_fail("Zagwizdz [jak] [na kogo] ?\n");
    return 0;
}

int
zakrztus(string str)
{
    if (str == "sie")
    {
        SOULDESC("krztusi sie");
        write("Zakrztusil" + this_player()->koncowka("es", "as")
            +  " sie nagle.\n");
        allbb("krztusi sie nagle.");
        return 1;
    }
    notify_fail("Zakrztus sie?\n");
    return 0;
}

int
zalam(string str)
{
    if (str == "sie")
    {
        SOULDESC("jest calkowicie zalaman"
                 + this_player()->koncowka("y", "a", "e"));
        write("Zalamujesz sie calkowicie.\n");
        allbb("zalamuje sie calkowicie.");
        return 1;
    }
    notify_fail("Zalam sie?\n");
    return 0;
}

int
zalkaj(string str)
{
    if (!str)
    {
        SOULDESC("zanosi sie lkaniem");
        write("Zanosisz sie lkaniem.\n");
        all("zanosi sie lkaniem.");
        return 1;
    }
    notify_fail("Zalkaj?\n");
    return 0;
}

int
zamrucz(string str)
{
    string jak;

    CHECK_MOUTH_BLOCKED;

    jak = check_adverb_with_space(str, "z luboscia");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        SOULDESC("mruczy" + jak);
        write("Mruczysz" + jak + ".\n");
        all("mruczy" + jak + ".", jak);
        return 1;
    }
    notify_fail("Zamrucz [jak] ?\n");
    return 0;
}

int
zaprzecz(string str)
{
    object *cele;

    CHECK_MOUTH_BLOCKED;

    if (!str)
    {
        write("Zdecydowanie zaprzeczasz.\n");
        all("zdecydowanie zaprzecza.");
        return 1;
    }
    cele = parse_this(str, "'slowom' %l:" + PL_DOP);
    if (sizeof(cele))
    {
        actor("Zaprzeczasz slowom", cele, PL_DOP);
        all2act("zaprzecza slowom", cele, PL_DOP);
        target("zaprzecza twoim slowom.", cele);
        return 1;
    }
    notify_fail("Zaprzecz [czyim slowom] ?\n");
    return 0;
}

int
zarechocz(string str)
{
    string jak;

    CHECK_MOUTH_BLOCKED;

    jak = check_adverb_with_space(str, "rubasznie");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        SOULDESC("rechocze" + jak);
        write("Rechoczesz" + jak + ".\n");
        all("rechocze" + jak + ".", jak);
        return 1;
    }
    notify_fail("Zarechocz [jak] ?\n");
    return 0;
}

int
zasmiej(string str)
{
    object *cele;
    string *jak;

    CHECK_MOUTH_BLOCKED;

    if (str == "sie")
    {
        SOULDESC("smieje sie radosnie");
        write("Wybuchasz smiechem.\n");
        all("wybucha smiechem.", NO_ADVERB_WITH_SPACE);
        return 1;
    }
    if (str && sscanf(str, "sie %s", str))
    {
        jak = parse_adverb_with_space(str, "kpiaco", 0);
        if (!jak[0])
        {
            SOULDESC("smieje sie" + jak[1]);
            write("Smiejesz sie" + jak[1] + ".\n");
            all("smieje sie" + jak[1] + ".", jak[1]);
            return 1;
        }
        cele = parse_this(jak[0], "'z' %l:" + PL_DOP);
        if (sizeof(cele))
        {
            SOULDESC("smieje sie" + jak[1]);
            actor("Smiejesz sie z", cele, PL_DOP, jak[1] + ".");
            all2act("smieje sie z", cele, PL_DOP, jak[1] + ".", jak[1]);
            target("smieje sie z ciebie" + jak[1] + ".", cele, jak[1]);
            return 1;
        }
    }
    notify_fail("Zasmiej sie [jak] [z kogo] ?\n");
    return 0;
}

int
zatancz(string str)
{
    object *cele;

    if (!str)
    {
        write("Wykonujesz dziki taniec, dajac upust rozpierajacej cie "
            + "energii.\n");
        allbb("wykonuje dziki taniec, dajac upust rozpierajacej "
            + this_player()->koncowka("go", "ja", "je") + " energii.");
        return 1;
    }
    cele = parse_this(str, "'z' %l:" + PL_NAR);
    if (sizeof(cele))
    {
        CHECK_MULTI_TARGETS("tanczyc jednoczesnie z", PL_NAR);

        actor("Porywasz", cele, PL_BIE, " do dzikiego tanca, wycinajac "
            + "kilka holubcow.");
        all2actbb("porywa", cele, PL_BIE, " do dzikiego tanca, wycinajac "
                + "kilka holubcow.");
        target("porywa cie do dzikiego tanca, wycinajac kilka holubcow.",
               cele);
        return 1;
    }
    notify_fail("Zatancz [z kim] ?\n");
    return 0;
}

int
zatrzyj(string str)
{
    if (!str || str == "rece")
    {
        write("Zacierasz rece.\n");
        allbb("zaciera rece.");
        return 1;
    }
    notify_fail("Zatrzyj [rece] ?\n");
    return 0;
}

int
zazgrzytaj(string str)
{
    object *cele;
    string *jak;

    if (!str || str == "zebami")
    {
        SOULDESC("zgrzyta zebami");
        write("Zgrzytasz z wsciekloscia zebami.\n");
        all("zgrzyta wsciekle zebami.");
        return 1;
    }
    notify_fail("Zazgrzytaj [zebami] ?\n");
    return 0;
}

int
zbeltaj(string str)
{
    object *cele;

    if (query_verb() == "zwymiotuj")
        str = str ? "sie " + str : "sie";

    if (str == "sie")
    {
        SOULDESC("jest unurzan" + this_player()->koncowka("y", "a", "e")
               + " we wlasnych wymiocinach");
        write("Zginasz sie w pol, czujac nagla potrzebe pozbycia sie "
            + "zawartosci zoladka. Skrecajac sie z bolu w miare jak twymi "
            + "wnetrznosciami targaja torsje, dlugo wyrzucasz z siebie na "
            + "wpol przetrawione resztki.\n"
            + "Z trudem znajdujesz w sobie sily by chwiejac sie na nogach "
            + "wstac wreszcie z kleczek. Coz za zalosny widok...\n");

        all("puszcza belta, zaswiniajac wszystko wokol, a przy okazji "
          + "siebie.");

        this_player()->set_stuffed(0);
        this_player()->add_fatigue(-MAX(66,
            2 * this_player()->query_max_fatigue() / 3));

        return 1;
    }
#if 0
    else if (sizeof(cele = parse_this(str, "'sie' 'na' %l:" + PL_BIE)) ||
        sizeof(cele = parse_this(str, "'sie' 'na' 'buty' %l:" + PL_CEL)))
    {
        CHECK_MULTI_TARGETS("zanieczyszczac jednoczesnie butow", PL_CEL);
        return 1;
    }

    notify_fail((query_verb() == "zwymiotuj" ? "Zwymiotuj" : "Zbeltaj sie")
              + " [na kogo] / [na buty komu] ?\n");
#endif
    notify_fail((query_verb() == "zwymiotuj" ? "Zwymiotuj" : "Zbeltaj sie")
              + "?\n");
    return 0;
}

int
zblednij(string str)
{
    if (!str)
    {
        SOULDESC("jest blad" + this_player()->koncowka("y", "a")
               + " jak sciana");
        write("Bledniesz jak sciana.\n");
        allbb("blednie jak sciana.");
        return 1;
    }
    notify_fail("Zblednij?\n");
    return 0;
}

int
zdziw(string str)
{
    if (str == "sie")
    {
        SOULDESC("wyglada na bardzo zdziwion"
               + this_player()->koncowka("ego", "a"));
        write("Unosisz brwi w niemym gescie zdumienia.\n");
        allbb("unosi brwi w niemym gescie zdumienia.");
        return 1;
    }
    notify_fail("Zdziw sie?\n");
    return 0;
}

int
ziewnij(string str)
{
    string jak;

    jak = check_adverb_with_space(str, "sennie");
    if (jak != NO_ADVERB_WITH_SPACE)
    {
        SOULDESC("ziewa" + jak);
        write("Na wpol przymykasz oczy, otwierasz usta i ziewasz" + jak
            + ".\n");
        allbb("na wpol przymyka oczy, otwiera usta i ziewa" + jak + ". Chyba "
            + "tez masz ochote sobie ziewnac.", jak);
        return 1;
    }
    notify_fail("Ziewnij [jak] ?\n");
    return 0;
}

int
zignoruj(string str)
{
    object *cele;

    cele = parse_this(str, "%l:" + PL_BIE);
    if (sizeof(cele))
    {
        actor("Odwrociwszy sie plecami, ignorujesz demonstracyjnie", cele,
              PL_BIE);
        all2actbb(", odwrociwszy sie plecami, ignoruje demonstracyjnie", cele,
              PL_BIE);
        targetbb("odwraca sie do ciebie plecami, demonstracyjnie cie "
               + "ignorujac.", cele);
        return 1;
    }
    notify_fail("Zignoruj kogo?\n");
    return 0;
}

int
zmarszcz(string str)
{
    object *cele;
    string *jak;

    if (str == "brwi")
    {
        SOULDESC("marszczy gniewnie brwi");
        write("Marszczysz gniewnie brwi.\n");
        allbb("marszczy gniewnie brwi.",
              ADD_SPACE_TO_ADVERB("gniewnie"));
        return 1;
    }

    if (str == "czolo")
    {
        SOULDESC("marszczy z namyslem czolo");
        write("Marszczysz z namyslem czolo.\n");
        allbb("marszczy z namyslem czolo.",
              ADD_SPACE_TO_ADVERB("z namyslem"));
        return 1;
    }

    if (str && sscanf(str, "brwi %s", str))
    {
        jak = parse_adverb_with_space(str, "gniewnie", 0);
        if (!jak[0])
        {
            SOULDESC("marszczy" + jak[1] + " brwi");
            write("Marszczysz" + jak[1] + " brwi.\n");
            allbb("marszczy" + jak[1] + " brwi.", jak[1]);
            return 1;
        }

        cele = parse_this(jak[0], "'na' [widok] %l:" + PL_DOP);
        if (sizeof(cele))
        {
            SOULDESC("marszczy" + jak[1] + " brwi");
            actor("Marszczysz" + jak[1] + " brwi na widok", cele, PL_DOP);
            all2actbb("marszczy" + jak[1] + " brwi na widok", cele, PL_DOP,
                      jak[1]);
            targetbb("marszczy" + jak[1] + " brwi na twoj widok.", cele,
                      jak[1]);
            return 1;
        }
    }

    if (str && sscanf(str, "czolo %s", str))
    {
        jak = parse_adverb_with_space(str, "z namyslem", 0);
        if (!jak[0])
        {
            SOULDESC("marszczy" + jak[1] + " czolo");
            write("Marszczysz" + jak[1] + " czolo.\n");
            allbb("marszczy" + jak[1] + " czolo.", jak[1]);
            return 1;
        }

        cele = parse_this(jak[0], "'na' [widok] %l:" + PL_DOP);
        if (sizeof(cele))
        {
            SOULDESC("marszczy" + jak[1] + " czolo");
            actor("Marszczysz" + jak[1] + " czolo na widok", cele, PL_DOP);
            all2actbb("marszczy" + jak[1] + " czolo na widok", cele, PL_DOP,
                      jak[1]);
            targetbb("marszczy" + jak[1] + " czolo na twoj widok.", cele,
                      jak[1]);
            return 1;
        }
    }

    notify_fail("Zmarszcz co [jak] [na [widok] kogo] ?\n");
    return 0;
}

int
zmruz(string str)
{
    if (str == "oczy")
    {
        write("Mruzysz oczy jak kot.\n");
        allbb("mruzy oczy jak kot.");
        return 1;
    }
    notify_fail(capitalize(query_verb()) + " oczy?\n");
    return 0;
}

/* oryginaly

nomask void
converse_more(string str)
{
    if (str == "**")
    {
	write("Left converse mode.\n");
	return;
    }

    say( ({ METNAME + LD_SAYS + LD_UNDERSTANDS(str) + "\n",
	    TART_NONMETNAME + LD_SAYS + LD_UNDERSTANDS(str) + "\n",
	    UNSEEN_NAME + LD_SAYS + LD_UNDERSTANDS(str) + "\n" }) );

    write("]");
    input_to(converse_more);
}

int
converse()
{
    write("Entering converse mode.\nGive '**' to stop.\n");
    write("]");
    input_to(converse_more);
    return 1;
}

int
grumble(string str)
{
    object *oblist;
    string *how;

    how = parse_adverb_with_space(str,
	(this_player()->query_wiz_level() ? "angrily" : "unhappily"), 0);

    if (!stringp(how[0]))
    {
	SOULDESC("grumbling" + how[1]);
	write("You grumble" + how[1] + ".\n");
	all(" grumbles" + how[1] + ".", how[1]);
	return 1;
    }

    oblist = parse_this(how[0], "[at] [the] %l");

    if (!sizeof(oblist))
    {
	notify_fail("Grumble [how] at whom?\n");
	return 0;
    }

    SOULDESC("grumbling" + how[1]);
    actor("You grumble" + how[1] + " at", oblist);
    all2act("grumbles" + how[1] + " at", oblist, 0, how[1]);
    target(" grumbles" + how[1] + " at you.", oblist, how[1]);
    return 1;
}

int
knee(string str)
{
    object *oblist;
    object *femlist;

    oblist = parse_this(str, "[the] %l");

    if (!sizeof(oblist))
    {
	notify_fail("Knee whom?\n");
	return 0;
    }

    femlist = FILTER_GENDER(oblist, G_FEMALE);
    if (sizeof(femlist))
    {
	actor("You try to knee", femlist, ".\nNot very effective though.");
	all2act("tries to knee", femlist, ".\nNot very effective though.");
	target(" tries to knee you, without much effect.", femlist);
    }

    oblist -= femlist;
    if (sizeof(oblist))
    {
	actor("You hit", oblist, " with your knee, sending " +
	    ((sizeof(oblist) > 1) ? "them" : "him") +
	    " to the ground, writhing in pain!");
	all2act("suddenly raises " + this_player()->query_possessive() +
	    " knee, sending", oblist, " to the floor, writhing in pain!");
	target(" hits you with " + this_player()->query_possessive() +
	    " knee below your belt!\n" +
	    "You double over and fall to the ground, writhing in " +
	    "excrutiating pain,\nfeeling like you may throw up " +
	    "everything you have eaten!", oblist);
    }

    return 1;
}

int
mumble(string str)
{
    object *oblist;

    if (!stringp(str))
    {
	SOULDESC("mumbling about something");
	write("You mumble something about something else.\n");
	all(" mumbles something about something else.");
	return 1;
    }

    if ((strlen(str) > 60) &&
	(!(this_player()->query_wiz_level())))
    {
	SOULDESC("mumbling about something");
	write("You mumble beyond the end of the line and .\n");
	all(" mumbles something about something else.");
	return 1;
    }

    oblist = parse_this(str, "[about] [the] %l");

    if (!sizeof(oblist))
    {
	SOULDESC("mumbling about something");
	write("You mumble " + str + "\n");
	all(" mumbles " + str);
	return 1;
    }

    SOULDESC("mumbling about someone");
    actor("You mumble something about", oblist);
    all2act("mumbles some about", oblist,
	" and it probably is a good thing you cannot understand it.");
    target(" mumbles about you and it probably is a good thing you cannot " +
	"understand it.", oblist);
    return 1;    
}

int
pat(string str)
{
    object *oblist;
    string *zones;
    string one, two;

    zones = ({ "back", "head", "shoulder" });

    notify_fail("Whom are you trying to pat?\n");
    if (!stringp(str))
    {
	str = "head";
    }

    str = lower_case(str);
    if (member_array(str, zones) != -1)
    {
	write("You pat yourself on your " + str + ".\n");
	all(" pats " + this_player()->query_objective() +
	   "self on " + this_player()->query_possessive() + " " + str + ".");
	return 1;
    }

    if (sscanf(str, "%s %s", one, two) == 2)
    {
	if (member_array(two, zones) == -1)
	    return 0;
	str = one;
    }

    if (!stringp(two))
	two = "head";

    oblist = parse_this(str, "[the] %l [on] [the]");
    if (!sizeof(oblist))
    {
	return 0;
    }

    str = ((sizeof(oblist) == 1) ?
	   (oblist[0]->query_possessive() + " " + two + ".") :
	   ("their " + two + "s."));

    actor("You pat", oblist, " on " + str);
    all2act("pats", oblist, " on " + str);
    target(" pats you on your " + two + ".", oblist);
    return 1;
}

#define PINCH_ZONES ({ "cheek", "ear", "nose", "arm", "bottom" })

int
pinch(string str)
{
    object *oblist;
    string  location;
    string *words;

    if (!stringp(str))
    {
	notify_fail("Pinch whom [where]?\n");
	return 0;
    }

    oblist = parse_this(str, "[the] %l [in] [the] " +
	"[cheek] [ear] [nose] [arm] [bottom]");

    if (!sizeof(oblist))
    {
	notify_fail("Pinch whom [where]?\n");
	return 0;
    }

    words = explode(lower_case(str), " ");
    if (member_array(words[sizeof(words) - 1], PINCH_ZONES) != -1)
    {
	location = words[sizeof(words) - 1];
    }
    else
    {
	location = "cheek";
    }

    actor("You pinch", oblist, "'s " + location + ".");
    all2act("pinches", oblist, "'s " + location + ".");
    target(" pinches your " + location + ".", oblist);
    return 1;
}

#define POKE_ZONES ({ "eye", "ear", "nose", "thorax", "abdomen", \
		"shoulder", "ribs" })

int
poke(string str)
{
    object *oblist;
    string  location;
    string *words;

    if (!stringp(str))
    {
	notify_fail("Poke whom [where]?\n");
	return 0;
    }

    oblist = parse_this(str, "[the] %l [in] [the] [eye] [ear] [nose] " +
	"[thorax] [abdomen] [shoulder] [ribs]");

    if (!sizeof(oblist))
    {
	notify_fail("Poke whom [where]?\n");
	return 0;
    }

    words = explode(str, " ");
    if (member_array(words[sizeof(words) - 1], POKE_ZONES) != -1)
    {
	location = words[sizeof(words) - 1];
    }
    else
    {
	location = "ribs";
    }

    actor("You poke", oblist, " in the " + location + ".");
    all2act("pokes", oblist, " in the " + location + ".");
    target(" pokes you in the " + location + ".", oblist);
    return 1;
}

int
ponder(string str)
{
    object *oblist;

    if (!stringp(str))
    {
	SOULDESC("pondering the situation");
	write("You ponder the situation.\n");
	all(" ponders the situation.");
	return 1;
    }

    if ((strlen(str) > 60) &&
	(!(this_player()->query_wiz_level())))
    {
	SOULDESC("pondering the situation");
	write("You ponder beyond the end of the line and wake up from " +
	    "your reveries.\n");
	all(" ponders the situation.");
	return 1;
    }

    oblist = parse_this(str, "[about] [the] [proposal] [of] [the] %l");

    if (!sizeof(oblist))
    {
	SOULDESC("pondering about something");
	write("You ponder " + str + "\n");
	all(" ponders " + str);
	return 1;
    }

    SOULDESC("pondering about a proposal");
    actor("You ponder about the proposal of", oblist);
    all2act("ponders about the proposal of", oblist);
    target(" ponders about your proposal.", oblist);
    return 1;    
}

int
rsay(string str)
{
    int     index;
    int     size;
    object *oblist;
    string  race = this_player()->query_race_name();
    string  pos = this_player()->query_possessive();
    int     skill;
    string  *words;
    int     sentence_size;
    int     sentence_index;
    string  to_print;

    if (!objectp(environment(this_player())))
	return 0;

    if (!stringp(str))
    {
	notify_fail("Say what in your racial tongue?\n");
	return 0;
    }

    oblist = FILTER_OTHER_LIVE(all_inventory(environment(this_player())));
    words = explode(str, " ") - ({ "" });
    sentence_size = sizeof(words);

    index = -1;
    size = sizeof(oblist);
    while(++index < size)
    {
	if ((race == oblist[index]->query_race_name()) ||
	    (oblist[index]->query_wiz_level()) ||
	    ((skill = oblist[index]->query_skill(SS_LANGUAGE)) >=
		LANGUAGE_ALL_RSAY))
	{
	    tell_object(oblist[index],
		this_player()->query_The_name(oblist[index]) + " says in " +
		pos + " own tongue: " + str + "\n");
	    break;
	}

	if (skill < LANGUAGE_MIN_RSAY)
	{
	    tell_object(oblist[index],
		this_player()->query_The_name(oblist[index]) +
		" says something completely incomprehensible.\n");
	    break;
	}

	skill -= LANGUAGE_MIN_RSAY;
	to_print = "";
	sentence_index = -1;
	while(++sentence_index < sentence_size)
	{
	    if (random(LANGUAGE_ALL_RSAY - LANGUAGE_MIN_RSAY) >= skill)
	    {
		to_print += " " + words[sentence_index];
	    }
	    else
	    {
		to_print += (" " +
		    extract("....................", 1,
			    strlen(words[sentence_index])));
	    }
	}

	tell_object(oblist[index],
	    this_player()->query_The_name(oblist[index]) +
	    " says in " + pos + " own tongue:" + to_print + "\n");
    }

    if (this_player()->query_option(OPT_ECHO))
	write("You rsay: " + str + "\n");
    else
	write("Ok.\n");

    return 1;
}

int
scratch(string str)
{
    object *oblist;
    string *zones;
    string one, two;

    zones = ({ "head", "chin", "back", "behind", "nose" });

    if (!stringp(str))
    {
	str = "head";
    }

    if (member_array(str, zones) != -1)
    {
	write("You scratch your " + str + ".\n");
	allbb(" scratches " + this_player()->query_possessive() +
	      " " + str + ".");
	return 1;
    }

    notify_fail("Scratch [whom] where?\n");
    if (sscanf(str, "%s %s", one, two) == 2)
    {
	if (member_array(two, zones) == -1)
	{
	    return 0;
	}
    }

    if (!stringp(two))
    {
	two = "head";
    }

    oblist = parse_this(one, "[the] %l [at] [the]");

    if (!sizeof(oblist))
    {
	return 0;
    }

    actor("You scratch", oblist, "'s " + two + ".");
    all2act("scratches", oblist, "'s " + two + ".");
    target(" scratches your " + two + ".", oblist);
    return 1;
}

int
tackle(string str)
{
    object *oblist;

    oblist = parse_this(str, "[the] %l");

    if (!sizeof(oblist))
    {
	notify_fail("Tackle whom?\n");
	return 0;
    }

    if (random(7))
    {
	actor("You tackle", oblist);
	all2act("tackles", oblist, ". " +
	    ((sizeof(oblist) > 1) ? "They fall": "The latter falls") +
	    " to the ground in a very unflattering way.");
	target(" comes running at you. " +
	    capitalize(this_player()->query_pronoun()) +
	    " attempts to tackle you and succeeds. You fall to the ground " +
	    "in a very unflattering way.", oblist);
    }
    else
    {
	actor("You try to tackle", oblist, " but fall flat on your face.");
	all2act("tries to tackle", oblist, " but misses and falls flat on " +
	    this_player()->query_possessive() + " face.");
	target(" comes running at you. " +
	    capitalize(this_player()->query_pronoun()) +
	    " attempts to tackle you but misses and falls flat on " +
	    this_player()->query_possessive() + " face.", oblist);
    }

    return 1;
}

int
think(string str)
{
    if (!stringp(str))
    {
	write("You try to look thoughtful but fail.\n");
	allbb(" tries to look thoughtful but fails.");
	SOULDESC("trying to look thoughtful");
	return 1;
    }

    if ((strlen(str) > 60) &&
	(!(this_player()->query_wiz_level())))
    {
	write("Geez.. That is a lot to think about at the same time.\n");
	allbb(" looks like " + this_player()->query_pronoun() +
	    " is trying to think hard about a lot of things.");
	SOULDESC("thinking about a lot of things");
	return 1;
    }

    write("You think hard about " + str + "\n");
    allbb(" looks like " + this_player()->query_pronoun() +
	" is thinking hard about " + str);
    SOULDESC("thinking hard about something");
    return 1;
}

int
tickle(string str)
{
    object *oblist;
    string *how;
    string location;
    int    i;

    oblist = parse_this(str, "[the] %l [in] [under] [the] " +
	"'feet' / 'foot' / 'chin' / 'abdomen' / 'belly' / 'side'");
    if (sizeof(oblist))
    {
    	how = explode(str, " ");
	switch(location = how[sizeof(how) - 1])
	{
	case "chin":
	    oblist->add_prop(LIVE_S_SOULEXTRA, "smiling sweetly");
	    target(" tickles you under your chin. " +
		"You smile sweetly in return.", oblist);
	    actor("You tickle", oblist, " under the chin. " +
		((sizeof(oblist) == 1) ?
		    (capitalize(oblist[0]->query_pronoun()) + " smiles") :
		    "They smile") + " sweetly in return.");
	    all2act("tickles", oblist, " under the chin. " +
		((sizeof(oblist) == 1) ? "The latter smiles" :
		    "They smile") + " sweetly in return.");
	    return 1;

	case "feet":
	case "foot":
	    oblist->add_prop(LIVE_S_SOULEXTRA, "laughing uncontrollably");
	    target(" tickles you under your " + location + ". " +
		"You fall down, laughing uncontrollably.", oblist);
	    actor("You tickle", oblist, " under the " + location + ". " +
		((sizeof(oblist) == 1) ?
		    (capitalize(oblist[0]->query_pronoun()) + " falls") :
		    "They fall") + " down, laughing uncontrollably.");
	    all2act("tickles", oblist, " under the " + location + ". " +
		((sizeof(oblist) == 1) ? "The latter falls" :
		    "They fall") + " down, laughing uncontrollably.");
	    return 1;

	case "abdomen":
	case "belly":
	case "side":
	    oblist->add_prop(LIVE_S_SOULEXTRA, "giggling merrily");
	    target(" tickles you in your " + location + ". " +
		"You start giggling merrily.", oblist);
	    actor("You tickle", oblist, " in the " + location + ". " +
		((sizeof(oblist) == 1) ?
		    (capitalize(oblist[0]->query_pronoun()) + " starts") :
		    "They start") + " giggling merrily.");
	    all2act("tickles", oblist, " in the " + location + ". " +
		((sizeof(oblist) == 1) ? "The latter starts" :
		    "They start") + " giggling merrily.");
	    return 1;

	default:
	    notify_fail("Tickle whom [where / how]? Rather... this should " +
		"not happen. Please make a sysbugreport about this.\n");
	    return 0;
	}

	return 1;
    }

    how = parse_adverb_with_space(str, "playfully", 1);

    oblist = parse_this(how[0], "[the] %l");

    if (!sizeof(oblist))
    {
	notify_fail("Tickle whom [how / where]?\n");
	return 0;
    }

    oblist->add_prop(LIVE_S_SOULEXTRA, "laughing");
    actor("You tickle", oblist, how[1] + ". " +
	((sizeof(oblist) == 1) ?
	    (capitalize(oblist[0]->query_pronoun()) + " falls") :
	    "They fall") + " down laughing, rolling over in an attempt to " +
	    "evade your tickling fingers.");
    all2act("tickles", oblist, how[1] + ". " +
	((sizeof(oblist) == 1) ? "The latter falls" :
	    "They fall") + " down laughing, rolling over in an attempt to " +
	    "evade the tickling fingers.", how[1]);
    target(" tickles you" + how[1] + ". You fall down laughing and roll " +
	"over in an attempt to evade those tickling fingers.", oblist,
	how[1]);

    return 1;
}
*/