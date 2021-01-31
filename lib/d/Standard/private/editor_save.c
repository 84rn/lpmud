inherit "/std/room.c";

#pragma no_inherit
#pragma strict_types
#pragma save_binary

#define SAVE "/d/Standard/private/editor_save/"

public void
create_room()
{
    set_short("Magazyn cudzej korespondencji");
    set_long("W to miejsce trafiaja wszystkie zblakane listy, ktore nie " +
        "mogly odnalezc swego wlasciciela.\n");
}

public void
linkdie(string str)
{
    string file;
    
    if (file_name(previous_object())[0..9] != "/obj/edit#")
        return ;
        
    file = SAVE + this_player()->query_real_name();
    
    setuid(); seteuid(getuid());
    
    rm(file);
    write_file(file, str);
    
    return ;
}

public string
restore()
{
    string file, str;
    
    if (file_name(previous_object())[0..9] != "/obj/edit#")
        return 0;
        
    file = SAVE + this_player()->query_real_name();
    
    setuid(); seteuid(getuid());

    str = read_file(file);
    
    return ((str) ? str : "");
}
