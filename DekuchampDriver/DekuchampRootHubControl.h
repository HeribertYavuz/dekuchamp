/*
 * Copyright (c) 2013-2019 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef DEKUCHAMP_ROOTHUB_CONTROL_H
#define DEKUCHAMP_ROOTHUB_CONTROL_H

#include "DekuchampMain.h"

NTSTATUS DekuchampCreateRootHubControlDevice(IN PDEVICE_EXTENSION hubExt,
                                           OUT PDEVICE_OBJECT *control,
                                           OUT USHORT *busId);

VOID DekuchampDeleteRootHubControlDevice(IN PDEVICE_OBJECT controlDevice);

#endif /* DEKUCHAMP_ROOTHUB_CONTROL_H */
