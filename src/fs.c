/*
 * fs.c — Filesystem API and Path Resolution Implementation
 */

#include "fs.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ── fs_format ─────────────────────────────────────────────────────────── */

int fs_format(const char *image_path)
{
    Disk disk;
    if (disk_create(image_path, &disk) != 0) {
        fprintf(stderr, "fs: failed to create disk image '%s'\n", image_path);
        return -1;
    }

    /* 1. Initialize and write Superblock */
    Superblock sb;
    memset(&sb, 0, sizeof(sb));
    sb.magic         = FS_MAGIC;
    sb.block_size    = BLOCK_SIZE;
    sb.total_blocks  = TOTAL_BLOCKS;
    sb.bitmap_start  = BITMAP_BLOCK;
    sb.bitmap_blocks = BITMAP_BLOCKS;
    sb.inode_start   = INODE_START_BLOCK;
    sb.inode_blocks  = INODE_BLOCKS;
    sb.data_start    = INODE_START_BLOCK + INODE_BLOCKS;
    sb.root_inode    = 0;

    if (sb_write(&disk, &sb) != 0) {
        disk_close(&disk);
        return -1;
    }

    /* 2. Initialize Bitmap */
    if (bitmap_init(&disk) != 0) {
        disk_close(&disk);
        return -1;
    }

    /* Mark metadata blocks (superblock, bitmap, inode table) as allocated */
    uint32_t metadata_blocks = sb.data_start;
    for (uint32_t i = 0; i < metadata_blocks; i++) {
        uint32_t allocated_block;
        if (bitmap_alloc(&disk, &allocated_block) != 0 || allocated_block != i) {
            fprintf(stderr, "fs: failed to reserve metadata block %u\n", i);
            disk_close(&disk);
            return -1;
        }
    }

    /* 3. Initialize Inode Table */
    if (inode_table_init(&disk) != 0) {
        disk_close(&disk);
        return -1;
    }

    /* 4. Allocate and initialize Root Directory (inode 0) */
    uint32_t root_inode_id;
    if (inode_alloc(&disk, INODE_DIRECTORY, &root_inode_id) != 0 || root_inode_id != 0) {
        fprintf(stderr, "fs: failed to allocate root directory inode\n");
        disk_close(&disk);
        return -1;
    }

    if (dir_init(&disk, root_inode_id, root_inode_id) != 0) {
        fprintf(stderr, "fs: failed to initialize root directory data\n");
        disk_close(&disk);
        return -1;
    }

    disk_close(&disk);
    return 0;
}

/* ── fs_mount / fs_unmount ─────────────────────────────────────────────── */

int fs_mount(const char *image_path, Filesystem *fs)
{
    if (disk_open(image_path, &fs->disk) != 0) {
        return -1;
    }

    if (sb_read(&fs->disk, &fs->sb) != 0) {
        disk_close(&fs->disk);
        return -1;
    }

    if (sb_validate(&fs->sb) != 0) {
        disk_close(&fs->disk);
        return -1;
    }

    fs->current_dir_inode = fs->sb.root_inode;
    strncpy(fs->current_path, "/", sizeof(fs->current_path) - 1);
    fs->current_path[sizeof(fs->current_path) - 1] = '\0';

    return 0;
}

void fs_unmount(Filesystem *fs)
{
    if (fs) {
        disk_close(&fs->disk);
    }
}

/* ── Path Resolution Helpers ───────────────────────────────────────────── */

int fs_resolve_path(Filesystem *fs, const char *path, uint32_t *out_inode_id)
{
    if (!fs || !path || strlen(path) == 0)
        return -1;

    uint32_t curr_inode;
    if (path[0] == '/') {
        curr_inode = fs->sb.root_inode;
    } else {
        curr_inode = fs->current_dir_inode;
    }

    char path_copy[MAX_PATH_LEN];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    char *token = strtok(path_copy, "/");
    while (token != NULL) {
        uint32_t next_inode;
        if (dir_lookup(&fs->disk, curr_inode, token, &next_inode) != 0) {
            return -1; /* Path component not found */
        }
        curr_inode = next_inode;
        token = strtok(NULL, "/");
    }

    if (out_inode_id) {
        *out_inode_id = curr_inode;
    }
    return 0;
}

int fs_resolve_parent(Filesystem *fs, const char *path, uint32_t *parent_inode_id, char *out_name)
{
    if (!fs || !path || strlen(path) == 0 || !parent_inode_id || !out_name)
        return -1;

    char path_copy[MAX_PATH_LEN];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    /* Strip trailing slashes unless path is just "/" */
    size_t len = strlen(path_copy);
    while (len > 1 && path_copy[len - 1] == '/') {
        path_copy[len - 1] = '\0';
        len--;
    }

    char *last_slash = strrchr(path_copy, '/');
    if (!last_slash) {
        /* Relative path in current directory, e.g. "myfile.txt" */
        *parent_inode_id = fs->current_dir_inode;
        strncpy(out_name, path_copy, MAX_NAME_LEN);
        out_name[MAX_NAME_LEN] = '\0';
        return 0;
    }

    if (last_slash == path_copy) {
        /* Direct child of root, e.g. "/myfile.txt" */
        *parent_inode_id = fs->sb.root_inode;
        strncpy(out_name, last_slash + 1, MAX_NAME_LEN);
        out_name[MAX_NAME_LEN] = '\0';
        return 0;
    }

    /* Split dirname and basename */
    *last_slash = '\0';
    const char *dir_part = path_copy;
    const char *base_part = last_slash + 1;

    if (fs_resolve_path(fs, dir_part, parent_inode_id) != 0) {
        return -1;
    }

    strncpy(out_name, base_part, MAX_NAME_LEN);
    out_name[MAX_NAME_LEN] = '\0';
    return 0;
}
