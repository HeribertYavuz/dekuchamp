/*
 * Copyright (c) 2013 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef DEKUCHAMP_CMD_FILTERS_H
#define DEKUCHAMP_CMD_FILTERS_H

struct filters
{
    char *device; /* \\.\DekuchampX */
};

/* NULL-terminated array of pointers to struct filters */
extern struct filters **dekuchampFilters;

void filters_initialize();
void filters_free();

BOOL is_dekuchamp_upper_filter_installed();

#endif /* DEKUCHAMP_CMD_FILTERS_H */
