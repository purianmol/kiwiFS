/*
 * disk.c — Virtual Disk Layer Implementation
 *
 * This is the ONLY file in kiwiFS allowed to call fread, fwrite,
 * fseek, or fclose directly.  All higher layers must go through
 * disk_read_block() and disk_write_block().
 */

#include "disk.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdint.h>

/* ── disk_create ───────────────────────────────────────────────────────── */

int disk_create(const char *path, Disk *out)
{
    out->fp = fopen(path, "w+b");
    if (!out->fp) {
        fprintf(stderr, "disk: cannot create '%s': %s\n", path, strerror(errno));
        return -1;
    }

    /* zero-fill the entire disk image so every block starts clean */
    uint8_t zero_block[BLOCK_SIZE];
    memset(zero_block, 0, sizeof(zero_block));

    for (uint32_t i = 0; i < TOTAL_BLOCKS; i++) {
        if (fwrite(zero_block, 1, BLOCK_SIZE, out->fp) != BLOCK_SIZE) {
            fprintf(stderr, "disk: failed to zero block %u: %s\n",
                    i, strerror(errno));
            fclose(out->fp);
            out->fp = NULL;
            return -1;
        }
    }

    rewind(out->fp);

    strncpy(out->path, path, sizeof(out->path) - 1);
    out->path[sizeof(out->path) - 1] = '\0';

    return 0;
}

/* ── disk_open ─────────────────────────────────────────────────────────── */

int disk_open(const char *path, Disk *out)
{
    out->fp = fopen(path, "r+b");
    if (!out->fp) {
        fprintf(stderr, "disk: cannot open '%s': %s\n", path, strerror(errno));
        return -1;
    }

    strncpy(out->path, path, sizeof(out->path) - 1);
    out->path[sizeof(out->path) - 1] = '\0';

    return 0;
}

/* ── Internal helper ───────────────────────────────────────────────────── */

/*
 * seek_to_block - seek the file pointer to the start of block_no.
 * Returns 0 on success, -1 if the block number is out of range or fseek fails.
 */
static int seek_to_block(Disk *d, uint32_t block_no)
{
    if (block_no >= TOTAL_BLOCKS) {
        fprintf(stderr, "disk: block %u out of range (max %u)\n",
                block_no, TOTAL_BLOCKS - 1);
        return -1;
    }

    long offset = (long)block_no * BLOCK_SIZE;
    if (fseek(d->fp, offset, SEEK_SET) != 0) {
        fprintf(stderr, "disk: fseek to block %u failed: %s\n",
                block_no, strerror(errno));
        return -1;
    }
    return 0;
}

/* ── disk_close ────────────────────────────────────────────────────────── */

void disk_close(Disk *d)
{
    if (d && d->fp) {
        fflush(d->fp);
        fclose(d->fp);
        d->fp = NULL;
    }
}

/* ── disk_read_block ───────────────────────────────────────────────────── */

int disk_read_block(Disk *d, uint32_t block_no, void *buf)
{
    if (seek_to_block(d, block_no) != 0)
        return -1;

    if (fread(buf, 1, BLOCK_SIZE, d->fp) != BLOCK_SIZE) {
        fprintf(stderr, "disk: short read on block %u: %s\n",
                block_no, strerror(errno));
        return -1;
    }
    return 0;
}

/* ── disk_write_block ──────────────────────────────────────────────────── */

int disk_write_block(Disk *d, uint32_t block_no, const void *buf)
{
    if (seek_to_block(d, block_no) != 0)
        return -1;

    if (fwrite(buf, 1, BLOCK_SIZE, d->fp) != BLOCK_SIZE) {
        fprintf(stderr, "disk: short write on block %u: %s\n",
                block_no, strerror(errno));
        return -1;
    }

    fflush(d->fp);
    return 0;
}
