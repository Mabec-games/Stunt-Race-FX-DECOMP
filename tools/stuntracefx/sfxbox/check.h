/* check.h: the per-call oracle (gate A5); see check.c. */
#ifndef SFXBOX_CHECK_H
#define SFXBOX_CHECK_H
int check_begin(void *core, const char *lib, const char *only, const char *report_path);
void check_end(void);
int lockstep_begin(void *core, const char *lib, const char *only, const char *report_path);
int lockstep_role(void);
void lockstep_before_frame(long f);
void lockstep_after_frame(long f);
void lockstep_end(long frames);
#endif
