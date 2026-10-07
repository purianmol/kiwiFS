/*
 * test_directory.c — Directory Entry Unit Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "disk.h"
#include "bitmap.h"
#include "inode.h"
#include "directory.h"

#define TEST_IMG "test_directory.img"

static void test_directory_operations(void)
{
    unlink(TEST_IMG);

    Disk d;
    assert(disk_create(TEST_IMG, &d) == 0);
    assert(bitmap_init(&d) == 0);
    assert(inode_table_init(&d) == 0);

    /* Reserve metadata blocks 0..3 (superblock, bitmap, inode table) */
    for (uint32_t i = 0; i < 4; i++) {
        uint32_t b;
        assert(bitmap_alloc(&d, &b) == 0 && b == i);
    }

    /* Allocate root dir (inode 0) */
    uint32_t root_id;
    assert(inode_alloc(&d, INODE_DIRECTORY, &root_id) == 0);
    assert(dir_init(&d, root_id, root_id) == 0);

    /* Verify '.' and '..' */
    uint32_t target;
    assert(dir_lookup(&d, root_id, ".", &target) == 0 && target == root_id);
    assert(dir_lookup(&d, root_id, "..", &target) == 0 && target == root_id);

    /* Initially empty except '.' and '..' */
    assert(dir_is_empty(&d, root_id) == 1);

    /* Add entry */
    uint32_t child_inode = 5;
    assert(dir_add_entry(&d, root_id, "testdir", child_inode) == 0);
    assert(dir_is_empty(&d, root_id) == 0);

    /* Duplicate rejection */
    assert(dir_add_entry(&d, root_id, "testdir", child_inode) == -1);

    /* Lookup entry */
    assert(dir_lookup(&d, root_id, "testdir", &target) == 0 && target == child_inode);

    /* Remove entry */
    assert(dir_remove_entry(&d, root_id, "testdir") == 0);
    assert(dir_lookup(&d, root_id, "testdir", &target) == -1);
    assert(dir_is_empty(&d, root_id) == 1);

    /* Cannot remove '.' or '..' */
    assert(dir_remove_entry(&d, root_id, ".") == -1);
    assert(dir_remove_entry(&d, root_id, "..") == -1);

    disk_close(&d);
    unlink(TEST_IMG);
}

int main(void)
{
    printf("Running test_directory...\n");
    test_directory_operations();
    printf("test_directory passed.\n");
    return 0;
}
