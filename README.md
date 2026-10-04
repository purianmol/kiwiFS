# kiwiFS 🥝

> A small, educational userspace filesystem simulator written in C11.

[![Language](https://img.shields.io/badge/Language-C11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))
[![Build](https://img.shields.io/badge/Build-Make-green.svg)](https://www.gnu.org/software/make/)
[![License](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Linux-orange.svg)](https://kernel.org/)

---

## Table of Contents

1. [Overview](#overview)
2. [Project Story & Motivation](#project-story--motivation)
3. [Goals](#goals)
4. [Features](#features)
5. [Architecture](#architecture)
6. [Filesystem Layout](#filesystem-layout)
7. [Project Structure](#project-structure)
8. [Prerequisites](#prerequisites)
9. [Building the Project](#building-the-project)
10. [Formatting a Filesystem](#formatting-a-filesystem)
11. [Running kiwiFS](#running-kiwifs)
12. [Supported Commands](#supported-commands)
13. [Example Session](#example-session)
14. [Design Decisions](#design-decisions)
15. [Limitations (v1)](#limitations-v1)
16. [Testing](#testing)
17. [Future Improvements (v2+)](#future-improvements-v2)
18. [License](#license)

---

## Overview

**kiwiFS** is a userspace filesystem simulator built entirely in C11. It creates and manages a virtual 4 MB disk image (`disk.img`) that contains a complete, self-contained filesystem — including a superblock, a block bitmap, an inode table, directory structures, and file data.

The host operating system sees only a single file: `disk.img`. Everything else — every file you create, every directory you make, every byte you write — lives inside that image, managed entirely by kiwiFS.

```
Host Linux Filesystem
        │
        ▼
    disk.img  (4 MB binary file)
        │
        ▼
     kiwiFS
   ┌───────────┐
   │ Superblock│   ← filesystem metadata
   │ Bitmap    │   ← free-space tracking
   │ Inodes    │   ← file/directory metadata
   │ Directories│  ← name → inode mappings
   │ File data │   ← actual file contents
   └───────────┘
```

---

## Project Story & Motivation

Every time we use a computer, we're relying on a filesystem — an invisible layer that organizes our data into files and directories on a physical disk. But how does it actually work? How does the OS know where a file starts, how big it is, or which disk blocks it occupies?

**kiwiFS** was born from the curiosity to answer these questions by building a filesystem from scratch.

The name *kiwi* reflects a simple, honest, self-contained thing: a kiwi fruit is compact, green inside, and holds everything it needs within its own skin. Similarly, kiwiFS holds its entire world inside a single binary file.

This project demonstrates real operating-systems concepts — **block allocation**, **inodes**, **directories**, and **persistence** — through a clean, beginner-readable C implementation. It is designed to be a strong interview artifact: something you can walk through line-by-line and explain every design decision.

---

## Goals

The goals of this project are:

1. **Educational** — Understand how real filesystems (ext2, FAT32) work by building one from scratch.
2. **Demonstrable** — Produce a working interactive shell that can create, read, write, and delete files and directories.
3. **Persistent** — Data written in one session must survive program restarts (it lives in `disk.img`).
4. **Modular** — Each filesystem layer (disk, superblock, bitmap, inodes, directories, files) is implemented in its own isolated module.
5. **Testable** — Unit and integration tests verify each subsystem independently.
6. **Interview-ready** — Clean code, meaningful commits, proper documentation, and the ability to defend every design decision.

### Specific Technical Goals (v1)

| Goal | Description |
|------|-------------|
| Virtual disk | Simulate a 4 MB disk as a binary file (`disk.img`) |
| Block management | Divide disk into 1024 fixed-size 4096-byte blocks |
| Free-space tracking | Manage free/allocated blocks with a bitmap |
| Inode table | Track file/directory metadata with inodes |
| Directory entries | Map filenames to inodes |
| File I/O | Create, write, read, and delete files |
| Directory navigation | Create directories, `cd`, `ls`, `pwd` |
| Path resolution | Support absolute and relative paths including `.` and `..` |
| Persistence | All state is stored in `disk.img` and survives restarts |
| Interactive shell | A `kiwifs>` prompt for issuing filesystem commands |

---

## Features

- **Virtual disk** backed by a single binary file (`disk.img`)
- **Superblock** with magic-number validation
- **Block bitmap** for efficient free-space management
- **Inode table** for file and directory metadata (direct block pointers)
- **Directory entries** mapping names (up to 27 characters) to inodes
- **Root directory** (`/`) created automatically during format
- **Persistent storage** — all changes survive program restart
- **Interactive CLI** with intuitive commands
- **Path resolution** supporting `/`, `.`, `..`, relative and absolute paths
- **Error handling** — never crashes on user errors; always reports meaningful messages
- **Unit tests** for every filesystem subsystem
- **Integration tests** including persistence verification

---

## Architecture

kiwiFS is organized in clear, layered modules. Each layer only depends on layers below it:

```
┌─────────────────────────────────────┐
│         CLI Shell (shell.c)         │  ← user-facing interface
├─────────────────────────────────────┤
│    Filesystem API + Paths (fs.c)    │  ← path resolution, high-level ops
├───────────────┬─────────────────────┤
│   Files       │    Directories      │  ← file.c, directory.c
│   (file.c)    │    (directory.c)    │
├───────────────┴─────────────────────┤
│          Inodes (inode.c)           │  ← inode allocation/read/write
├─────────────────────────────────────┤
│          Bitmap (bitmap.c)          │  ← block allocation tracking
├─────────────────────────────────────┤
│       Superblock (superblock.c)     │  ← filesystem metadata & layout
├─────────────────────────────────────┤
│       Virtual Disk (disk.c)         │  ← raw block I/O, only layer touching disk
└─────────────────────────────────────┘
              disk.img
```

### Module Responsibilities

| Module | File(s) | Responsibility |
|--------|---------|----------------|
| **Disk layer** | `disk.c`, `disk.h` | Open/close `disk.img`; raw block read/write. The **only** layer that calls `fread`/`fwrite`/`fseek`. |
| **Superblock** | `superblock.c`, `superblock.h` | Stores filesystem metadata (magic, sizes, layout offsets). Validated on every mount. |
| **Bitmap** | `bitmap.c`, `bitmap.h` | Tracks which blocks are free/allocated. Provides `bitmap_allocate()` and `bitmap_free()`. |
| **Inodes** | `inode.c`, `inode.h` | Stores per-file/dir metadata: type, size, direct block pointers. Provides allocate/free/read/write. |
| **Directories** | `directory.c`, `directory.h` | Manages `DirectoryEntry` arrays inside directory data blocks. Provides lookup/add/remove. |
| **Files** | `file.c`, `file.h` | Implements `touch`, `write`, `cat`, `rm` at the filesystem level. |
| **Filesystem API** | `fs.c`, `fs.h` | Path resolution (`resolve_path`), ties all modules together. |
| **Shell** | `shell.c`, `shell.h` | REPL loop: reads commands, dispatches to the filesystem API, prints results. |

---

## Filesystem Layout

The 4 MB virtual disk is organized as follows:

```
Block 0        ┌──────────────────────┐
               │     Superblock       │  ← magic, sizes, layout info
               └──────────────────────┘
Block 1+       ┌──────────────────────┐
               │    Block Bitmap      │  ← 1 bit per block
               └──────────────────────┘
Next blocks    ┌──────────────────────┐
               │    Inode Table       │  ← fixed array of Inode structs
               └──────────────────────┘
Remaining      ┌──────────────────────┐
               │    Data Blocks       │
               │                      │  ← file contents, directory data
               └──────────────────────┘
```

### Key Constants

| Constant | Value | Meaning |
|----------|-------|---------|
| `BLOCK_SIZE` | 4096 bytes | Size of one disk block |
| `TOTAL_BLOCKS` | 1024 | Total number of blocks |
| `DISK_SIZE` | 4,194,304 bytes (4 MB) | Total virtual disk size |
| `DIRECT_BLOCKS` | 8 | Data block pointers per inode |
| `MAX_FILE_SIZE` | 32,768 bytes (32 KB) | Maximum file size in v1 |
| `MAX_NAME_LEN` | 27 chars | Maximum filename length |
| `FS_MAGIC` | `0x4B575331` | Magic number ("KWS1" = kiwiFS v1) |

### Superblock Structure

```c
typedef struct {
    uint32_t magic;           // 0x4B575331 ("KWS1")
    uint32_t block_size;      // 4096
    uint32_t total_blocks;    // 1024

    uint32_t bitmap_start;    // block number where bitmap begins
    uint32_t bitmap_blocks;   // number of blocks used by bitmap

    uint32_t inode_start;     // block number where inode table begins
    uint32_t inode_blocks;    // number of blocks used by inode table

    uint32_t data_start;      // block number where data blocks begin

    uint32_t root_inode;      // inode ID of root directory (always 0)
} Superblock;
```

### Inode Structure

```c
typedef struct {
    uint32_t id;                    // inode number
    uint32_t type;                  // INODE_FREE, INODE_FILE, INODE_DIRECTORY
    uint32_t size;                  // file size in bytes
    uint32_t blocks[DIRECT_BLOCKS]; // up to 8 direct data block references
    uint32_t used;                  // 1 = allocated, 0 = free
} Inode;
```

### Directory Entry Structure

```c
typedef struct {
    uint32_t inode_id;      // inode number this entry points to
    char     name[28];      // null-terminated filename (max 27 chars)
} DirectoryEntry;
```

---

## Project Structure

```
kiwiFS/
│
├── include/             ← Public header files
│   ├── fs.h             ← Filesystem API and path resolution
│   ├── disk.h           ← Virtual disk layer interface
│   ├── superblock.h     ← Superblock structure and operations
│   ├── bitmap.h         ← Block bitmap interface
│   ├── inode.h          ← Inode structure and operations
│   ├── directory.h      ← Directory entry structure and operations
│   ├── file.h           ← File operation interface
│   └── shell.h          ← Interactive shell interface
│
├── src/                 ← Implementation files
│   ├── main.c           ← Entry point: argument parsing, format/mount
│   ├── fs.c             ← Path resolution, high-level filesystem API
│   ├── disk.c           ← Raw disk I/O (the only module using fread/fwrite)
│   ├── superblock.c     ← Superblock read/write/validate
│   ├── bitmap.c         ← Block allocation bitmap
│   ├── inode.c          ← Inode management
│   ├── directory.c      ← Directory entry management
│   ├── file.c           ← File operations (touch, write, cat, rm)
│   └── shell.c          ← Interactive REPL shell
│
├── tests/               ← Unit and integration tests
│   ├── test_disk.c      ← Disk layer tests
│   ├── test_bitmap.c    ← Bitmap allocation tests
│   ├── test_inode.c     ← Inode management tests
│   ├── test_directory.c ← Directory entry tests
│   └── test_file.c      ← File operation tests
│
├── .gitignore           ← Ignores binaries, disk.img, IDE files
├── Makefile             ← Build system (make, make test, make clean)
├── LICENSE              ← MIT License
└── README.md            ← This file
```

---

## Prerequisites

- **GCC** (with C11 support): `gcc --version` should show 5.0 or newer
- **Make**: `make --version`
- **Linux** (or any POSIX-compatible OS)
- *(Optional)* **Valgrind** for memory leak detection: `valgrind --version`

Install on Debian/Ubuntu:
```bash
sudo apt-get update
sudo apt-get install build-essential valgrind
```

Install on Fedora/RHEL:
```bash
sudo dnf install gcc make valgrind
```

---

## Building the Project

Clone the repository and build:

```bash
git clone https://github.com/purianmol/kiwiFS.git
cd kiwiFS
make
```

This compiles all source files and produces the `kiwifs` executable.

To clean build artifacts:
```bash
make clean
```

---

## Formatting a Filesystem

Before using kiwiFS for the first time (or to start fresh), format a new disk image:

```bash
./kiwifs --format disk.img
```

This will:
1. Create a new 4 MB `disk.img` file
2. Write the superblock with the magic number `0x4B575331`
3. Initialize the block bitmap (all blocks free)
4. Initialize the inode table
5. Allocate inode 0 as the root directory `/`
6. Create the root directory data block

> **Note:** Formatting destroys all existing data in `disk.img`.

---

## Running kiwiFS

After formatting, mount and start the interactive shell:

```bash
./kiwifs disk.img
```

You will see:
```
kiwiFS v1.0 — virtual filesystem
Mounted disk.img (4 MB, 1024 blocks)
kiwifs>
```

---

## Supported Commands

| Command | Syntax | Description |
|---------|--------|-------------|
| `format` | `format` | Re-format the current disk (warning: destroys all data) |
| `ls` | `ls` | List files and directories in current directory |
| `mkdir` | `mkdir <name>` | Create a new directory |
| `cd` | `cd <path>` | Change current directory |
| `pwd` | `pwd` | Print current working directory path |
| `touch` | `touch <name>` | Create an empty file |
| `write` | `write <file> "<text>"` | Write text to a file (replaces existing content) |
| `cat` | `cat <file>` | Display file contents |
| `rm` | `rm <file>` | Delete a file |
| `help` | `help` | Show all available commands |
| `exit` | `exit` | Exit kiwiFS (data is already persisted) |

### Path Support

| Path | Meaning |
|------|---------|
| `/` | Root directory |
| `.` | Current directory |
| `..` | Parent directory |
| `projects` | Relative path |
| `/projects/main.c` | Absolute path |

---

## Example Session

The following session demonstrates creating files, navigating directories, and verifying persistence across two runs:

```
$ ./kiwifs --format disk.img
Filesystem formatted successfully.

$ ./kiwifs disk.img
kiwiFS v1.0 — virtual filesystem
Mounted disk.img (4 MB, 1024 blocks)

kiwifs> mkdir projects
Directory created: projects

kiwifs> cd projects
kiwifs> touch hello.txt
File created: hello.txt

kiwifs> write hello.txt "Hello, kiwiFS!"
Written 14 bytes to hello.txt

kiwifs> cat hello.txt
Hello, kiwiFS!

kiwifs> ls
hello.txt

kiwifs> cd ..
kiwifs> pwd
/

kiwifs> ls
projects/

kiwifs> exit

$ ./kiwifs disk.img
kiwiFS v1.0 — virtual filesystem
Mounted disk.img (4 MB, 1024 blocks)

kiwifs> cd projects
kiwifs> cat hello.txt
Hello, kiwiFS!

kiwifs> rm hello.txt
Removed: hello.txt

kiwifs> ls
(empty directory)

kiwifs> exit
```

The second session proves **persistence**: data written in one session survives a complete program restart, because all state is stored in `disk.img`.

---

## Design Decisions

### 1. Single binary disk image
All filesystem data lives in one file (`disk.img`). This cleanly separates kiwiFS from the host filesystem and makes persistence straightforward — every write goes through the disk layer to the image.

### 2. Fixed-size blocks
Using 4096-byte blocks (matching the common Linux page size) simplifies block arithmetic and avoids fragmentation complexity in v1. Every read/write operates on one full block.

### 3. Disk layer abstraction
Only `disk.c` is allowed to call `fread`, `fwrite`, `fseek`, or `fclose`. All other modules use `disk_read_block()` and `disk_write_block()`. This encapsulates I/O concerns and makes the upper layers easier to unit-test independently.

### 4. Direct block pointers only
Each inode holds 8 direct block pointers, capping maximum file size at 32 KB. This is intentional for v1 — it avoids the complexity of indirect block chains while still fully demonstrating the core concept of block-based file storage.

### 5. Directory entries as fixed-size arrays
Directories store `DirectoryEntry` structs (32 bytes each: 4-byte inode ID + 28-byte name) packed into data blocks. This is simple to implement, has a predictable layout, and is easy to iterate over.

### 6. Magic number validation
Every mount validates the superblock magic number (`0x4B575331`). This catches corrupted images and prevents silently treating arbitrary binary files as kiwiFS disks.

### 7. Write replaces entire file
In v1, `write <file> "<text>"` replaces the entire file contents. This avoids the complexity of partial writes, seek positions, and append semantics while still demonstrating block allocation and inode metadata updates.

### 8. Inode 0 = root directory
Root is always inode 0. This is a well-established convention (ext2 uses inode 2 for root; we use 0 for simplicity). The shell starts with `current_dir_inode = 0`.

### 9. Fixed-width integer types
The code uses `uint32_t` throughout (via `<stdint.h>`) for all on-disk structures. This ensures consistent struct sizes regardless of host architecture, making the disk image portable across 32-bit and 64-bit systems.

---

## Limitations (v1)

These are intentional constraints for v1, not bugs. They keep the codebase focused and understandable.

| Limitation | Reason |
|------------|--------|
| Max file size: 32 KB | Only 8 direct block pointers per inode |
| Max filename: 27 chars | Fixed `DirectoryEntry` struct size |
| No file permissions | Out of scope for v1 |
| No symbolic links | Out of scope for v1 |
| No hard links | One directory entry per file |
| No journaling | Crash consistency is a v2 concern |
| No concurrent access | Single process only |
| No in-memory caching | Every read/write goes directly to `disk.img` |
| `write` replaces file | No append or partial write |
| `rm` files only | Cannot remove non-empty directories |
| No `cp` or `mv` | No copy or rename commands |
| No FUSE | Cannot mount as a real host filesystem |

---

## Testing

Build and run all tests:

```bash
make test
```

### Test Coverage

| Test file | What it tests |
|-----------|---------------|
| `tests/test_disk.c` | Disk creation, block write, block read, data consistency |
| `tests/test_bitmap.c` | Allocation, free, set, clear, exhaustion |
| `tests/test_inode.c` | Allocate, read, write, free, type fields |
| `tests/test_directory.c` | Add entry, lookup, remove, duplicate name rejection |
| `tests/test_file.c` | Create, write, read, delete; persistence after program reopen |

### Memory Check (optional)

If Valgrind is installed:

```bash
valgrind --leak-check=full ./kiwifs disk.img
```

No memory leaks should be reported during normal operation.

---

## Future Improvements (v2+)

These are the natural next steps after v1 is solid:

- **Indirect blocks** — Support files larger than 32 KB
- **Append mode** — `write --append <file> "<text>"`
- **File permissions** — Basic Unix `rwxrwxrwx` permission bits
- **Hard links** — Multiple directory entries pointing to the same inode
- **Symbolic links** — Soft links between paths
- **Directory deletion** — `rmdir` for empty directories
- **File rename/move** — `mv` command
- **File copy** — `cp` command
- **Journaling** — Write-ahead log for crash consistency
- **FUSE integration** — Mount kiwiFS as a real Linux filesystem
- **Fragmentation analysis** — Report disk usage and fragmentation
- **Timestamps** — `atime`, `mtime`, `ctime` fields in inodes

---

## License

This project is licensed under the **MIT License** — see the [LICENSE](LICENSE) file for details.