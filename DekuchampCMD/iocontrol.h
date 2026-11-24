/*
 * Copyright (c) 2013 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef DEKUCHAMP_CMD_IOCONTROL_H
#define DEKUCHAMP_CMD_IOCONTROL_H

#include <basetsd.h>
#include <wtypes.h>
#include "Dekuchamp.h"

BOOLEAN DekuchampIsDeviceFiltered(PDEKUCHAMP_ADDRESS_FILTER filter, int address);
BOOLEAN DekuchampSetDeviceFiltered(PDEKUCHAMP_ADDRESS_FILTER filter, int address);
BOOLEAN DekuchampInitAddressFilter(PDEKUCHAMP_ADDRESS_FILTER filter, PCHAR list, BOOLEAN filterAll);

#endif /* DEKUCHAMP_CMD_IOCONTROL_H */
