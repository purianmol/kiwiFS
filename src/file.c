/*
 * file.c — File Operation Implementation
 *
 * Implements create, read, write, and delete operations for files.
 */

#include "file.h"
#include "directory.h"
#include "bitmap.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ── file_create ───────────────────────────────────────────────────────── */

int file_create(Disk *disk, uint32_t dir_inode_id, const char *name, uint32_t *out_inode_id)
{
    if (!name || strlen(name) == 0 || strlen(name) > MAX_NAME_LEN) {
        fprintf(stderr, "file: invalid file name\n");
        return -1;
    }

    /* Check if name already exists in target directory */
    uint32_t existing;
    if (dir_lookup(disk, dir_inode_id, name, &existing) == 0) {
        fprintf(stderr, "file: '%s' already exists\n", name);
        return -1;
    }

    /* Allocate a new inode for the file */
    uint32_t new_inode_id;
    if (inode_alloc(disk, INODE_FILE, &new_inode_id) != 0) {
        fprintf(stderr, "file: failed to allocate inode for '%s'\n", name);
        return -1;
    }

    /* Link into directory */
    if (dir_add_entry(disk, dir_inode_id, name, new_inode_id) != 0) {
        inode_free(disk, new_inode_id);
        fprintf(stderr, "file: failed to add directory entry for '%s'\n", name);
        return -1;
    }

    if (out_inode_id) {
        *out_inode_id = new_inode_id;
    }

    return 0;
}

/* ── file_read ─────────────────────────────────────────────────────────── */

int file_read(Disk *disk, uint32_t file_inode_id, void *buf, uint32_t buf_size, uint32_t *bytes_read)
{
    if (!buf || !bytes_read)
        return -1;

    Inode file_inode;
    if (inode_read(disk, file_inode_id, &file_inode) != 0)
        return -1;

    if (file_inode.type != INODE_FILE || !file_inode.used) {
        fprintf(stderr, "file: inode %u is not a regular file\n", file_inode_id);
        return -1;
    }

    uint32_t to_read = (file_inode.size < buf_size) ? file_inode.size : buf_size;
    uint32_t read_so_far = 0;
    uint8_t block_buf[BLOCK_SIZE];

    for (size_t b = 0; b < DIRECT_BLOCKS && read_so_far < to_read; b++) {
        uint32_t block_no = file_inode.blocks[b];
        if (block_no == 0)
            break;

        if (disk_read_block(disk, block_no, block_buf) != 0)
            return -1;

        uint32_t chunk = to_read - read_so_far;
        if (chunk > BLOCK_SIZE)
            chunk = BLOCK_SIZE;

        memcpy((uint8_t *)buf + read_so_far, block_buf, chunk);
        read_so_far += chunk;
    }

    *bytes_read = read_so_far;
    return 0;
}

/* ── file_write ────────────────────────────────────────────────────────── */

int file_write(Disk *disk, uint32_t file_inode_id, const void *data, uint32_t size)
{
    if (size > MAX_FILE_SIZE) {
        fprintf(stderr, "file: size %u exceeds maximum file size %u\n", size, MAX_FILE_SIZE);
        return -1;
    }

    Inode file_inode;
    if (inode_read(disk, file_inode_id, &file_inode) != 0)
        return -1;

    if (file_inode.type != INODE_FILE || !file_inode.used) {
        fprintf(stderr, "file: inode %u is not a regular file\n", file_inode_id);
        return -1;
    }

    uint32_t blocks_needed = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    /* Free any previously allocated blocks beyond what is needed */
    for (size_t b = blocks_needed; b < DIRECT_BLOCKS; b++) {
        if (file_inode.blocks[b] != 0) {
            bitmap_free(disk, file_inode.blocks[b]);
            file_inode.blocks[b] = 0;
        }
    }

    /* Allocate new blocks if more are needed */
    for (size_t b = 0; b < blocks_needed; b++) {
        if (file_inode.blocks[b] == 0) {
            uint32_t new_block;
            if (bitmap_alloc(disk, &new_block) != 0) {
                fprintf(stderr, "file: disk full while writing file\n");
                return -1;
            }
            file_inode.blocks[b] = new_block;
        }
    }

    /* Write data block by block */
    uint32_t written = 0;
    uint8_t block_buf[BLOCK_SIZE];

    for (size_t b = 0; b < blocks_needed; b++) {
        uint32_t chunk = size - written;
        if (chunk > BLOCK_SIZE)
            chunk = BLOCK_SIZE;

        memset(block_buf, 0, BLOCK_SIZE);
        if (data && chunk > 0) {
            memcpy(block_buf, (const uint8_t *)data + written, chunk);
        }

        if (disk_write_block(disk, file_inode.blocks[b], block_buf) != 0)
            return -1;

        written += chunk;
    }

    file_inode.size = size;
    return inode_write(disk, &file_inode);
}

/* ── file_delete ───────────────────────────────────────────────────────── */

int file_delete(Disk *disk, uint32_t dir_inode_id, const char *name)
{
    if (!name || strlen(name) == 0)
        return -1;

    uint32_t file_inode_id;
    if (dir_lookup(disk, dir_inode_id, name, &file_inode_id) != 0) {
        fprintf(stderr, "file: '%s' not found\n", name);
        return -1;
    }

    Inode file_inode;
    if (inode_read(disk, file_inode_id, &file_inode) != 0)
        return -1;

    if (file_inode.type != INODE_FILE) {
        fprintf(stderr, "file: '%s' is not a regular file\n", name);
        return -1;
    }

    /* Free all allocated data blocks */
    for (size_t b = 0; b < DIRECT_BLOCKS; b++) {
        if (file_inode.blocks[b] != 0) {
            bitmap_free(disk, file_inode.blocks[b]);
            file_inode.blocks[b] = 0;
        }
    }

    /* Free the inode */
    if (inode_free(disk, file_inode_id) != 0)
        return -1;

    /* Remove entry from parent directory */
    return dir_remove_entry(disk, dir_inode_id, name);
}
