/*
 * /d/Genesis/obj/board.c
 *
 * This object is the common board that should contain notes that are
 * important to all players in Genesis. In order to get it, just clone
 * this object into the room.
 *
 * Please note that this object is not meant to be inherited by a fancy
 * object you make. This common board is provided on a take it or leave
 * it basis. Boards are very sensitive and we would not like to have
 * problems with it.
 *
 * If you are going to make a board reader or something like that,
 * the notes are kept in /d/Genesis/log/com_board_data
 *
 * /Mercade, 23 March 1994
 *
 * Revision history:
 */

#pragma no_inherit   /* no messing with this board */
#pragma save_binary  /* quick loading              */
#pragma strict_types /* you should be tidy         */

inherit "/std/object";

#include <stdproperties.h>

#define COMMON "/d/Standard/obj/common_master"

/*
 * Function name: create_object
 * Description  : This function is called to create the board.
 */
nomask void
create_object()
{
    set_name("board");
    add_name("bulletinboard");
    set_adj("common");
    add_adj("bulletin");
    set_short("common bulletin board");
    add_prop(OBJ_M_NO_GET,  "It's firmly secured to the ground.\n");
    add_prop(OBJ_M_NO_SELL, "No-one is interested in such a thing.\n");
    add_prop(OBJ_I_VOLUME,  50000);
    add_prop(OBJ_I_WEIGHT,  60000);
    add_prop(OBJ_I_VALUE,     250);

    seteuid(getuid());
    COMMON->add_cloned_board(this_object());
}

nomask varargs string
long()
{
    return COMMON->long();
}

nomask void
init()
{
    add_action("new_msg", "note");
    add_action("read_msg", "read");
    add_action("read_msg", "mread");
    add_action("remove_msg", "remove");
    add_action("store_msg", "store");
}

nomask varargs int
new_msg(string new_head)
{
    return COMMON->new_msg(new_head);
}

nomask int
read_msg(string what_msg, int mr)
{
    return COMMON->read_msg(what_msg, mr);
}

nomask int
remove_msg(int what_msg)
{
    return COMMON->remove_note(what_msg);
}

nomask int
store_msg(string str)
{
    write(break_string("Sorry. In order to store a message you shall have " +
        "to go to the common board on the wizards island and do it from " +
        "the room there.", 75) + "\n");
    return 1;

/*
 * store_msg() in /std/board.c is declared static, so I cannot call it
 * from another board.
 *
    return COMMON->store_msg(str);
 */
}

/*
 * Function name: query_prevent_shadow
 * Description  : This function is to make sure that this object is never
 *                shadowed.
 * Returns      : 1 - always.
 */
nomask int
query_prevent_shadow()
{
    return 1;
}
