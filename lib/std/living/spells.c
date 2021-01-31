/* 
 * /std/living/spells.c
 */

static object *spell_objs;	/* The list of spell objects */

void
spells_reset()
{
    spell_objs = ({});
}

/*
 * Function name: add_spellobj
 * Description:	  Add a spell object to the list of spell objects.
 * Arguments:	  obj - The object to add.
 */
void
add_spellobj(object obj)
{
    spell_objs += ({ obj });
}

/*
 * Function name: remove_spellobj
 * Description:   Remove a spell object from the list of spell objects
 * Arguments:     obj - the object to remove
 */
void
remove_spellobj(object obj)
{
    spell_objs -= ({ obj });
}

/*
 * Function name: query_spellobjs
 * Description:	  return the spellobj list.
 */
object *
query_spellobjs()
{
    spell_objs -= ({ 0 });

    return secure_var(spell_objs); 
}

#if 0
/*
 * Function name: query_magic_res
 * Description:   Return the total resistance of worn objects
 * Arguments:	  prop - The searched for property.
 */
public int 
query_magic_res(string prop)
{
    int no_objs, max, max_add, max_stat, i, *value;
    object *list;

    no_objs = (int)this_object()->query_prop(prop);

    if (!no_objs)
	return (int)this_object()->query_prop(PRE_OBJ_MAGIC_RES + prop);

    list = all_inventory(this_object());

    max_add = 100;
    for (i = 0; i < sizeof(list); i++)
    {
	value = (int *)list[i]->query_prop(prop);
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
#endif
