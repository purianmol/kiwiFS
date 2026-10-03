/*
 * inode.h — Inode Structure and Operations
 *
 * Each file and directory in kiwiFS is represented by an inode.
 * Inodes are stored in a fixed-size table that occupies a contiguous
 * range of blocks starting at INODE_START_BLOCK.
 *
 * An inode holds:
 *   - the file type (free / file / directory)
 *   - the file size in bytes
 *   - up to DIRECT_BLOCKS data block numbers
 *   - an in-use flag
 *
 * Inode 0 is always the root directory.
 */

#ifndef KIWI_INODE_H
#define KIWI_INODE_H

#include <stdint.h>
#include "disk.h"

/* ── Constants ─────────────────────────────────────────────────────────── */

#define DIRECT_BLOCKS      8u          /* direct block pointers per inode   */
#define MAX_FILE_SIZE      (DIRECT_BLOCKS * BLOCK_SIZE)  /* 32 768 bytes    */

#define INODE_START_BLOCK  2u          /* first block of the inode table    */
#define INODES_PER_BLOCK   (BLOCK_SIZE / sizeof(Inode))
#define MAX_INODES         128u        /* maximum number of inodes          */
#define INODE_BLOCKS       ((MAX_INODES + INODES_PER_BLOCK - 1) / INODES_PER_BLOCK)

/* inode type field values */
#define INODE_FREE         0u
#define INODE_FILE         1u
#define INODE_DIRECTORY    2u

/* ── On-disk structure ─────────────────────────────────────────────────── */

/*
 * Inode - per-file/directory metadata stored persistently.
 *
 * All fields are uint32_t for a stable, architecture-independent layout.
 * sizeof(Inode) == 44 bytes; ~93 inodes fit in one 4096-byte block.
 */
typedef struct {
    uint32_t id;                      /* inode number (index in the table)  */
    uint32_t type;                    /* INODE_FREE / FILE / DIRECTORY      */
    uint32_t size;                    /* file size in bytes                 */
    uint32_t blocks[DIRECT_BLOCKS];   /* data block numbers (0 = unused)   */
    uint32_t used;                    /* 1 = allocated, 0 = free            */
} Inode;

/* ── API ───────────────────────────────────────────────────────────────── */

/*
 * inode_table_init - zero-fill every inode slot on disk.
 * Called once during filesystem format.
 * Returns 0 on success, -1 on failure.
 */
int inode_table_init(Disk *disk);

/*
 * inode_alloc - find the first free inode, mark it used, and write it back.
 * Stores the inode number in *inode_id.
 * Returns 0 on success, -1 if the table is full or on I/O error.
 */
int inode_alloc(Disk *disk, uint32_t type, uint32_t *inode_id);

/*
 * inode_read - load inode `id` from disk into *out.
 * Returns 0 on success, -1 on failure.
 */
int inode_read(Disk *disk, uint32_t id, Inode *out);

/*
 * inode_write - persist *in to the inode table on disk.
 * Returns 0 on success, -1 on failure.
 */
int inode_write(Disk *disk, const Inode *in);

/*
 * inode_free - mark inode `id` as free (zeros the struct) on disk.
 * Returns 0 on success, -1 on failure.
 */
int inode_free(Disk *disk, uint32_t id);

#endif /* KIWI_INODE_H */
