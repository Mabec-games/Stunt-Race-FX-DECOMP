/* gsucall.h: Super FX routines in the per-call oracle and the lockstep
   run; see gsucall.c. check.c drives it. */
#ifndef SFXBOX_GSUCALL_H
#define SFXBOX_GSUCALL_H
#include <stdint.h>
#include <stdio.h>

/* Load the Super FX routines the library has C for (those named by `only`,
   when given): how many, or -1 when it has some and the core has no Super
   FX hooks. */
int gsu_load(void *core, void *lib, const char *only);
int gsu_count(void);

/* Per-call oracle (A5). */
void gsu_check_begin(FILE *report);
void gsu_check_foreign(uint32_t addr);  /* the S-CPU or a DMA wrote Super FX RAM */
void gsu_check_rows(FILE *report);      /* one CSV row per routine, as check.c's */

/* Lockstep (A7). gsu_lockstep_prepare runs before the fork. */
int gsu_lockstep_prepare(void);
void gsu_lockstep_role(int role, FILE *report, volatile int *stuck);
void gsu_before_frame(long f);
/* After frame f: appends what differs to buf (the hybrid side); 1 if anything does. */
int gsu_after_frame(long f, char *buf, size_t n);
void gsu_lockstep_done(void);           /* the original side has finished */
/* The hybrid's totals for the report line; 1 if any of them is a divergence. */
int gsu_lockstep_summary(char *buf, size_t n);
#endif
