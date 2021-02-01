#ifndef _simulate_h_
#define _simulate_h_
#include "object.h"

void update_actions(struct object *aob);
char *get_gamedriver_info(char *str);
void fatal(char *fmt, ...);
void error(char *fmt, ...);

#endif
