/*
 * inode.c — Inode Management Implementation
 *
 * Inodes are stored sequentially in the inode table, which starts at
 * INODE_START_BLOCK.  Because sizeof(Inode) == 44 bytes, about 93 inodes
 * fit in each 4096-byte block.  To read or write inode `id` we calculate:
 *
 *   block_no  = INODE_START_BLOCK + (id / INODES_PER_BLOCK)
 *   offset    = (id % INODES_PER_BLOCK) * sizeof(Inode)
 *
 * We always read the full block, modify our one slot, and write it back.
 */

#include "inode.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* ── inode_table_init ──────────────────────────────────────────────────── */

int inode_table_init(Disk *disk)
{
    uint8_t block[BLOCK_SIZE];
    memset(block, 0, sizeof(block));

    for (uint32_t b = 0; b < INODE_BLOCKS; b++) {
        if (disk_write_block(disk, INODE_START_BLOCK + b, block) != 0)
            return -1;
    }
    return 0;
}

/* ── inode_read ────────────────────────────────────────────────────────── */

int inode_read(Disk *disk, uint32_t id, Inode *out)
{
    if (id >= MAX_INODES) {
        fprintf(stderr, "inode: id %u out of range (max %u)\n", id, MAX_INODES - 1);
        return -1;
    }

    uint32_t block_no = INODE_START_BLOCK + (id / INODES_PER_BLOCK);
    uint32_t offset   = (id % INODES_PER_BLOCK) * sizeof(Inode);

    uint8_t block[BLOCK_SIZE];
    if (disk_read_block(disk, block_no, block) != 0)
        return -1;

    memcpy(out, block + offset, sizeof(Inode));
    return 0;
}

/* ── inode_write ───────────────────────────────────────────────────────── */

int inode_write(Disk *disk, const Inode *in)
{
    if (in->id >= MAX_INODES) {
        fprintf(stderr, "inode: id %u out of range (max %u)\n", in->id, MAX_INODES - 1);
        return -1;
    }

    uint32_t block_no = INODE_START_BLOCK + (in->id / INODES_PER_BLOCK);
    uint32_t offset   = (in->id % INODES_PER_BLOCK) * sizeof(Inode);

    uint8_t block[BLOCK_SIZE];
    if (disk_read_block(disk, block_no, block) != 0)
        return -1;

    memcpy(block + offset, in, sizeof(Inode));
    return disk_write_block(disk, block_no, block);
}
