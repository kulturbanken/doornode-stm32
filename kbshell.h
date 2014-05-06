#ifndef KBSHELL_H
#define KBSHELL_H

#include "shell.h"

void kb_shell_check_running(void);
BaseSequentialStream *kb_shell_get_stream(void);
void kb_shell_init(void);

#endif/*KBSHELL_H*/
