#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#ifdef VAX_BSD
char vax_bsd_toupper(char s);

#define xtoupper(s) vax_bsd_toupper(s)
#else
#define	xtoupper(s) toupper(s)
#endif /* VAX_BSD */

int
main(int argc, char **argv)
{
    int i = 0, j;
    char name[160];
    char *func_name;
    char *prefix;
    char *fname;
    FILE *header_file, *data_file;
    if (argc < 3)
	exit(1);

    prefix = argv[1];
    fname = argv[2];
    for (j = 0; prefix[j]; j++)
	prefix[j] = xtoupper(prefix[j]);

    (void)sprintf(name, "%s.h", fname);
    header_file = fopen(name, "w");
    (void)sprintf(name, "%s.t", fname);
    data_file = fopen(name, "w");

    (void)fprintf(data_file, "static struct fkntab %s_fkntab[] =\n{\n", fname);

    for ((void)fgets(name, sizeof(name), stdin);
	 !feof(stdin);
	 (void)fgets(name, sizeof(name), stdin))
    {
	if ((func_name = strchr(name, '\n')) != NULL)
	    *func_name = '\0';
	if (!*name || *name == '#')
	    continue;
	func_name = name;
	for (j = 0; name[j]; j++)
	{
	    if (name[j] == '\t' ||
		name[j] == ' ')
	    {
		name[j] = '\0';
		j++;
		while (name[j] || name[j] == ' ' || name[j] == '\t')
		    j++;
		if (name[j])
		    func_name = &name[j];
		break;
	    }
	}
	
	(void)fprintf(data_file, "    {\"%s\", 0, 0},\n", func_name);
	for (j = 0; name[j]; j++)
	    name[j] = xtoupper(name[j]);
	(void)fprintf(header_file, "#define %s_%s %d\n", prefix, name, i++);
    }

    (void)fputs("    {NULL, 0, 0}\n};\n", data_file);
    exit(0);
    /* NOTREACHED */
}

#ifdef VAX_BSD
char
vax_bsd_toupper(char s)
{
    if (s == '_')
	return s;
    else
	return toupper(s);
}
#endif /* VAX_BSD */
