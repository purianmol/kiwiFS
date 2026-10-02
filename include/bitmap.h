/*
 * bitmap.h — Block Allocation Bitmap Interface
 *
 * The bitmap tracks which disk blocks are free or allocated.
 * One bit corresponds to one block: 0 = free, 1 = allocated.
 *
 * With TOTAL_BLOCKS = 1024 we need 1024 bits = 128 bytes, which
 * comfortably fits inside a single 4096-byte disk block (block 1).
 *
 * The bitmap is always read from disk before use and written back
 * after every allocation or free, so it stays in sync with disk.img.
 */

#ifndef KIWI_BITMAP_H
#define KIWI_BITMAP_H

#include <stdint.h>
#include "disk.h"

/* ── Layout constants ──────────────────────────────────────────────────── */

#define BITMAP_BLOCK   1u    /* block number where the bitmap lives        */
#define BITMAP_BLOCKS  1u    /* number of blocks the bitmap occupies       */

/* bytes needed to hold one bit per disk block */
#define BITMAP_BYTES   (TOTAL_BLOCKS / 8u)

/* ── API ───────────────────────────────────────────────────────────────── */

/*
 * bitmap_alloc - find the first free block, mark it allocated, and
 *                write the updated bitmap back to disk.
 *
 * On success stores the allocated block number in *block_no and returns 0.
 * Returns -1 if no free blocks remain or on I/O error.
 */
int bitmap_alloc(Disk *disk, uint32_t *block_no);

/*
 * bitmap_free - mark block `block_no` as free and write the bitmap back.
 *
 * Returns 0 on success, -1 if block_no is out of range or on I/O error.
 */
int bitmap_free(Disk *disk, uint32_t block_no);

/*
 * bitmap_test - check whether block `block_no` is allocated.
 *
 * Reads the bitmap from disk.
 * Returns 1 if allocated, 0 if free, -1 on error.
 */
int bitmap_test(Disk *disk, uint32_t block_no);

/*
 * bitmap_init - zero the entire bitmap on disk (all blocks free).
 *
 * Called once during filesystem format.
 * Returns 0 on success, -1 on failure.
 */
int bitmap_init(Disk *disk);

#endif /* KIWI_BITMAP_H */
