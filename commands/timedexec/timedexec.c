#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <signal.h>
#include <errno.h>
#include <string.h>

void print_usage(const char *prog_name) {
    printf("Usage: %s [-t cpu_time_seconds] [-m memory_limit_bytes] command [args...]\n", prog_name);
}

int main(int argc, char *argv[]) {
    int opt;
    rlim_t cpu_limit = RLIM_INFINITY;
    rlim_t mem_limit = RLIM_INFINITY;

    while ((opt = getopt(argc, argv, "t:m:")) != -1) {
        switch (opt) {
            case 't':
                cpu_limit = atoll(optarg); 
                break;
            case 'm':
                mem_limit = atoll(optarg);
                break;
            default:
                print_usage(argv[0]);
                return EXIT_FAILURE;
        }
    }

    if (optind >= argc) {
        fprintf(stderr, "Error: No command specified to execute.\n");
        print_usage(argv[0]);
        return EXIT_FAILURE;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork error");
        return EXIT_FAILURE;
    } else if (pid == 0) {

        // --- CHILD PROCESS ---
        
        if (cpu_limit != RLIM_INFINITY) {
            struct rlimit rl_cpu;
            rl_cpu.rlim_cur = cpu_limit; 
            rl_cpu.rlim_max = cpu_limit; 
            if (setrlimit(RLIMIT_CPU, &rl_cpu) != 0) {
                perror("setrlimit CPU error");
                exit(EXIT_FAILURE);
            }
        }

        if (mem_limit != RLIM_INFINITY) {
            struct rlimit rl_mem;
            rl_mem.rlim_cur = mem_limit;
            rl_mem.rlim_max = mem_limit;
            if (setrlimit(RLIMIT_AS, &rl_mem) != 0) {
                perror("setrlimit Memory error");
                exit(EXIT_FAILURE);
            }
        }

        execvp(argv[optind], &argv[optind]);
        
        perror("execvp error");
        exit(EXIT_FAILURE);
        
    } else {

        // --- PARENT PROCESS ---

        int status;
        
        if (waitpid(pid, &status, 0) < 0) {
            perror("waitpid error");
            return EXIT_FAILURE;
        }

        if (WIFEXITED(status)) {
            printf("[timedexec] Command exited normally with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            int sig = WTERMSIG(status);
            
            if (sig == SIGXCPU) {
                printf("\n[timedexec] KILLED: Exceeded CPU time limit of %d seconds.\n", (int)cpu_limit);
            } else {
                printf("\n[timedexec] TERMINATED by signal %d (%s).\n", sig, strsignal(sig));
                
                if (mem_limit != RLIM_INFINITY && (sig == SIGSEGV || sig == SIGABRT)) {
                    printf("[timedexec] NOTE: This crash was likely caused by exceeding the strict memory limit (%lld bytes).\n", (long long)mem_limit);
                }
            }
        }
    }

    return EXIT_SUCCESS;
}