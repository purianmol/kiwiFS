/*
 * shell.c — Interactive REPL Shell Implementation
 */

#include "shell.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINE_LEN 1024

/* Callback used by fs_ls to print directory items */
static void ls_printer(const char *name, uint32_t inode_id, uint32_t type, uint32_t size, void *user_data)
{
    (void)inode_id;
    (void)size;
    (void)user_data;

    /* Don't list '.' and '..' by default to keep output clean, matching example session */
    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
        return;

    if (type == INODE_DIRECTORY) {
        printf("%s/\n", name);
    } else {
        printf("%s\n", name);
    }
}

/* Print help menu */
static void print_help(void)
{
    printf("Supported commands:\n");
    printf("  ls [path]                List directory contents\n");
    printf("  mkdir <name>             Create a new directory\n");
    printf("  cd <path>                Change current directory\n");
    printf("  pwd                      Print current working directory\n");
    printf("  touch <file>             Create an empty file\n");
    printf("  write <file> \"<text>\"    Write text to a file\n");
    printf("  cat <file>               Display file contents\n");
    printf("  rm <file>                Delete a file\n");
    printf("  format                   Re-format the virtual disk\n");
    printf("  help                     Display this help message\n");
    printf("  exit                     Exit kiwiFS\n");
}

/* Trim leading and trailing whitespace */
static char *trim_whitespace(char *str)
{
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;

    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

void shell_run(Filesystem *fs)
{
    char line[MAX_LINE_LEN];
    char cat_buffer[MAX_FILE_SIZE + 1];

    while (1) {
        printf("kiwifs> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break; /* EOF */
        }

        char *cmd_line = trim_whitespace(line);
        if (strlen(cmd_line) == 0)
            continue;

        /* Extract command keyword */
        char cmd[64];
        int offset = 0;
        if (sscanf(cmd_line, "%63s%n", cmd, &offset) != 1)
            continue;

        char *args = trim_whitespace(cmd_line + offset);

        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
        } else if (strcmp(cmd, "pwd") == 0) {
            char cwd[MAX_PATH_LEN];
            fs_pwd(fs, cwd, sizeof(cwd));
            printf("%s\n", cwd);
        } else if (strcmp(cmd, "ls") == 0) {
            fs_ls(fs, strlen(args) > 0 ? args : NULL, ls_printer, NULL);
        } else if (strcmp(cmd, "mkdir") == 0) {
            if (strlen(args) == 0) {
                printf("Usage: mkdir <name>\n");
            } else {
                if (fs_mkdir(fs, args) == 0) {
                    printf("Directory created: %s\n", args);
                }
            }
        } else if (strcmp(cmd, "cd") == 0) {
            fs_cd(fs, strlen(args) > 0 ? args : "/");
        } else if (strcmp(cmd, "touch") == 0) {
            if (strlen(args) == 0) {
                printf("Usage: touch <filename>\n");
            } else {
                if (fs_touch(fs, args) == 0) {
                    printf("File created: %s\n", args);
                }
            }
        } else if (strcmp(cmd, "cat") == 0) {
            if (strlen(args) == 0) {
                printf("Usage: cat <filename>\n");
            } else {
                uint32_t bytes_read = 0;
                if (fs_cat(fs, args, cat_buffer, MAX_FILE_SIZE, &bytes_read) == 0) {
                    cat_buffer[bytes_read] = '\0';
                    printf("%s\n", cat_buffer);
                }
            }
        } else if (strcmp(cmd, "rm") == 0) {
            if (strlen(args) == 0) {
                printf("Usage: rm <filename>\n");
            } else {
                if (fs_rm(fs, args) == 0) {
                    printf("Removed: %s\n", args);
                }
            }
        } else if (strcmp(cmd, "write") == 0) {
            char filename[MAX_NAME_LEN + 1];
            char *text_start = NULL;

            /* Parse filename and quoted text: write <file> "<text>" */
            if (sscanf(args, "%27s%n", filename, &offset) == 1) {
                char *rest = trim_whitespace(args + offset);
                if (rest[0] == '"') {
                    text_start = rest + 1;
                    char *closing_quote = strrchr(text_start, '"');
                    if (closing_quote) {
                        *closing_quote = '\0';
                    }
                } else {
                    text_start = rest;
                }

                if (fs_write(fs, filename, text_start) == 0) {
                    printf("Written %zu bytes to %s\n", strlen(text_start), filename);
                }
            } else {
                printf("Usage: write <file> \"<text>\"\n");
            }
        } else if (strcmp(cmd, "format") == 0) {
            char path[256];
            strncpy(path, fs->disk.path, sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
            fs_unmount(fs);

            if (fs_format(path) == 0 && fs_mount(path, fs) == 0) {
                printf("Filesystem formatted successfully.\n");
            } else {
                fprintf(stderr, "Error reformatting filesystem\n");
            }
        } else {
            printf("Unknown command '%s'. Type 'help' for available commands.\n", cmd);
        }
    }
}
