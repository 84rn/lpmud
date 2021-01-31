/* (c) Copyright by Anders Chrigstroem 1993, All rights reserved */
/* Permission is granted to use this source code and any executables
 * created from this source code as part of the CD Gamedriver as long
 * as it is not used in any way whatsoever for monetary gain. */

#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

#include "config.h"

#ifdef BINARIES /* Skip this entire file if BINARIES not defined */
#include "lint.h"
#include "mstring.h"
#include "interpret.h"
#include "object.h"
#include "exec.h"
#include "swap.h"

FILE *crfile(char *);

#define MAGIC "CDBIN"

extern int current_time;
extern int driver_mtime;
extern int comp_flag;

FILE *crfile(name)
    char *name;
{
    char *ptr, *dir = string_copy(name);
    struct stat st;

    ptr = dir;
    for (ptr = strchr(ptr, '/'); ptr; ptr = strchr(ptr + 1, '/'))
    {
	*ptr = '\0';

	if (stat(dir, &st) == -1)
	{
	    if (mkdir(dir, 0774) == -1)
	    {
		free(dir);
		return (FILE *) 0;
	    }
	}
	else if ((st.st_mode & S_IFMT) != S_IFDIR)
	{
	    if (unlink(dir) == -1 ||
		mkdir(dir, 0774) == -1)
	    {
		free(dir);
		return (FILE *) 0;
	    }
	}

	*ptr = '/';
    }
    free(dir);

    return fopen(name, "w");
}

static void
calculate_offsets(struct program *dest, struct program *prog)
{
    int i, j;
    
    for (i = 0; segm_desc[i].sections; i++)
    {
	struct section_desc *sec;
	char *block;

	if (segm_desc[i].ptr_offset != -1)
	    block = *(char **)(((char *)prog) + segm_desc[i].ptr_offset);
	else
	    block = (char *)prog;
	sec = segm_desc[i].sections;

	for (j = 0; sec[j].section != -1; j++)
	{
	    if (sec[j].ptr_offset != -1)
	    {
		*(int *)(((char *)dest) + sec[j].ptr_offset) = 
		    *(char **)(((char *)prog) + sec[j].ptr_offset) - block;
	    }
	}
    }
}

static void
restore_sections(struct program *dest, struct program *prog)
{
    int i, j;
    
    for (i = 0; segm_desc[i].sections; i++)
    {
	struct section_desc *sec;
	char *block;

	if (segm_desc[i].ptr_offset != -1)
	    block = *(char **)(((char *)prog) + segm_desc[i].ptr_offset);
	else
	    block = (char *)dest;

	if (segm_desc[i].swap_idx_offset != -1)
	    *(int *)(((char *)dest) + segm_desc[i].swap_idx_offset) = 0;

	sec = segm_desc[i].sections;

	for (j = 0; sec[j].section != -1; j++)
	{
	    if (sec[j].ptr_offset != -1)
	    {
		*(char **)(((char *)dest) + sec[j].ptr_offset) = 
		    block + *(int *)(((char *)dest) + sec[j].ptr_offset);
	    }
	}
    }
}

void 
save_binary(prog)
struct program *prog;
{
    int i;
    extern int driver_mtime;
    char *binname;
    extern struct object *master_ob;
    extern char *add_slash(char *);
    FILE *f;
    size_t strsize = 0;
    struct program pgm;
    int strsize_tell;
    struct svalue *ret;

    if (master_ob)
    {
	push_mstring(add_slash(prog->name));
	ret = apply_master_ob(M_VALID_SAVE_BINARY, 1);
	if (ret && (ret->type != T_NUMBER || ret->u.number == 0))
	{
	    if (comp_flag)
		(void)fprintf(stderr, "BIN Save for %s not allowed!\n",
			      prog->name);
	    return;
	}
    }
    binname = (char *)alloca(strlen(BINARIES) + 
			     strlen(prog->name) + 1);
    (void)sprintf(binname, "%s%s", BINARIES, prog->name);
    
    if ((f = crfile(binname)) == NULL)
    {
	if (comp_flag)
	    (void)fprintf(stderr, "BIN Can't create file %s!\n",
			  binname);
	return;
    }

