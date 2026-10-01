/*
 * superblock.h — Superblock Structure and Operations
 *
 * The superblock is stored in block 0 of the virtual disk.  It holds all
 * filesystem-wide metadata: magic number, block geometry, and the starting
 * block numbers of every major region (bitmap, inode table, data blocks).
 *
 * Every mount validates the magic number before any other operation.
 */

#ifndef KIWI_SUPERBLOCK_H
#define KIWI_SUPERBLOCK_H

#include <stdint.h>
#include "disk.h"

/* ── Magic number ──────────────────────────────────────────────────────── */

/*
 * FS_MAGIC encodes "KWS1" (kiwiFS version 1) in little-endian order.
 * If the first four bytes of block 0 are not this value, the image is
 * either unformatted or corrupted, and the mount must be refused.
 */
#define FS_MAGIC  0x4B575331u   /* 'K','W','S','1' */

/* ── Superblock region layout ──────────────────────────────────────────── */

#define SUPERBLOCK_BLOCK  0u    /* superblock always lives in block 0 */

/* ── On-disk structure ─────────────────────────────────────────────────── */

/*
 * Superblock - filesystem metadata stored persistently in block 0.
 *
 * All fields are uint32_t so the struct has a stable, architecture-
 * independent layout when written with fwrite.  The struct is smaller
 * than one BLOCK_SIZE; the rest of block 0 is unused padding.
 */
typedef struct {
    uint32_t magic;           /* must equal FS_MAGIC on a valid disk      */
    uint32_t block_size;      /* size of one block in bytes (4096)        */
    uint32_t total_blocks;    /* total number of blocks on the disk       */

    uint32_t bitmap_start;    /* first block of the block-allocation bitmap */
    uint32_t bitmap_blocks;   /* number of blocks used by the bitmap      */

    uint32_t inode_start;     /* first block of the inode table           */
    uint32_t inode_blocks;    /* number of blocks used by the inode table */

    uint32_t data_start;      /* first block available for file/dir data  */

    uint32_t root_inode;      /* inode number of the root directory (0)   */
} Superblock;

/* ── API ───────────────────────────────────────────────────────────────── */

/*
 * sb_write - serialise `sb` and write it to block 0 of `disk`.
 * Returns 0 on success, -1 on failure.
 */
int sb_write(Disk *disk, const Superblock *sb);

/*
 * sb_read - read block 0 of `disk` and deserialise it into `sb`.
 * Returns 0 on success, -1 on failure.
 */
int sb_read(Disk *disk, Superblock *sb);

/*
 * sb_validate - check that `sb->magic == FS_MAGIC`.
 * Returns 0 if the magic is correct, -1 otherwise (prints an error).
 */
int sb_validate(const Superblock *sb);

#endif /* KIWI_SUPERBLOCK_H */
