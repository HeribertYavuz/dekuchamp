/*
 * Copyright (c) 2013-2019 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef DEKUCHAMP_BUFFER_H
#define DEKUCHAMP_BUFFER_H

#include "DekuchampMain.h"

typedef struct
{
    UINT32  size;
    PVOID   buffer;
} DEKUCHAMP_PAYLOAD_ENTRY, *PDEKUCHAMP_PAYLOAD_ENTRY;

NTSTATUS DekuchampSetUpBuffer(PDEKUCHAMP_ROOTHUB_DATA pData,
                            UINT32 bytes);
NTSTATUS DekuchampSetSnaplenSize(PDEKUCHAMP_ROOTHUB_DATA pData,
                               UINT32 bytes);

VOID DekuchampBufferRemoveBuffer(PDEVICE_EXTENSION pDevExt);
VOID DekuchampBufferInitializeBuffer(PDEVICE_EXTENSION pDevExt);
NTSTATUS DekuchampBufferHandleReadIrp(PIRP pIrp,
                                    PDEVICE_EXTENSION pDevExt,
                                    PUINT32 pBytesRead);

/* Same as DekuchampBufferWriteTimestampedPacket but take {0, NULL} terminated
 * array of payload entries instead of single buffer pointer.
 */
NTSTATUS DekuchampBufferWriteTimestampedPayload(PDEKUCHAMP_ROOTHUB_DATA pRootData,
                                              LARGE_INTEGER timestamp,
                                              PDEKUCHAMP_BUFFER_PACKET_HEADER header,
                                              PDEKUCHAMP_PAYLOAD_ENTRY payload);
/* Same as DekuchampBufferWritePacket but take {0, NULL} terminated
 * array of payload entries instead of single buffer pointer.
 */
NTSTATUS DekuchampBufferWritePayload(PDEKUCHAMP_ROOTHUB_DATA pRootData,
                                   PDEKUCHAMP_BUFFER_PACKET_HEADER header,
                                   PDEKUCHAMP_PAYLOAD_ENTRY payload);

NTSTATUS DekuchampBufferWriteTimestampedPacket(PDEKUCHAMP_ROOTHUB_DATA pRootData,
                                             LARGE_INTEGER timestamp,
                                             PDEKUCHAMP_BUFFER_PACKET_HEADER header,
                                             PVOID buffer);
NTSTATUS DekuchampBufferWritePacket(PDEKUCHAMP_ROOTHUB_DATA pRootData,
                                  PDEKUCHAMP_BUFFER_PACKET_HEADER header,
                                  PVOID buffer);

#endif /* DEKUCHAMP_BUFFER_H */
