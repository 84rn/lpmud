inherit "/std/object";

#include <macros.h>
#include <std.h>
#include <stdproperties.h>
#include <filepath.h>
#include "/d/Standard/login/login.h"
#include "smierci.h"


#pragma strict_types


private	mapping	smierci = ([]);

public int dorzuc_smierc(string str);


public void
create_object()
{
    setuid();
    seteuid(getuid());
    
    ustaw_nazwe(({ "kosc", "kosci", "kosci", "kosc", "koscia", "kosci" }),
        ({ "kosci", "kosci", "kosciom", "kosci", "koscmi", "kosciach" }),
        PL_ZENSKI);
        
    dodaj_przym("dlugi", "dludzy");
    dodaj_przym("pozolkly", "pozolkli");
    
    set_long("Sadzac po ksztalcie jest to ludzka kosc udowa. " +
	"Stanowi ona magiczne medium, pozwalajace dowolnie kreowac " +
	"zaswiaty wedle wlasnej woli. Sluzy do tego komenda " +
	"'dodaj'.\n");
	
    add_prop(OBJ_I_WEIGHT, 1250);
    add_prop(OBJ_I_VOLUME, 930);
    add_prop(OBJ_I_VALUE, 0);
}

public void
init()
{
    add_action(&dorzuc_smierc(), "dodaj");
}

public int
dorzuc_smierc(string str)
{
    string tekst, plik, rasa;
    mixed *zawartosc;
    int ix, size;

    if (!strlen(str))
    {
	notify_fail("Skladnia: dodaj <plik> <rasa>.\n");
	return 0;
    }
    
    zawartosc = explode(str, " ");
    if (sizeof(zawartosc) != 2)
    {
	notify_fail("Skladnia: dodaj <plik> <rasa>.\n");
	return 0;
    }
    
    if (SECURITY->query_wiz_rank(this_player()->query_real_name()) < WIZ_ARCH)
    {
	write("Tylko czlonkowie administracji moga dodawac nowe smierci.\n");
	return 1;
    }
    
    plik = zawartosc[0]; rasa = lower_case(zawartosc[1]);
    
    if (member_array(rasa, RACES) == -1)
    {
	write(sprintf("Nie ma takiej rasy: '%s'.\n", rasa));
	return 1;
    }

    plik = FTPATH(this_player()->query_path(), plik);
    tekst = read_file(plik);
    if (!tekst)
    {
	write(sprintf("Nie ma takiego pliku: '%s'.\n", plik));
	return 1;
    }
    
    zawartosc = explode(tekst, "\n\n");
    ix = -1; size = sizeof(zawartosc);
    while(++ix < size)
    {
	if (strlen(zawartosc[ix]))
	    zawartosc[ix] = implode(explode(zawartosc[ix], "\n"), " ");
    }
    
    restore_object(SMIERCI_SAVE);
    
    if (!mappingp(smierci))
	smierci = ([ ]);
	
    if (smierci[rasa])
	smierci[rasa] += ({ zawartosc });
    else
	smierci[rasa] = ({ zawartosc });
	
    save_object(SMIERCI_SAVE);

    write("Ok.\n");
    return 1;
}
