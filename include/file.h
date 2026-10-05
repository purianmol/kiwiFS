/*
 * file.h — File Operations Interface
 *
 * Implements file-level operations: create, write, read, and delete.
 *
 * Files in kiwiFS can grow up to MAX_FILE_SIZE (32 KB = 8 direct blocks).
 * In v1, writing to a file replaces its entire contents.
 */

#ifndef KIWI_FILE_H
#define KIWI_FILE_H

#include <stdint.h>
#include <stddef.h>
#include "disk.h"
#include "inode.h"

/* ── API ───────────────────────────────────────────────────────────────── */

/*
 * file_create - create an empty file with `name` in directory `dir_inode_id`.
 *
 * Allocates a new inode of type INODE_FILE and links it into the directory.
 * On success, stores the allocated inode ID in *out_inode_id.
 * Returns 0 on success, -1 on failure.
 */
int file_create(Disk *disk, uint32_t dir_inode_id, const char *name, uint32_t *out_inode_id);

/*
 * file_write - write `size` bytes from `data` into file `file_inode_id`.
 *
 * Replaces existing file contents.  Allocates data blocks as needed from
 * the bitmap, and frees any old data blocks that are no longer used.
 * `size` cannot exceed MAX_FILE_SIZE (32 KB).
 *
 * Returns 0 on success, -1 on failure.
 */
int file_write(Disk *disk, uint32_t file_inode_id, const void *data, uint32_t size);

/*
 * file_read - read up to `buf_size` bytes from `file_inode_id` into `buf`.
 *
 * Reads across direct data blocks.  Stores actual number of bytes read
 * in *bytes_read.
 *
 * Returns 0 on success, -1 on failure.
 */
int file_read(Disk *disk, uint32_t file_inode_id, void *buf, uint32_t buf_size, uint32_t *bytes_read);

/*
 * file_delete - delete file `name` from directory `dir_inode_id`.
 *
 * Verifies that the target is a regular file (not directory), frees all its
 * data blocks in the bitmap, frees the inode, and removes directory entry.
 *
 * Returns 0 on success, -1 on failure.
 */
int file_delete(Disk *disk, uint32_t dir_inode_id, const char *name);

#endif /* KIWI_FILE_H */
