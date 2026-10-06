/*
 * fs.h — Filesystem API and Path Resolution Interface
 *
 * Provides the top-level filesystem abstraction, path resolution,
 * formatting, mounting, directory navigation, and file operations.
 */

#ifndef KIWI_FS_H
#define KIWI_FS_H

#include <stdint.h>
#include <stddef.h>
#include "disk.h"
#include "superblock.h"
#include "bitmap.h"
#include "inode.h"
#include "directory.h"
#include "file.h"

/* ── Constants ─────────────────────────────────────────────────────────── */

#define MAX_PATH_LEN  512u

/* ── Filesystem Handle ─────────────────────────────────────────────────── */

typedef struct {
    Disk       disk;
    Superblock sb;
    uint32_t   current_dir_inode;    /* current working directory inode */
    char       current_path[MAX_PATH_LEN]; /* cached cwd string (e.g. "/projects") */
} Filesystem;

/* Directory entry callback for listing */
typedef void (*fs_ls_callback)(const char *name, uint32_t inode_id, uint32_t type, uint32_t size, void *user_data);

/* ── Lifecycle API ─────────────────────────────────────────────────────── */

/*
 * fs_format - create and initialize a fresh kiwiFS filesystem image.
 *
 * Writes superblock, zeros bitmap, marks system metadata blocks as allocated,
 * initializes inode table, and sets up root directory at inode 0.
 *
 * Returns 0 on success, -1 on failure.
 */
int fs_format(const char *image_path);

/*
 * fs_mount - open and validate an existing kiwiFS image.
 *
 * Verifies magic number and initializes cwd to "/".
 * Returns 0 on success, -1 on failure.
 */
int fs_mount(const char *image_path, Filesystem *fs);

/*
 * fs_unmount - flush buffers and close the mounted filesystem.
 */
void fs_unmount(Filesystem *fs);

/* ── Path Resolution API ───────────────────────────────────────────────── */

/*
 * fs_resolve_path - traverse path components to find target inode ID.
 *
 * Supports absolute ("/a/b"), relative ("b/c"), and special (".", "..") paths.
 * Returns 0 on success, -1 if any component does not exist.
 */
int fs_resolve_path(Filesystem *fs, const char *path, uint32_t *out_inode_id);

/*
 * fs_resolve_parent - split path into parent directory inode and base filename.
 *
 * e.g., "/a/b/c.txt" -> parent inode for "/a/b", out_name = "c.txt".
 * Returns 0 on success, -1 on failure.
 */
int fs_resolve_parent(Filesystem *fs, const char *path, uint32_t *parent_inode_id, char *out_name);

/* ── High-Level Operations ─────────────────────────────────────────────── */

int fs_mkdir(Filesystem *fs, const char *path);
int fs_touch(Filesystem *fs, const char *path);
int fs_cat(Filesystem *fs, const char *path, char *buf, uint32_t buf_size, uint32_t *bytes_read);
int fs_write(Filesystem *fs, const char *path, const char *text);
int fs_rm(Filesystem *fs, const char *path);
int fs_cd(Filesystem *fs, const char *path);
int fs_pwd(Filesystem *fs, char *buf, size_t buf_size);
int fs_ls(Filesystem *fs, const char *path, fs_ls_callback cb, void *user_data);

#endif /* KIWI_FS_H */
