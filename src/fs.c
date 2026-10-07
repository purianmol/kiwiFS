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

/* ── High-Level Operations ─────────────────────────────────────────────── */

int fs_mkdir(Filesystem *fs, const char *path)
{
    if (!fs || !path)
        return -1;

    uint32_t parent_inode_id;
    char name[MAX_NAME_LEN + 1];

    if (fs_resolve_parent(fs, path, &parent_inode_id, name) != 0) {
        fprintf(stderr, "mkdir: cannot create directory '%s': No such file or directory\n", path);
        return -1;
    }

    uint32_t existing;
    if (dir_lookup(&fs->disk, parent_inode_id, name, &existing) == 0) {
        fprintf(stderr, "mkdir: cannot create directory '%s': File exists\n", path);
        return -1;
    }

    uint32_t new_dir_inode;
    if (inode_alloc(&fs->disk, INODE_DIRECTORY, &new_dir_inode) != 0) {
        fprintf(stderr, "mkdir: failed to allocate inode\n");
        return -1;
    }

    if (dir_init(&fs->disk, new_dir_inode, parent_inode_id) != 0) {
        inode_free(&fs->disk, new_dir_inode);
        fprintf(stderr, "mkdir: failed to initialize directory data\n");
        return -1;
    }

    if (dir_add_entry(&fs->disk, parent_inode_id, name, new_dir_inode) != 0) {
        inode_free(&fs->disk, new_dir_inode);
        fprintf(stderr, "mkdir: failed to add directory entry\n");
        return -1;
    }

    return 0;
}

int fs_touch(Filesystem *fs, const char *path)
{
    if (!fs || !path)
        return -1;

    uint32_t parent_inode_id;
    char name[MAX_NAME_LEN + 1];

    if (fs_resolve_parent(fs, path, &parent_inode_id, name) != 0) {
        fprintf(stderr, "touch: cannot touch '%s': No such file or directory\n", path);
        return -1;
    }

    uint32_t existing;
    if (dir_lookup(&fs->disk, parent_inode_id, name, &existing) == 0) {
        /* File already exists; touch succeeds without re-creating */
        return 0;
    }

    uint32_t file_inode;
    return file_create(&fs->disk, parent_inode_id, name, &file_inode);
}

int fs_write(Filesystem *fs, const char *path, const char *text)
{
    if (!fs || !path)
        return -1;

    uint32_t file_inode_id;
    if (fs_resolve_path(fs, path, &file_inode_id) != 0) {
        /* If file does not exist, create it first */
        if (fs_touch(fs, path) != 0)
            return -1;
        if (fs_resolve_path(fs, path, &file_inode_id) != 0)
            return -1;
    }

    uint32_t len = text ? (uint32_t)strlen(text) : 0;
    return file_write(&fs->disk, file_inode_id, text, len);
}

int fs_cat(Filesystem *fs, const char *path, char *buf, uint32_t buf_size, uint32_t *bytes_read)
{
    if (!fs || !path || !buf || !bytes_read)
        return -1;

    uint32_t file_inode_id;
    if (fs_resolve_path(fs, path, &file_inode_id) != 0) {
        fprintf(stderr, "cat: '%s': No such file or directory\n", path);
        return -1;
    }

    return file_read(&fs->disk, file_inode_id, buf, buf_size, bytes_read);
}

int fs_rm(Filesystem *fs, const char *path)
{
    if (!fs || !path)
        return -1;

    uint32_t parent_inode_id;
    char name[MAX_NAME_LEN + 1];

    if (fs_resolve_parent(fs, path, &parent_inode_id, name) != 0) {
        fprintf(stderr, "rm: cannot remove '%s': No such file or directory\n", path);
        return -1;
    }

    return file_delete(&fs->disk, parent_inode_id, name);
}

