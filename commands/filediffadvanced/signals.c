#include "signals.h"
#include <signal.h>
#include <string.h>
#include <unistd.h>

volatile sig_atomic_t g_interrupted = 0;
volatile sig_atomic_t g_progress_request = 0;

static void handle_interrupt(int sig) {
    (void)sig;
    g_interrupted = 1;
    const char msg[] = "\nComparison interrupted.\n";
    write(STDERR_FILENO, msg, sizeof(msg) - 1);
}

static void handle_progress(int sig) {
    (void)sig;
    g_progress_request = 1;
}

void setup_signals(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handle_interrupt;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    sa.sa_handler = handle_progress;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sa, NULL);
}
