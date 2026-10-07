/*
 * test_inode.c — Inode Management Unit Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "disk.h"
#include "inode.h"

#define TEST_IMG "test_inode.img"

static void test_inode_operations(void)
{
    unlink(TEST_IMG);

    Disk d;
    assert(disk_create(TEST_IMG, &d) == 0);
    assert(inode_table_init(&d) == 0);

    /* Allocate an inode */
    uint32_t id1;
    assert(inode_alloc(&d, INODE_FILE, &id1) == 0);
    assert(id1 == 0);

    /* Read back */
    Inode in;
    assert(inode_read(&d, id1, &in) == 0);
    assert(in.used == 1);
    assert(in.type == INODE_FILE);
    assert(in.size == 0);

    /* Update inode fields */
    in.size = 1234;
    in.blocks[0] = 42;
    assert(inode_write(&d, &in) == 0);

    /* Verify updated values */
    Inode in2;
    assert(inode_read(&d, id1, &in2) == 0);
    assert(in2.size == 1234);
    assert(in2.blocks[0] == 42);

    /* Allocate second inode */
    uint32_t id2;
    assert(inode_alloc(&d, INODE_DIRECTORY, &id2) == 0);
    assert(id2 == 1);

    /* Free first inode */
    assert(inode_free(&d, id1) == 0);
    assert(inode_read(&d, id1, &in) == 0);
    assert(in.used == 0);

    /* Reallocate -> should reuse slot 0 */
    uint32_t id_reused;
    assert(inode_alloc(&d, INODE_FILE, &id_reused) == 0);
    assert(id_reused == 0);

    disk_close(&d);
    unlink(TEST_IMG);
}

int main(void)
{
    printf("Running test_inode...\n");
    test_inode_operations();
    printf("test_inode passed.\n");
    return 0;
}
