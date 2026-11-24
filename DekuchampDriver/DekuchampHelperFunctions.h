/*
 * Copyright (c) 2013-2019 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef DEKUCHAMP_HELPER_FUNCTIONS_H
#define DEKUCHAMP_HELPER_FUNCTIONS_H

#include "DekuchampMain.h"

NTSTATUS DekuchampGetTargetDevicePdo(IN PDEVICE_OBJECT DeviceObject,
                                   OUT PDEVICE_OBJECT *pdo);

NTSTATUS DekuchampGetNumberOfPorts(PDEVICE_OBJECT parent,
                                 PULONG numberOfPorts);

#if DBG
NTSTATUS DekuchampPrintUSBPChildrenInformation(PDEVICE_OBJECT hub);
#else
#define DekuchampPrintUSBPChildrenInformation(hub) {}
#endif

NTSTATUS DekuchampGetDeviceUSBInfo(PDEVICE_EXTENSION pDevExt);

BOOLEAN DekuchampIsDeviceRootHub(PDEVICE_OBJECT device);

PWSTR DekuchampGetHubInterfaces(PDEVICE_OBJECT hub);


BOOLEAN DekuchampIsDeviceFiltered(PDEKUCHAMP_ADDRESS_FILTER filter, int address);
BOOLEAN DekuchampSetDeviceFiltered(PDEKUCHAMP_ADDRESS_FILTER filter, int address);

LARGE_INTEGER DekuchampGetCurrentTimestamp(VOID);

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, DekuchampGetTargetDevicePdo)
#pragma alloc_text (PAGE, DekuchampGetNumberOfPorts)
#if DBG
#pragma alloc_text (PAGE, DekuchampPrintUSBPChildrenInformation)
#endif
#pragma alloc_text (PAGE, DekuchampGetDeviceUSBInfo)
#pragma alloc_text (PAGE, DekuchampIsDeviceRootHub)
#pragma alloc_text (PAGE, DekuchampGetHubInterfaces)
#endif

#endif /* DEKUCHAMP_HELPER_FUNCTIONS_H */