    if (fwrite(MAGIC, sizeof(char), strlen(MAGIC), f) != strlen(MAGIC)) {
	(void)fclose(f);
	(void)unlink(binname);
	if (comp_flag)
	    (void)fprintf(stderr, "BIN Error writing file %s!\n",
			  binname);
	return;
    }
    if (fwrite((char *)&driver_mtime, sizeof(driver_mtime), 1, f) != 1) {
	(void)fclose(f);
	(void)unlink(binname);
	if (comp_flag)
	    (void)fprintf(stderr, "BIN Error writing file %s!\n",
			  binname);
	return;
    }
    strsize_tell = (int)ftell(f);
    if (fwrite((char *)&strsize, sizeof(strsize), 1, f) != 1) {
	(void)fclose(f);
	(void)unlink(binname);
	if (comp_flag)
	    (void)fprintf(stderr, "BIN Error writing file %s!\n",
			  binname);
	return;
    }
    
#ifdef USE_SWAP
    load_lineno_from_swap(prog);
#endif

    /* Write out the different segments of the program */
    if (fwrite((char *)prog, (size_t)prog->total_size, 1, f) != 1) {
	(void)fclose(f);
	(void)unlink(binname);
	if (comp_flag)
	    (void)fprintf(stderr, "BIN Error writing file %s!\n",
			  binname);
	return;
    }
    if (fwrite((char *)prog->program, (size_t)prog->exec_size, 1, f) != 1) {
	(void)fclose(f);
	(void)unlink(binname);
	if (comp_flag)
	    (void)fprintf(stderr, "BIN Error writing file %s!\n",
			  binname);
	return;
    }

    if (fwrite((char *)prog->line_numbers, (size_t)prog->debug_size, 1, f) != 1) {
	(void)fclose(f);
	(void)unlink(binname);
	if (comp_flag)
	    (void)fprintf(stderr, "BIN Error writing file %s!\n",
			  binname);
	return;
    }

    /* Write the inherit names, and the names of the
       inherited programs to the strtab */
    for (i = 0; i < (int)prog->num_inherited; i++)
    {
	char *str;
	struct program *iprog = prog->inherit[i].prog;
	char buff[100];
	
	str = iprog->name;
	if (fwrite(str, strlen(str) + 1, 1, f) != 1) {
	    (void)fclose(f);
	    (void)unlink(binname);
	    if (comp_flag)
		(void)fprintf(stderr, "BIN Error writing file %s!\n",
			      binname);
	    return;
	}
	strsize += strlen(str) + 1;
	(void)sprintf(buff, "%d", iprog->mod_time);
	if (fwrite(buff, strlen(buff) + 1, 1, f) != 1) {
	    (void)fclose(f);
	    (void)unlink(binname);
	    if (comp_flag)
		(void)fprintf(stderr, "BIN Error writing file %s!\n",
			      binname);
	    return;
	}
	strsize += strlen(buff) + 1;

	str = prog->inherit[i].name;
	if (fwrite(str, strlen(str) + 1, 1, f) != 1) {
	    (void)fclose(f);
	    (void)unlink(binname);
	    if (comp_flag)
		(void)fprintf(stderr, "BIN Error writing file %s!\n",
			      binname);
	    return;
	}
	strsize += strlen(str) + 1;
	
    }
    
    
    /* Write the function names to the strtab */
    for (i = 0; i < (int)prog->num_functions; i++)
    {
	char *str = prog->functions[i].name;
	
	if (fwrite(str, strlen(str) + 1, 1, f) != 1) {
	    (void)fclose(f);
	    (void)unlink(binname);
	    if (comp_flag)
		(void)fprintf(stderr, "BIN Error writing file %s!\n",
			      binname);
	    return;
	}
	strsize += strlen(str) + 1;
    }
    
    /* Write the variable names to the strtab */
    for (i = 0; i < (int)prog->num_variables; i++)
    {
	char *str = prog->variable_names[i].name;
	
	if (fwrite(str, strlen(str) + 1, 1, f) != 1) {
	    (void)fclose(f);
	    (void)unlink(binname);
	    if (comp_flag)
		(void)fprintf(stderr, "BIN Error writing file %s!\n",
			      binname);
	    return;
	}
	strsize += strlen(str) + 1;
    }
    
    if (!strsize)
    {
	if (fwrite((char *)&strsize, sizeof(strsize), 1, f) != 1) {
	    (void)fclose(f);
	    (void)unlink(binname);
	    if (comp_flag)
		(void)fprintf(stderr, "BIN Error writing file %s!\n",
			      binname);
	    return;
	}
	strsize = sizeof(strsize);
    }

