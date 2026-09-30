/*
 * disk.h — Virtual Disk Layer Interface
 *
 * The disk layer is the only part of kiwiFS that talks directly to the
 * host filesystem.  Every other module goes through disk_read_block()
 * and disk_write_block() instead of calling fread/fwrite themselves.
 *
 * The virtual disk is a flat binary file (disk.img) divided into fixed-
 * size blocks of BLOCK_SIZE bytes.  Block numbers are zero-based.
 */

#ifndef KIWI_DISK_H
#define KIWI_DISK_H

#include <stdint.h>
#include <stdio.h>

/* ── Constants ─────────────────────────────────────────────────────────── */

#define BLOCK_SIZE    4096u          /* bytes per block                      */
#define TOTAL_BLOCKS  1024u          /* total blocks on the virtual disk     */
#define DISK_SIZE     (BLOCK_SIZE * TOTAL_BLOCKS)  /* 4 MB                  */

/* ── Disk handle ───────────────────────────────────────────────────────── */

/*
 * Disk - opaque handle to an open virtual disk image.
 *
 * Callers obtain one via disk_open() or disk_create() and must release it
 * with disk_close() when finished.
 */
typedef struct {
    FILE    *fp;         /* underlying file pointer         */
    char     path[256];  /* path to the disk image file     */
} Disk;

/* ── API ───────────────────────────────────────────────────────────────── */

/*
 * disk_create - create a new, zero-filled disk image at `path`.
 *
 * Overwrites any existing file.  The image is DISK_SIZE bytes long.
 * Returns 0 on success, -1 on failure.
 */
int  disk_create(const char *path, Disk *out);

/*
 * disk_open - open an existing disk image for reading and writing.
 *
 * Returns 0 on success, -1 if the file cannot be opened.
 */
int  disk_open(const char *path, Disk *out);

/*
 * disk_close - flush and close the disk image.
 */
void disk_close(Disk *d);

/*
 * disk_read_block - read one block from the disk into `buf`.
 *
 * `block_no` must be in [0, TOTAL_BLOCKS).
 * `buf` must point to at least BLOCK_SIZE bytes of writable memory.
 * Returns 0 on success, -1 on failure.
 */
int  disk_read_block(Disk *d, uint32_t block_no, void *buf);

/*
 * disk_write_block - write one block from `buf` to the disk.
 *
 * `block_no` must be in [0, TOTAL_BLOCKS).
 * `buf` must point to exactly BLOCK_SIZE bytes of data.
 * Returns 0 on success, -1 on failure.
 */
int  disk_write_block(Disk *d, uint32_t block_no, const void *buf);

#endif /* KIWI_DISK_H */
