/*
 * directory.c — Directory Entry Management Implementation
 *
 * Implements directory operations for kiwiFS.
 * A directory's data blocks contain arrays of DirectoryEntry structs.
 */

#include "directory.h"
#include "bitmap.h"

#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* ── dir_init ──────────────────────────────────────────────────────────── */

int dir_init(Disk *disk, uint32_t dir_inode_id, uint32_t parent_inode_id)
{
    Inode dir_inode;
    if (inode_read(disk, dir_inode_id, &dir_inode) != 0)
        return -1;

    /* Allocate one data block for the directory's initial entries */
    uint32_t data_block;
    if (bitmap_alloc(disk, &data_block) != 0) {
        fprintf(stderr, "directory: failed to allocate data block for dir inode %u\n", dir_inode_id);
        return -1;
    }

    /* Prepare the block: fill with DIR_ENTRY_UNUSED */
    DirectoryEntry entries[ENTRIES_PER_BLOCK];
    for (size_t i = 0; i < ENTRIES_PER_BLOCK; i++) {
        entries[i].inode_id = DIR_ENTRY_UNUSED;
        entries[i].name[0] = '\0';
    }

    /* Entry 0: "." -> self */
    entries[0].inode_id = dir_inode_id;
    strncpy(entries[0].name, ".", DIR_ENTRY_NAME_LEN - 1);
    entries[0].name[DIR_ENTRY_NAME_LEN - 1] = '\0';

    /* Entry 1: ".." -> parent */
    entries[1].inode_id = parent_inode_id;
    strncpy(entries[1].name, "..", DIR_ENTRY_NAME_LEN - 1);
    entries[1].name[DIR_ENTRY_NAME_LEN - 1] = '\0';

    if (disk_write_block(disk, data_block, entries) != 0) {
        bitmap_free(disk, data_block);
        return -1;
    }

    /* Update inode metadata */
    dir_inode.type = INODE_DIRECTORY;
    dir_inode.size = 2 * sizeof(DirectoryEntry);
    dir_inode.blocks[0] = data_block;
    for (size_t i = 1; i < DIRECT_BLOCKS; i++) {
        dir_inode.blocks[i] = 0;
    }
    dir_inode.used = 1;

    return inode_write(disk, &dir_inode);
}

/* ── dir_lookup ────────────────────────────────────────────────────────── */

int dir_lookup(Disk *disk, uint32_t dir_inode_id, const char *name, uint32_t *out_inode_id)
{
    if (!name || strlen(name) == 0 || strlen(name) > MAX_NAME_LEN)
        return -1;

    Inode dir_inode;
    if (inode_read(disk, dir_inode_id, &dir_inode) != 0)
        return -1;

    if (dir_inode.type != INODE_DIRECTORY || !dir_inode.used) {
        fprintf(stderr, "directory: inode %u is not a directory\n", dir_inode_id);
        return -1;
    }

    DirectoryEntry entries[ENTRIES_PER_BLOCK];

    for (size_t b = 0; b < DIRECT_BLOCKS; b++) {
        uint32_t block_no = dir_inode.blocks[b];
        if (block_no == 0)
            continue;

        if (disk_read_block(disk, block_no, entries) != 0)
            return -1;

        for (size_t i = 0; i < ENTRIES_PER_BLOCK; i++) {
            if (entries[i].inode_id != DIR_ENTRY_UNUSED &&
                strncmp(entries[i].name, name, DIR_ENTRY_NAME_LEN) == 0) {
                if (out_inode_id) {
                    *out_inode_id = entries[i].inode_id;
                }
                return 0;
            }
        }
    }

    return -1; /* Not found */
}

/* ── dir_add_entry ─────────────────────────────────────────────────────── */

