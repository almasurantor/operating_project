#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <getopt.h> 

#define BUFFER_SIZE 1024

/* signal handler for graceful exit */
void handle_signal(int sig)
{
    printf("\nInterrupted (signal %d). Exiting.\n", sig);
    exit(1);
}

/* Vigenere encrypt - shifts each byte forward by key value, wraps at 256 */
void vigenere_encrypt(char *buffer, int length, char *key, int key_len)
{
    int i;
    for (i = 0; i < length; i++) {
        buffer[i] = (unsigned char)((buffer[i] + key[i % key_len]) % 256);
    }
}

/* Vigenere decrypt  */
void vigenere_decrypt(char *buffer, int length, char *key, int key_len)
{
    int i;
    for (i = 0; i < length; i++) {
        buffer[i] = (unsigned char)((buffer[i] - key[i % key_len] + 256) % 256);
    }
}

void print_usage(char *prog)
{
    printf("Usage: %s -e|-d -k <key> -i <input file> -o <output file>\n", prog);
    printf("  -e           encrypt mode\n");
    printf("  -d           decrypt mode\n");
    printf("  -k <key>     key string\n");
    printf("  -i <file>    input file\n");
    printf("  -o <file>    output file\n");
}

int main(int argc, char *argv[])
{
    /* register signal handlers for graceful exit */
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    int mode = 0;       /* 1 = encrypt, 2 = decrypt */
    char *key = NULL;
    char *infile = NULL;
    char *outfile = NULL;
    int opt;

    /* getopt() parses command line flags */
    while ((opt = getopt(argc, argv, "edk:i:o:")) != -1) {
        switch (opt) {
            case 'e':
                mode = 1;
                break;
            case 'd':
                mode = 2;
                break;
            case 'k':
                key = optarg;
                break;
            case 'i':
                infile = optarg;
                break;
            case 'o':
                outfile = optarg;
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    /* validate all required arguments are provided */
    if (mode == 0 || key == NULL || infile == NULL || outfile == NULL) {
        printf("Error: missing required arguments.\n");
        print_usage(argv[0]);
        return 1;
    }

    int key_len = strlen(key);
    if (key_len == 0) {
        printf("Error: key cannot be empty.\n");
        return 1;
    }

    /* lock the key in memory so it cannot be swapped to disk */
    if (mlock(key, key_len) != 0) {
        perror("Warning: mlock failed");
        /* non-fatal, continue anyway */
    }

    /* open input file using open() system call*/
    int fd_in = open(infile, O_RDONLY);
    if (fd_in < 0) {
        perror("Error opening input file");
        return 1;
    }

    /* open output file using open() system call*/
    int fd_out = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        perror("Error opening output file");
        close(fd_in);
        return 1;
    }

    /* read, encrypt/decrypt, and write in chunks using read()/write() */
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = read(fd_in, buffer, BUFFER_SIZE)) > 0) {
        if (mode == 1)
            vigenere_encrypt(buffer, bytes_read, key, key_len);
        else
            vigenere_decrypt(buffer, bytes_read, key, key_len);

        if (write(fd_out, buffer, bytes_read) < 0) {
            perror("Error writing to output file");
            close(fd_in);
            close(fd_out);
            return 1;
        }
    }

    if (bytes_read < 0) {
        perror("Error reading input file");
        close(fd_in);
        close(fd_out);
        return 1;
    }

    /* close files using close() system call */
    close(fd_in);
    close(fd_out);

    /* unlock the key from memory after we are done */
    munlock(key, key_len);

    if (mode == 1)
        printf("File encrypted successfully -> %s\n", outfile);
    else
        printf("File decrypted successfully -> %s\n", outfile);

    return 0;
}