    /* Write the size of the extra stringtable */
    if (fseek(f, (long)strsize_tell, SEEK_SET) != 0) {
	(void)fclose(f);
	(void)unlink(binname);
	if (comp_flag)
	    (void)fprintf(stderr, "BIN Error writing file %s!\n",
			  binname);
	return;
    }
    if (fwrite((char *)&strsize, sizeof(strsize), 1, f) != 1) {
	(void)fclose(f);
	(void)unlink(binname);
	if (comp_flag)
	    (void)fprintf(stderr, "BIN Error writing file %s!\n",
			  binname);
	return;
    }
    
    /* Write a new program header with offsets instead of
       pointers for the sections */
    pgm = *prog;
    calculate_offsets(&pgm, prog);
    if (fwrite((char *)&pgm, sizeof(pgm), 1, f) != 1) {
	(void)fclose(f);
	(void)unlink(binname);
	if (comp_flag)
	    (void)fprintf(stderr, "BIN Error writing file %s!\n",
			  binname);
	return;
    }

#ifdef USE_SWAP
    swap_lineno(prog);
#endif
    
    if (fclose(f) != 0) {
	(void)unlink(binname);
	if (comp_flag)
	    (void)fprintf(stderr, "BIN Error writing file %s!\n",
			  binname);
	return;
    }
    if (comp_flag)
	(void)fprintf(stderr, "BIN %s saved!\n",
		      binname);
}

