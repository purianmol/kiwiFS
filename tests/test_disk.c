/*
 * test_disk.c — Virtual Disk Layer Unit Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "disk.h"

#define TEST_IMG "test_disk.img"

static void test_disk_lifecycle(void)
{
    unlink(TEST_IMG);

    Disk d;
    assert(disk_create(TEST_IMG, &d) == 0);

    /* Verify zero-fill */
    uint8_t buf[BLOCK_SIZE];
    assert(disk_read_block(&d, 0, buf) == 0);
    for (size_t i = 0; i < BLOCK_SIZE; i++) {
        assert(buf[i] == 0);
    }

    /* Write pattern and read back */
    memset(buf, 0xAB, sizeof(buf));
    assert(disk_write_block(&d, 5, buf) == 0);

    uint8_t read_buf[BLOCK_SIZE];
    assert(disk_read_block(&d, 5, read_buf) == 0);
    assert(memcmp(buf, read_buf, BLOCK_SIZE) == 0);

    /* Out of bounds checks */
    assert(disk_read_block(&d, TOTAL_BLOCKS, read_buf) == -1);
    assert(disk_write_block(&d, TOTAL_BLOCKS, buf) == -1);

    disk_close(&d);

    /* Reopen existing */
    assert(disk_open(TEST_IMG, &d) == 0);
    assert(disk_read_block(&d, 5, read_buf) == 0);
    assert(memcmp(buf, read_buf, BLOCK_SIZE) == 0);
    disk_close(&d);

    unlink(TEST_IMG);
}

int main(void)
{
    printf("Running test_disk...\n");
    test_disk_lifecycle();
    printf("test_disk passed.\n");
    return 0;
}
