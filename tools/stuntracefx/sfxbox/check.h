/* check.h: the per-call oracle (gate A5); see check.c. */
#ifndef SFXBOX_CHECK_H
#define SFXBOX_CHECK_H
#include <stdint.h>
int check_begin(void *core, const char *lib, const char *only, const char *report_path);
void check_end(void);
void check_unit_call(uint32_t resume_pc, uint16_t s);
void check_unit_entry(uint32_t pc, uint32_t ret, uint16_t exit_s, uint16_t entry_s);
void lockstep_unit_noreturn(uint32_t pc);
void lockstep_unit_entry(uint32_t pc, uint32_t ret, uint16_t exit_s, uint16_t entry_s);
int check_depth(void);
void check_rollback(int d);
int lockstep_begin(void *core, const char *lib, const char *only, const char *report_path);
int lockstep_role(void);
void lockstep_state_loaded(void);
int lockstep_call_ok(uint32_t pc);
uint32_t lockstep_clock(void);
void lockstep_debug_state(void);
long lockstep_unit_end(long f);
/* Hybrid side: the original's script has ended; report and stop. */
void lockstep_finish_now(void);
void lockstep_before_frame(long f);
void lockstep_after_frame(long f);
void lockstep_end(long frames);
#endif
