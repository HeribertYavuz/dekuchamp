/*
 * Copyright (c) 2013-2019 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef DEKUCHAMP_URB_H
#define DEKUCHAMP_URB_H

#include "DekuchampMain.h"

VOID DekuchampAnalyzeURB(PIRP pIrp, PURB pUrb, BOOLEAN post,
                       PDEKUCHAMP_DEVICE_DATA pDeviceData);

#endif /* DEKUCHAMP_URB_H */
