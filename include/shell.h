/*
 * shell.h — Interactive CLI Shell Interface
 *
 * Provides the interactive REPL prompt ("kiwifs> ") for issuing
 * filesystem commands (ls, mkdir, cd, pwd, touch, write, cat, rm, help, exit).
 */

#ifndef KIWI_SHELL_H
#define KIWI_SHELL_H

#include "fs.h"

/*
 * shell_run - start the interactive REPL loop on standard input.
 *
 * Runs until the user types 'exit', 'quit', or EOF (Ctrl+D).
 */
void shell_run(Filesystem *fs);

#endif /* KIWI_SHELL_H */
