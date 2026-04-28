#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

void print_usage(char *program_name) {
    printf("Usage: %s -f <logfile>\n", program_name);
}

int main(int argc, char *argv[]) {
    int opt;
    char *filename = NULL;

    int fd;
    struct stat file_stat;
    char *data;

    long total_lines = 0;
    long error_count = 0;
    long warning_count = 0;
    long info_count = 0;

    while ((opt = getopt(argc, argv, "f:")) != -1) {
        switch (opt) {
            case 'f':
                filename = optarg;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    if (filename == NULL) {
        print_usage(argv[0]);
        return 1;
    }

    fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        return 1;
    }

    if (fstat(fd, &file_stat) < 0) {
        perror("Error getting file size");
        close(fd);
        return 1;
    }

    if (file_stat.st_size == 0) {
        printf("The file is empty.\n");
        close(fd);
        return 0;
    }

    data = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) {
        perror("mmap failed");
        close(fd);
        return 1;
    }

    char *start = data;
    char *end = data + file_stat.st_size;
    char *line_start = start;

    while (line_start < end) {
        char *line_end = memchr(line_start, '\n', end - line_start);
        size_t line_length;

        if (line_end != NULL) {
            line_length = line_end - line_start;
        } else {
            line_length = end - line_start;
        }

        total_lines++;

        if (memmem(line_start, line_length, "ERROR", 5) != NULL) {
            error_count++;
        }
        if (memmem(line_start, line_length, "WARNING", 7) != NULL) {
            warning_count++;
        }
        if (memmem(line_start, line_length, "INFO", 4) != NULL) {
            info_count++;
        }

        if (line_end == NULL) {
            break;
        }

        line_start = line_end + 1;
    }

    printf("Log Analysis Result\n");
    printf("-------------------\n");
    printf("File name     : %s\n", filename);
    printf("Total lines   : %ld\n", total_lines);
    printf("ERROR count   : %ld\n", error_count);
    printf("WARNING count : %ld\n", warning_count);
    printf("INFO count    : %ld\n", info_count);

    if (munmap(data, file_stat.st_size) < 0) {
        perror("munmap failed");
    }

    close(fd);
    return 0;
}
