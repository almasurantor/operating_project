#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_INPUT 1024
#define MAX_ARGS  64

static const char *commands[] = {
    "timedexec", "loganalyzer", "filecrypt", "filediffadvanced", NULL
};

static volatile sig_atomic_t child_pid_global = 0;

static void handle_sigint(int sig) {
    (void)sig;
    if (child_pid_global > 0)
        kill(child_pid_global, SIGINT);
    else
        write(STDOUT_FILENO, "\ncustomshell> ", 14);
}

static int is_valid_command(const char *cmd) {
    for (int i = 0; commands[i]; i++) {
        if (strcmp(cmd, commands[i]) == 0)
            return 1;
    }
    return 0;
}

static void print_help(void) {
    printf("\nCustom Shell - Operating Systems I Group Project\n");
    printf("================================================\n\n");
    printf("Available commands:\n");
    printf("  timedexec          Run a program with CPU/memory limits\n");
    printf("  loganalyzer        Parse log files for statistics using mmap\n");
    printf("  filecrypt          Encrypt/decrypt files with Vigenere cipher\n");
    printf("  filediffadvanced   Compare files with performance metrics\n\n");
    printf("Built-in commands:\n");
    printf("  help               Show this help message\n");
    printf("  exit / quit        Exit the shell\n\n");
    printf("Usage: type a command name followed by its arguments.\n");
    printf("Example: filediffadvanced -t -m file1.txt file2.txt\n\n");
}

static int parse_input(char *input, char **args) {
    int count = 0;
    char *p = input;

    while (*p && count < MAX_ARGS - 1) {
        while (*p == ' ' || *p == '\t' || *p == '\n') p++;
        if (*p == '\0') break;

        if (*p == '"') {
            p++;
            args[count++] = p;
            while (*p && *p != '"') p++;
            if (*p == '"') *p++ = '\0';
        } else if (*p == '\'') {
            p++;
            args[count++] = p;
            while (*p && *p != '\'') p++;
            if (*p == '\'') *p++ = '\0';
        } else {
            args[count++] = p;
            while (*p && *p != ' ' && *p != '\t' && *p != '\n') p++;
            if (*p) *p++ = '\0';
        }
    }

    args[count] = NULL;
    return count;
}

static char *resolve_command_path(const char *cmd) {
    static char path[512];
    snprintf(path, sizeof(path), "./commands/%s/%s", cmd, cmd);
    if (access(path, X_OK) == 0)
        return path;
    return NULL;
}

static int run_command(int argc __attribute__((unused)), char **args) {
    if (!is_valid_command(args[0])) {
        fprintf(stderr, "customshell: unknown command '%s'\n", args[0]);
        fprintf(stderr, "Type 'help' for available commands.\n");
        return 127;
    }

    char *path = resolve_command_path(args[0]);
    if (!path) {
        fprintf(stderr, "customshell: '%s' not built. Run 'make' first.\n", args[0]);
        return 126;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 2;
    }

    if (pid == 0) {
        args[0] = path;
        execv(path, args);
        perror("exec");
        _exit(127);
    }

    child_pid_global = pid;
    int status;
    waitpid(pid, &status, 0);
    child_pid_global = 0;

    if (WIFEXITED(status))
        return WEXITSTATUS(status);
    if (WIFSIGNALED(status)) {
        fprintf(stderr, "Command killed by signal %d\n", WTERMSIG(status));
        return 128 + WTERMSIG(status);
    }
    return 1;
}

static int interactive_mode(void) {
    char input[MAX_INPUT];
    char *args[MAX_ARGS];

    printf("Custom Shell v1.0\n");
    printf("Type 'help' for available commands, 'exit' to quit.\n\n");

    while (1) {
        printf("customshell> ");
        fflush(stdout);

        if (!fgets(input, sizeof(input), stdin)) {
            printf("\n");
            break;
        }

        int argc = parse_input(input, args);
        if (argc == 0)
            continue;

        if (strcmp(args[0], "exit") == 0 || strcmp(args[0], "quit") == 0)
            break;

        if (strcmp(args[0], "help") == 0) {
            print_help();
            continue;
        }

        run_command(argc, args);
    }

    return 0;
}

int main(int argc, char *argv[]) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGINT, &sa, NULL);

    if (argc < 2)
        return interactive_mode();

    return run_command(argc - 1, &argv[1]);
}