int
load_binary(FILE *f, char *lname)
{
    extern int current_id_number;
    struct program pgm;
    struct program *progp;
    extern struct program *prog;
    extern char *inherit_file;
    extern int driver_mtime;
    int comp_driver_mtime;
    char buf[100];
    char *strtab, *strptr;
    size_t strsize;
    int i;
    extern int total_prog_block_size, total_program_size,
    total_num_prog_blocks;
    int pgm_seek;
    extern void hash_func (int size, struct function *from,
			   struct function_hash *to);
    if (inherit_file)
    {
	free(inherit_file);
	inherit_file	= (char *)0;
    }

    /* Is the file compatible with the current driver? */
    (void)fread(buf, sizeof(char), strlen(MAGIC), f);
    (void)fread((char *)&comp_driver_mtime, sizeof(comp_driver_mtime), 1, f);
    if (strncmp(buf, MAGIC, strlen(MAGIC)) ||
	comp_driver_mtime != driver_mtime)
    {
	(void)fclose(f);
	if (comp_flag)
	    (void)fprintf(stderr, "BIN Wrong magic for %s!\n",
			  lname);
	
	prog = 0;
	return -1;
    }

    /* Read strtab size and program header */
    (void)fread((char *)&strsize, sizeof(strsize), 1, f);
    if (!strsize)
    {
	prog = 0;
	return -1;
    }

    pgm_seek = (int)ftell(f);
    (void)fread((char *)&pgm, sizeof(pgm), 1 ,f);

    /* Allocate segments */
    progp = (struct program *)xalloc((size_t)pgm.total_size);
    pgm.program = xalloc((size_t)pgm.exec_size);
    pgm.line_numbers = xalloc((size_t)pgm.debug_size);

    /* Read segments */
    (void)fseek(f, (long)pgm_seek, SEEK_SET);
    (void)fread((char *)progp, (size_t)pgm.total_size, 1, f);
    (void)fread(pgm.program, (size_t)pgm.exec_size, 1, f);
    (void)fread(pgm.line_numbers, (size_t)pgm.debug_size, 1, f);
    
    /* Calculate pointers for sections from the offsets in the header */
    restore_sections(progp, &pgm);

    /* Read the extra strtab */
    strptr = strtab = xalloc(strsize);
    (void)fread(strtab, strsize, 1, f);

    /* Done reading from the file! */
    (void)fclose(f);
    
    /* Now comes an check of the ages of all INCLUDED files: */
    if (progp->sizeof_include_files) 
    {
	struct stat stb;
	int age;
	char *t, *s;

	t = progp->include_files;
	while(t < progp->include_files + progp->sizeof_include_files) 
	{
	    age = atoi(t);
	    s = strchr(t, ':');
	    if (!s)
	    {
		if (comp_flag)
		    (void)fprintf(stderr,
				  "BIN Can not separate age & iname %s->%s!\n",
				  lname, t);
		/* Free allocated memory */
		free(pgm.program);
		free((char *)pgm.line_numbers);
		free((char *)progp);
		free(strtab);
		prog = 0;
		return -1;
	    }
	    else
		t = s + 1;
	    if (stat(t, &stb) == -1) 
	    {
		if (comp_flag)
		    (void)fprintf(stderr,
				  "BIN Included file not found (%s->%s)\n",
				  lname, t);
		/* Free allocated memory */
		free(pgm.program);
		free((char *)pgm.line_numbers);
		free((char *)progp);
		free(strtab);
		prog = 0;
		return -1;
	    }
	    if (stb.st_mtime != age)
	    {
		if (comp_flag)
		    (void)fprintf(stderr,"BIN Included file changed (%s->%s)\n",
				  lname, t);
		/* Free allocated memory */
		free(pgm.program);
		free((char *)pgm.line_numbers);
		free((char *)progp);
		free(strtab);
		prog = 0;
		return -1;
	    }
	    t += strlen(t) + 1;
	}
    }

    /* Find the inherited programs */
    for (i = 0; i < (int)progp->num_inherited - 1; i++)
    {
	extern struct object *find_object2(char *);
	struct object *ob;
	int mod_time;
	
	ob = find_object2(strptr);
	if (!ob)
	{
	    inherit_file = string_copy(strptr);
	    for (i--; i >= 0; i--)
		free_sstring(progp->inherit[i].name);
	    if (comp_flag)
		(void)fprintf(stderr,"BIN Inherited file not loaded (%s->%s)\n",
			      lname, inherit_file);
	    /* Free allocated memory */
	    free(pgm.program);
	    free((char *)pgm.line_numbers);
	    free((char *)progp);
	    free(strtab);
	    prog = 0;
	    return 0;
	}
	strptr += strlen(strptr) + 1;
	mod_time = atoi(strptr);
	if (mod_time != ob->prog->mod_time)
	{
	    /* Free allocated memory */
	    for (i--; i >= 0; i--)
		free_sstring(progp->inherit[i].name);
	    free(pgm.program);
	    free((char *)pgm.line_numbers);
	    free((char *)progp);
	    free(strtab);
	    if (comp_flag)
		(void)fprintf(stderr,"BIN Inherited file changed (%s(%d)->%s(%d))\n",
			      lname, mod_time, ob->prog->name, mod_time);
	    prog = 0;
	    return -1;
	}
	strptr += strlen(strptr) + 1;
	progp->inherit[i].prog = ob->prog;
	progp->inherit[i].name = make_sstring(strptr);
	strptr += strlen(strptr) + 1;
    }

    /* Fix the inherit self */
    /* Check that the name is correct */
    if (strcmp(lname, strptr))
    {
	if (comp_flag)
	    (void)fprintf(stderr,"BIN Wrong file name (%s->%s)\n",
			  lname, strptr);
        /* Free allocated memory */
	for (i--; i >= 0; i--)
	    free_sstring(progp->inherit[i].name);
	free(pgm.program);
	free((char *)pgm.line_numbers);
	free((char *)progp);
	free(strtab);
	prog = 0;
	return -1;
    }
    progp->name = string_copy(strptr);
    strptr += strlen(strptr) + 1;
    strptr += strlen(strptr) + 1; /* skip mod_time */
    progp->inherit[i].name = make_sstring(strptr);
    strptr += strlen(strptr) + 1; 
    progp->inherit[i].prog = progp;
    
    
    /* Read the function names */
    for (i = 0; i < (int)progp->num_functions; i++)
    {
	progp->functions[i].name =
	    make_sstring(strptr);
	strptr += strlen(strptr) + 1; 
#ifdef PROFILE_FUNS
	progp->functions[i].num_calls = 0;
	progp->functions[i].time_spent = 0;
#ifdef SOLARIS
	progp->functions[i].ticks_call = 0;
#else
	progp->functions[i].stime_call = 0;
	progp->functions[i].utime_call = 0;
#endif
#endif
    }

    /* Read the variable name */
    for (i = 0; i < (int)progp->num_variables; i++)
    {
	progp->variable_names[i].name =
	    make_sstring(strptr);
	strptr += strlen(strptr) + 1;
    }

    free(strtab);

    /* Done! */

    hash_func(progp->num_functions, progp->functions, progp->func_hash);

    progp->ref = 1;
    progp->swap_num = 0;
    progp->id_number = current_id_number++;
#if defined(RUSAGE) && defined(PROFILE_OBJS)
    progp->cpu = 0;
#endif
    progp->swap_lineno_index = 0;
    progp->load_time = current_time;

    for(i = 0; i < (int)progp->num_inherited - 1; i++)
	reference_prog(progp->inherit[i].prog, "inheritance");
    
    total_prog_block_size += progp->total_size;
    total_program_size += progp->exec_size;
    
    total_num_prog_blocks += 1;
    register_program(progp);
    progp->load_time = current_time;
#ifdef USE_SWAP
    swap_lineno(progp);
#endif
    
    prog = progp;
    return 0;
}
#endif
