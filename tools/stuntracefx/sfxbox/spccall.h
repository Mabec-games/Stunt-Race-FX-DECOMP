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

/* Lockstep (A7); spc_lockstep_prepare runs before the fork. */
int spc_lockstep_prepare(void *core);
void spc_lockstep_role(int role, FILE *report, volatile int *stuck);
void spc_before_frame(long f);
int spc_after_frame(long f, char *buf, size_t n);  /* the hybrid: 1 if the frame's SPC700 calls differ */
void spc_lockstep_done(void);
void spc_lockstep_state_loaded(void);
void spc_lockstep_past_end(void);
int spc_lockstep_summary(char *buf, size_t n);
void spc_timing_line(FILE *report);   /* the `spc timing:` line */
/* SPC700 unit calls: spccall/spcset script commands. */
int spc_unit_init(void *core);
void spc_unit_set(uint16_t at, uint8_t v);
void spc_unit_call(uint16_t target, uint16_t at, const int *has, const uint8_t *reg);
int spc_unit_busy(void);

#endif