int dir_add_entry(Disk *disk, uint32_t dir_inode_id, const char *name, uint32_t target_inode_id)
{
    if (!name || strlen(name) == 0 || strlen(name) > MAX_NAME_LEN) {
        fprintf(stderr, "directory: invalid name\n");
        return -1;
    }

    /* Check for duplicate entry */
    uint32_t existing;
    if (dir_lookup(disk, dir_inode_id, name, &existing) == 0) {
        fprintf(stderr, "directory: entry '%s' already exists\n", name);
        return -1;
    }

    Inode dir_inode;
    if (inode_read(disk, dir_inode_id, &dir_inode) != 0)
        return -1;

    DirectoryEntry entries[ENTRIES_PER_BLOCK];

    /* First pass: try to find an unused slot in existing allocated blocks */
    for (size_t b = 0; b < DIRECT_BLOCKS; b++) {
        uint32_t block_no = dir_inode.blocks[b];
        if (block_no == 0)
            continue;

        if (disk_read_block(disk, block_no, entries) != 0)
            return -1;

        for (size_t i = 0; i < ENTRIES_PER_BLOCK; i++) {
            if (entries[i].inode_id == DIR_ENTRY_UNUSED) {
                entries[i].inode_id = target_inode_id;
                strncpy(entries[i].name, name, DIR_ENTRY_NAME_LEN - 1);
                entries[i].name[DIR_ENTRY_NAME_LEN - 1] = '\0';

                if (disk_write_block(disk, block_no, entries) != 0)
                    return -1;

                dir_inode.size += sizeof(DirectoryEntry);
                return inode_write(disk, &dir_inode);
            }
        }
    }

    /* Second pass: allocate a new data block if direct blocks available */
    for (size_t b = 0; b < DIRECT_BLOCKS; b++) {
        if (dir_inode.blocks[b] == 0) {
            uint32_t new_block;
            if (bitmap_alloc(disk, &new_block) != 0) {
                fprintf(stderr, "directory: disk full, cannot allocate block for directory\n");
                return -1;
            }

            for (size_t i = 0; i < ENTRIES_PER_BLOCK; i++) {
                entries[i].inode_id = DIR_ENTRY_UNUSED;
                entries[i].name[0] = '\0';
            }

            entries[0].inode_id = target_inode_id;
            strncpy(entries[0].name, name, DIR_ENTRY_NAME_LEN - 1);
            entries[0].name[DIR_ENTRY_NAME_LEN - 1] = '\0';

            if (disk_write_block(disk, new_block, entries) != 0) {
                bitmap_free(disk, new_block);
                return -1;
            }

            dir_inode.blocks[b] = new_block;
            dir_inode.size += sizeof(DirectoryEntry);
            return inode_write(disk, &dir_inode);
        }
    }

    fprintf(stderr, "directory: directory inode %u is full\n", dir_inode_id);
    return -1;
}

/* ── dir_remove_entry ──────────────────────────────────────────────────── */

int dir_remove_entry(Disk *disk, uint32_t dir_inode_id, const char *name)
{
    if (!name || strcmp(name, ".") == 0 || strcmp(name, "..") == 0) {
        fprintf(stderr, "directory: cannot remove special entry '.' or '..'\n");
        return -1;
    }

    Inode dir_inode;
    if (inode_read(disk, dir_inode_id, &dir_inode) != 0)
        return -1;

    DirectoryEntry entries[ENTRIES_PER_BLOCK];

    for (size_t b = 0; b < DIRECT_BLOCKS; b++) {
        uint32_t block_no = dir_inode.blocks[b];
        if (block_no == 0)
            continue;

        if (disk_read_block(disk, block_no, entries) != 0)
            return -1;

        for (size_t i = 0; i < ENTRIES_PER_BLOCK; i++) {
            if (entries[i].inode_id != DIR_ENTRY_UNUSED &&
                strncmp(entries[i].name, name, DIR_ENTRY_NAME_LEN) == 0) {

                entries[i].inode_id = DIR_ENTRY_UNUSED;
                entries[i].name[0] = '\0';

                if (disk_write_block(disk, block_no, entries) != 0)
                    return -1;

                if (dir_inode.size >= sizeof(DirectoryEntry)) {
                    dir_inode.size -= sizeof(DirectoryEntry);
                }
                return inode_write(disk, &dir_inode);
            }
        }
    }

    fprintf(stderr, "directory: entry '%s' not found in directory\n", name);
    return -1;
}

/* ── dir_is_empty ──────────────────────────────────────────────────────── */

int dir_is_empty(Disk *disk, uint32_t dir_inode_id)
{
    Inode dir_inode;
    if (inode_read(disk, dir_inode_id, &dir_inode) != 0)
        return -1;

    if (dir_inode.type != INODE_DIRECTORY)
        return -1;

    DirectoryEntry entries[ENTRIES_PER_BLOCK];

    for (size_t b = 0; b < DIRECT_BLOCKS; b++) {
        uint32_t block_no = dir_inode.blocks[b];
        if (block_no == 0)
            continue;

        if (disk_read_block(disk, block_no, entries) != 0)
            return -1;

        for (size_t i = 0; i < ENTRIES_PER_BLOCK; i++) {
            if (entries[i].inode_id != DIR_ENTRY_UNUSED) {
                if (strcmp(entries[i].name, ".") != 0 && strcmp(entries[i].name, "..") != 0) {
                    return 0; /* Contains at least one other file or directory */
                }
            }
        }
    }

    return 1; /* Empty */
}
