/*
 * This file is part of the bladeRF project
 *
 * Copyright (C) 2013 Nuand LLC
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
#include <stdlib.h>
#include <stdio.h>
#include <editline/readline.h>
#include <string.h>
#include <errno.h>
#include "input_impl.h"

// The caller of input_get_line does not free the returned buffer.
// The readline function expects the caller to free the returned
// buffer. Save the last returned buffer and free it on input_deinit
// or input_get_line if it is set.
static char* last_line = NULL;

int input_init()
{
    if (rl_initialize() || rl_set_prompt(CLI_DEFAULT_PROMPT)) {
        return CLI_RET_UNKNOWN;
    } else {
        return 0;
    }
}

void input_deinit()
{
    if (last_line) {
        free(last_line);
        last_line = NULL;
    }
}

char * input_get_line(const char *prompt)
{
    if (last_line) {
        free(last_line);
    }

    last_line = readline(prompt);

    if (last_line) {
        add_history(last_line);
    }

    return last_line;
}

int input_set_input(FILE *file)
{
    rl_instream = file;
    return 0;
}

char * input_expand_path(const char *path)
{
    return strdup(path);
}

void input_clear_terminal()
{
    rl_reset_terminal(NULL);
}

/* Nothing to do here, libedit handles the signal if we're in a call */
void input_ctrlc(void)
{
}
