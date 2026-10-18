/* spccall.h: SPC700 routines in the per-call oracle; see spccall.c.
   check.c drives it. */
#ifndef SFXBOX_SPCCALL_H
#define SFXBOX_SPCCALL_H
#include <stdio.h>

/* Load the SPC700 routines the library has C for (those named by `only`,
   when given): how many, or -1 when it has some and the core has no SPC700
   hooks. */
int spc_load(void *core, void *lib, const char *only);
int spc_count(void);
void spc_state_loaded(void);
void spc_check_begin(FILE *report);
void spc_check_rows(FILE *report);   /* one CSV row per routine, as check.c's */
#endif
