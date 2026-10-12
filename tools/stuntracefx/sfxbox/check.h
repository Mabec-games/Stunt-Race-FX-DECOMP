/* check.h: the per-call oracle (gate A5); see check.c. */
#ifndef SFXBOX_CHECK_H
#define SFXBOX_CHECK_H
int check_begin(void *core, const char *lib, const char *only, const char *report_path);
void check_end(void);
#endif
