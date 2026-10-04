/*
 * directory.h — Directory Entry Structure and Operations
 *
 * A directory in kiwiFS is an inode (type INODE_DIRECTORY) whose data
 * blocks contain an array of DirectoryEntry structs (32 bytes each).
 *
 * Each entry maps a human-readable name (up to MAX_NAME_LEN = 27 chars)
 * to an inode number.  An entry with inode_id == 0 is considered empty/unused,
 * EXCEPT for the root directory entry named "." or ".." which points to inode 0.
 * To distinguish active vs unused entries, we use DIR_ENTRY_UNUSED (0xFFFFFFFF).
 */

#ifndef KIWI_DIRECTORY_H
#define KIWI_DIRECTORY_H

#include <stdint.h>
#include <stdbool.h>
#include "disk.h"
#include "inode.h"

/* ── Constants ─────────────────────────────────────────────────────────── */

#define MAX_NAME_LEN        27u
#define DIR_ENTRY_NAME_LEN  28u   /* including null terminator */

#define DIR_ENTRY_UNUSED    0xFFFFFFFFu

#define ENTRIES_PER_BLOCK   (BLOCK_SIZE / sizeof(DirectoryEntry))  /* 128 */
#define MAX_DIR_ENTRIES     (DIRECT_BLOCKS * ENTRIES_PER_BLOCK)    /* 1024 */

/* ── On-disk structure ─────────────────────────────────────────────────── */

/*
 * DirectoryEntry - 32-byte fixed-size record mapping name -> inode.
 */
typedef struct {
    uint32_t inode_id;                 /* target inode number or DIR_ENTRY_UNUSED */
    char     name[DIR_ENTRY_NAME_LEN]; /* null-terminated filename (max 27 chars) */
} DirectoryEntry;

/* ── API ───────────────────────────────────────────────────────────────── */

/*
 * dir_init - initialize a newly allocated directory inode.
 *
 * Allocates a data block for `dir_inode_id`, links "." (to dir_inode_id)
 * and ".." (to parent_inode_id), updates the directory inode, and persists it.
 *
 * Returns 0 on success, -1 on failure.
 */
int dir_init(Disk *disk, uint32_t dir_inode_id, uint32_t parent_inode_id);

/*
 * dir_lookup - search directory `dir_inode_id` for an entry with `name`.
 *
 * Stores the found inode number in *out_inode_id.
 * Returns 0 on success, -1 if not found or on error.
 */
int dir_lookup(Disk *disk, uint32_t dir_inode_id, const char *name, uint32_t *out_inode_id);

/*
 * dir_add_entry - add a (name -> target_inode_id) mapping to `dir_inode_id`.
 *
 * Fails if `name` already exists, exceeds MAX_NAME_LEN, or if directory is full.
 * Allocates new data blocks as needed.
 * Returns 0 on success, -1 on failure.
 */
int dir_add_entry(Disk *disk, uint32_t dir_inode_id, const char *name, uint32_t target_inode_id);

/*
 * dir_remove_entry - remove `name` from directory `dir_inode_id`.
 *
 * Cannot remove "." or "..".
 * Returns 0 on success, -1 if not found or invalid operation.
 */
int dir_remove_entry(Disk *disk, uint32_t dir_inode_id, const char *name);

/*
 * dir_is_empty - check if directory contains only "." and ".." entries.
 *
 * Returns 1 if empty, 0 if it contains other files/subdirectories, -1 on error.
 */
int dir_is_empty(Disk *disk, uint32_t dir_inode_id);

#endif /* KIWI_DIRECTORY_H */
