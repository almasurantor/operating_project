#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>

extern volatile sig_atomic_t g_interrupted;
extern volatile sig_atomic_t g_progress_request;

void setup_signals(void);

#endif
