/*
 * test_bitmap.c — Block Allocation Bitmap Unit Tests
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>

#include "disk.h"
#include "bitmap.h"

#define TEST_IMG "test_bitmap.img"

static void test_bitmap_operations(void)
{
    unlink(TEST_IMG);

    Disk d;
    assert(disk_create(TEST_IMG, &d) == 0);
    assert(bitmap_init(&d) == 0);

    /* Initially, block 0 should be free */
    assert(bitmap_test(&d, 0) == 0);
    assert(bitmap_test(&d, 100) == 0);

    /* Allocate block 0 */
    uint32_t b0;
    assert(bitmap_alloc(&d, &b0) == 0);
    assert(b0 == 0);
    assert(bitmap_test(&d, 0) == 1);

    /* Allocate block 1 */
    uint32_t b1;
    assert(bitmap_alloc(&d, &b1) == 0);
    assert(b1 == 1);
    assert(bitmap_test(&d, 1) == 1);

    /* Free block 0 */
    assert(bitmap_free(&d, 0) == 0);
    assert(bitmap_test(&d, 0) == 0);
    assert(bitmap_test(&d, 1) == 1);

    /* Reallocate -> should reuse block 0 */
    uint32_t b_reuse;
    assert(bitmap_alloc(&d, &b_reuse) == 0);
    assert(b_reuse == 0);
    assert(bitmap_test(&d, 0) == 1);

    disk_close(&d);
    unlink(TEST_IMG);
}

int main(void)
{
    printf("Running test_bitmap...\n");
    test_bitmap_operations();
    printf("test_bitmap passed.\n");
    return 0;
}
