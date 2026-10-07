/*
 * main.c — kiwiFS Command-Line Interface Entry Point
 *
 * Usage:
 *   ./kiwifs --format <disk.img>   Format a new virtual disk image
 *   ./kiwifs <disk.img>            Mount an existing virtual disk and start shell
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fs.h"
#include "shell.h"

static void print_usage(const char *prog_name)
{
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s --format <disk.img>    Format a new kiwiFS disk image\n", prog_name);
    fprintf(stderr, "  %s <disk.img>             Mount an existing disk image\n", prog_name);
    fprintf(stderr, "  %s --help                 Show this help message\n", prog_name);
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    /* Format mode */
    if (strcmp(argv[1], "--format") == 0 || strcmp(argv[1], "-f") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Error: --format requires a disk image filename\n");
            print_usage(argv[0]);
            return 1;
        }

        const char *disk_path = argv[2];
        if (fs_format(disk_path) != 0) {
            fprintf(stderr, "Failed to format filesystem image '%s'\n", disk_path);
            return 1;
        }

        printf("Filesystem formatted successfully.\n");
        return 0;
    }

    /* Mount mode */
    const char *disk_path = argv[1];
    Filesystem fs;

    if (fs_mount(disk_path, &fs) != 0) {
        fprintf(stderr, "Failed to mount filesystem image '%s'\n", disk_path);
        return 1;
    }

    printf("kiwiFS v1.0 — virtual filesystem\n");
    printf("Mounted %s (4 MB, %u blocks)\n", disk_path, fs.sb.total_blocks);

    shell_run(&fs);

    fs_unmount(&fs);
    return 0;
}
