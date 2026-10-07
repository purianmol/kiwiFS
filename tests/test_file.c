/*
 * test_file.c — File Operations & Persistence Integration Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "fs.h"

#define TEST_IMG "test_file.img"

static void test_file_and_persistence(void)
{
    unlink(TEST_IMG);

    /* Format new filesystem */
    assert(fs_format(TEST_IMG) == 0);

    /* Mount session 1 */
    Filesystem fs;
    assert(fs_mount(TEST_IMG, &fs) == 0);

    /* Create and write file */
    assert(fs_mkdir(&fs, "docs") == 0);
    assert(fs_cd(&fs, "docs") == 0);
    assert(fs_touch(&fs, "hello.txt") == 0);

    const char *msg = "Hello, persistent kiwiFS world!";
    assert(fs_write(&fs, "hello.txt", msg) == 0);

    char read_buf[256];
    uint32_t bytes_read = 0;
    assert(fs_cat(&fs, "hello.txt", read_buf, sizeof(read_buf), &bytes_read) == 0);
    assert(bytes_read == strlen(msg));
    read_buf[bytes_read] = '\0';
    assert(strcmp(read_buf, msg) == 0);

    /* Unmount to simulate program exit */
    fs_unmount(&fs);

    /* Mount session 2 (verify persistence) */
    Filesystem fs2;
    assert(fs_mount(TEST_IMG, &fs2) == 0);

    assert(fs_cd(&fs2, "docs") == 0);
    memset(read_buf, 0, sizeof(read_buf));
    assert(fs_cat(&fs2, "hello.txt", read_buf, sizeof(read_buf), &bytes_read) == 0);
    assert(bytes_read == strlen(msg));
    read_buf[bytes_read] = '\0';
    assert(strcmp(read_buf, msg) == 0);

    /* Delete file */
    assert(fs_rm(&fs2, "hello.txt") == 0);
    assert(fs_cat(&fs2, "hello.txt", read_buf, sizeof(read_buf), &bytes_read) == -1);

    fs_unmount(&fs2);
    unlink(TEST_IMG);
}

int main(void)
{
    printf("Running test_file...\n");
    test_file_and_persistence();
    printf("test_file passed.\n");
    return 0;
}
