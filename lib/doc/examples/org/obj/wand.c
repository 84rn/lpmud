
inherit "/std/object";

#include <stdproperties.h>

void
create_wand()
{
	set_name("wand");
	set_pname("wands");
}

nomask void
create_object()
{
	add_prop(OBJ_I_LIGHT, 0);
	add_prop(OBJ_I_WEIGHT, 700);
	add_prop(OBJ_I_VOLUME, 2000);
	add_prop(OBJ_I_VALUE, 7000);

	create_wand();
}

int zaswiec();
int zgas();

void
init()
{
	::init();

	add_action(zaswiec,"zaswiec");
	add_action(zgas,"zgas");
}


int
zaswiec()
{
	add_prop(OBJ_I_LIGHT, 4);
	write("Wlaczyles magiczna rozdzke.\n");
	return 1;
}

int
zgas()
{
	add_prop(OBJ_I_LIGHT, 0);
	write("Zgasiles magiczna rozdzke.\n");
	return 1;
}

