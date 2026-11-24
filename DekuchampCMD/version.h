/*
 * Copyright (c) 2018 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */


#ifndef DEKUCHAMPCMD_VERSION_H
#define DEKUCHAMPCMD_VERSION_H

#define DEKUCHAMPCMD_VERSION_MAJOR    1
#define DEKUCHAMPCMD_VERSION_MINOR    5
#define DEKUCHAMPCMD_VERSION_REVISION 4
#define DEKUCHAMPCMD_VERSION_BUILD    0

#define makeStr(x) #x
#define makeString(x) makeStr(x)

#define DEKUCHAMPCMD_VERSION_STR \
    makeString(DEKUCHAMPCMD_VERSION_MAJOR) "." \
    makeString(DEKUCHAMPCMD_VERSION_MINOR) "." \
    makeString(DEKUCHAMPCMD_VERSION_REVISION) "." \
    makeString(DEKUCHAMPCMD_VERSION_BUILD)

#endif /* DEKUCHAMPCMD_VERSION_H */