/* Helper to normalize path for fs_cd */
static void normalize_path(char *dst, size_t dst_size, const char *current, const char *input)
{
    char temp[MAX_PATH_LEN * 2];
    if (input[0] == '/') {
        strncpy(temp, input, sizeof(temp) - 1);
    } else {
        if (strcmp(current, "/") == 0) {
            snprintf(temp, sizeof(temp), "/%s", input);
        } else {
            snprintf(temp, sizeof(temp), "%s/%s", current, input);
        }
    }
    temp[sizeof(temp) - 1] = '\0';

    /* Parse components */
    char *stack[64];
    int top = 0;

    char *token = strtok(temp, "/");
    while (token != NULL) {
        if (strcmp(token, ".") == 0) {
            /* ignore */
        } else if (strcmp(token, "..") == 0) {
            if (top > 0) {
                top--;
            }
        } else {
            if (top < 64) {
                stack[top++] = token;
            }
        }
        token = strtok(NULL, "/");
    }

    if (top == 0) {
        strncpy(dst, "/", dst_size - 1);
        dst[dst_size - 1] = '\0';
        return;
    }

    dst[0] = '\0';
    for (int i = 0; i < top; i++) {
        strncat(dst, "/", dst_size - strlen(dst) - 1);
        strncat(dst, stack[i], dst_size - strlen(dst) - 1);
    }
}

int fs_cd(Filesystem *fs, const char *path)
{
    if (!fs)
        return -1;

    if (!path || strlen(path) == 0) {
        /* cd with no args returns to root */
        fs->current_dir_inode = fs->sb.root_inode;
        strncpy(fs->current_path, "/", sizeof(fs->current_path) - 1);
        return 0;
    }

    uint32_t target_inode_id;
    if (fs_resolve_path(fs, path, &target_inode_id) != 0) {
        fprintf(stderr, "cd: '%s': No such file or directory\n", path);
        return -1;
    }

    Inode target_inode;
    if (inode_read(&fs->disk, target_inode_id, &target_inode) != 0)
        return -1;

    if (target_inode.type != INODE_DIRECTORY) {
        fprintf(stderr, "cd: '%s': Not a directory\n", path);
        return -1;
    }

    fs->current_dir_inode = target_inode_id;

    char new_path[MAX_PATH_LEN];
    normalize_path(new_path, sizeof(new_path), fs->current_path, path);
    strncpy(fs->current_path, new_path, sizeof(fs->current_path) - 1);
    fs->current_path[sizeof(fs->current_path) - 1] = '\0';

    return 0;
}

int fs_pwd(Filesystem *fs, char *buf, size_t buf_size)
{
    if (!fs || !buf)
        return -1;

    strncpy(buf, fs->current_path, buf_size - 1);
    buf[buf_size - 1] = '\0';
    return 0;
}

int fs_ls(Filesystem *fs, const char *path, fs_ls_callback cb, void *user_data)
{
    if (!fs)
        return -1;

    uint32_t dir_inode_id;
    if (!path || strlen(path) == 0) {
        dir_inode_id = fs->current_dir_inode;
    } else {
        if (fs_resolve_path(fs, path, &dir_inode_id) != 0) {
            fprintf(stderr, "ls: '%s': No such file or directory\n", path);
            return -1;
        }
    }

    Inode dir_inode;
    if (inode_read(&fs->disk, dir_inode_id, &dir_inode) != 0)
        return -1;

    if (dir_inode.type != INODE_DIRECTORY) {
        fprintf(stderr, "ls: '%s': Not a directory\n", path ? path : fs->current_path);
        return -1;
    }

    DirectoryEntry entries[ENTRIES_PER_BLOCK];

    for (size_t b = 0; b < DIRECT_BLOCKS; b++) {
        uint32_t block_no = dir_inode.blocks[b];
        if (block_no == 0)
            continue;

        if (disk_read_block(&fs->disk, block_no, entries) != 0)
            return -1;

        for (size_t i = 0; i < ENTRIES_PER_BLOCK; i++) {
            if (entries[i].inode_id != DIR_ENTRY_UNUSED) {
                if (cb) {
                    Inode item_inode;
                    if (inode_read(&fs->disk, entries[i].inode_id, &item_inode) == 0) {
                        cb(entries[i].name, entries[i].inode_id, item_inode.type, item_inode.size, user_data);
                    }
                }
            }
        }
    }

    return 0;
}
