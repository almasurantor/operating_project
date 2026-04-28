#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: ./shell <command> [args]\n");
        return 1;
    }

    char *cmd = argv[1];

    if (strcmp(cmd, "timedexec") == 0) {
        printf("timedexec not implemented yet\n");
    } 
    else if (strcmp(cmd, "loganalyzer") == 0) {
        printf("loganalyzer not implemented yet\n");
    } 
    else if (strcmp(cmd, "filecrypt") == 0) {
        printf("filecrypt not implemented yet\n");
    } 
    else if (strcmp(cmd, "filediffadvanced") == 0) {
        printf("filediffadvanced not implemented yet\n");
    } 
    else {
        printf("Unknown command: %s\n", cmd);
    }

    return 0;
}
