/*
 * superblock.c — Superblock Read / Write / Validate
 *
 * The superblock occupies block 0 of the virtual disk.  The struct is
 * smaller than one BLOCK_SIZE, so we read/write a full block and
 * copy only the Superblock-sized portion.
 */

#include "superblock.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>

int sb_write(Disk *disk, const Superblock *sb)
{
    uint8_t block[BLOCK_SIZE];
    memset(block, 0, sizeof(block));
    memcpy(block, sb, sizeof(Superblock));

    return disk_write_block(disk, SUPERBLOCK_BLOCK, block);
}

int sb_read(Disk *disk, Superblock *sb)
{
    uint8_t block[BLOCK_SIZE];

    if (disk_read_block(disk, SUPERBLOCK_BLOCK, block) != 0)
        return -1;

    memcpy(sb, block, sizeof(Superblock));
    return 0;
}

int sb_validate(const Superblock *sb)
{
    if (sb->magic != FS_MAGIC) {
        fprintf(stderr,
                "superblock: bad magic 0x%08X (expected 0x%08X) — "
                "disk is unformatted or corrupted\n",
                sb->magic, FS_MAGIC);
        return -1;
    }
    return 0;
}
