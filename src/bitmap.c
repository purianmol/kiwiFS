/*
 * bitmap.c — Block Allocation Bitmap Implementation
 *
 * The bitmap is stored as a flat array of bytes in BITMAP_BLOCK (block 1).
 * Bit `n` of byte `n/8` corresponds to disk block `n`.
 * 0 = free, 1 = allocated.
 *
 * Every public function reads the bitmap from disk, performs its operation
 * on an in-memory copy, then writes the (possibly modified) bitmap back.
 * This keeps disk.img as the single source of truth.
 */

#include "bitmap.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* ── Internal bit helpers ──────────────────────────────────────────────── */

/* Return the byte index within the bitmap for block `n`. */
static inline uint32_t byte_idx(uint32_t n) { return n / 8u; }

/* Return the bit mask for block `n` within its byte. */
static inline uint8_t  bit_mask(uint32_t n) { return (uint8_t)(1u << (n % 8u)); }

/* ── bitmap_init ───────────────────────────────────────────────────────── */

int bitmap_init(Disk *disk)
{
    uint8_t block[BLOCK_SIZE];
    memset(block, 0, sizeof(block));   /* all bits 0 = all blocks free */
    return disk_write_block(disk, BITMAP_BLOCK, block);
}

/* ── bitmap_test ───────────────────────────────────────────────────────── */

int bitmap_test(Disk *disk, uint32_t block_no)
{
    if (block_no >= TOTAL_BLOCKS) {
        fprintf(stderr, "bitmap: block %u out of range\n", block_no);
        return -1;
    }

    uint8_t block[BLOCK_SIZE];
    if (disk_read_block(disk, BITMAP_BLOCK, block) != 0)
        return -1;

    return (block[byte_idx(block_no)] & bit_mask(block_no)) ? 1 : 0;
}

/* ── bitmap_alloc ──────────────────────────────────────────────────────── */

int bitmap_alloc(Disk *disk, uint32_t *block_no)
{
    uint8_t block[BLOCK_SIZE];
    if (disk_read_block(disk, BITMAP_BLOCK, block) != 0)
        return -1;

    /* linear scan: find the first bit that is 0 (free) */
    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        if (!(block[byte_idx(i)] & bit_mask(i))) {
            /* mark allocated */
            block[byte_idx(i)] |= bit_mask(i);

            if (disk_write_block(disk, BITMAP_BLOCK, block) != 0)
                return -1;

            *block_no = i;
            return 0;
        }
    }

    fprintf(stderr, "bitmap: disk is full — no free blocks\n");
    return -1;
}

/* ── bitmap_free ───────────────────────────────────────────────────────── */

int bitmap_free(Disk *disk, uint32_t block_no)
{
    if (block_no >= TOTAL_BLOCKS) {
        fprintf(stderr, "bitmap: cannot free block %u — out of range\n", block_no);
        return -1;
    }

    uint8_t block[BLOCK_SIZE];
    if (disk_read_block(disk, BITMAP_BLOCK, block) != 0)
        return -1;

    /* clear the bit */
    block[byte_idx(block_no)] &= (uint8_t)~bit_mask(block_no);

    return disk_write_block(disk, BITMAP_BLOCK, block);
}
