/*
 * Copyright (c) 2013-2019 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include "DekuchampMain.h"
#include "DekuchampBuffer.h"
#include "DekuchampHelperFunctions.h"

#define DEKUCHAMP_BUFFER_TAG  (ULONG)'ffuB'

__inline static UINT32
DekuchampGetBufferFree(PDEKUCHAMP_ROOTHUB_DATA pData)
{
    if (pData->buffer == NULL)
    {
        /* There is no buffer, nothing can be written */
        return 0;
    }
    else if (pData->readOffset == pData->writeOffset)
    {
        /* readOffset is equal to writeOffset when buffer is empty
         *
         * At max, we can write bufferSize - 1 bytes of data
         */
        return pData->bufferSize - 1;
    }
    else if (pData->readOffset > pData->writeOffset)
    {
        /* readOffset is bigger than writeOffset when:
         * XXXXXXXW.............RXXXXXXX
         *
         * where:
         *   X is data to be read
         *   . is free data
         *   R is readOffset (first byte to be read)
         *   W is writeOffset (first empty byte)
         */

        return pData->readOffset - pData->writeOffset - 1;
    }
    else
    {
        /* readOffset is lower than writeOffset when:
         * ........RXXXXXXXXXXW.........
         */

        return pData->bufferSize - pData->writeOffset +
               pData->readOffset - 1;
    }
}

__inline static UINT32
DekuchampGetBufferAllocated(PDEKUCHAMP_ROOTHUB_DATA pData)
{
    if (pData->readOffset == pData->writeOffset)
    {
        /* readOffset is equal to writeOffset when buffer is empty
         */
        return 0;
    }
    else if (pData->readOffset > pData->writeOffset)
    {
        /* readOffset is bigger than writeOffset when:
         * XXXXXXXW.............RXXXXXXX
         */

        return pData->bufferSize - pData->readOffset +
               pData->writeOffset;
    }
    else
    {
        /* readOffset is lower than writeOffset when:
         * ........RXXXXXXXXXXW.........
         */

        return pData->writeOffset - pData->readOffset;
    }
}

__inline static void
DekuchampBufferWriteUnsafe(PDEKUCHAMP_ROOTHUB_DATA pData,
                         PVOID data,
                         UINT32 length)
{
    PCHAR buffer = (PCHAR)pData->buffer;

    if (pData->bufferSize - pData->writeOffset >= length)
    {
        /* We can write all data without looping */
        RtlCopyMemory((PVOID)&buffer[pData->writeOffset],
                      data,
                      (SIZE_T)length);
        pData->writeOffset += length;
        pData->writeOffset %= pData->bufferSize;
    }
    else
    {
        /* We need to loop */
        PCHAR origData = (PCHAR)data;
        UINT32 tmp;

        /* First copy */
        tmp = pData->bufferSize - pData->writeOffset;
        RtlCopyMemory((PVOID)&buffer[pData->writeOffset],
                      data,
                      (SIZE_T)tmp);

        /* Second copy */
        RtlCopyMemory(pData->buffer, /* Write at beginning of buffer */
                      (PVOID)&origData[tmp],
                      length - tmp);

        pData->writeOffset = length - tmp;
    }
}

/*
 * Writes data to buffer.
 *
 * Caller must have acquired buffer spin lock.
 */
static NTSTATUS DekuchampBufferWrite(PDEKUCHAMP_ROOTHUB_DATA pData,
                                   PVOID data,
                                   UINT32 length)
{
    if (length == 0)
    {
        DkDbgStr("Cannot write empty data.");
        return STATUS_INVALID_PARAMETER;
    }

    if (DekuchampGetBufferFree(pData) < length)
    {
        DkDbgStr("No free space left.");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    DekuchampBufferWriteUnsafe(pData, data, length);
    return STATUS_SUCCESS;
}

/*
 * Reads data from circular buffer.
 *
 * Retruns number of bytes read.
 */
static UINT32 DekuchampBufferRead(PDEKUCHAMP_ROOTHUB_DATA pData,
                                PVOID destBuffer,
                                UINT32 destBufferSize)
{
    UINT32 available;
    UINT32 toRead;

    PCHAR srcBuffer = (PCHAR)pData->buffer;

    available = DekuchampGetBufferAllocated(pData);

    /* No data to be read or empty destination buffer */
    if (available == 0 || destBufferSize == 0)
    {
        return 0;
    }

    /* Calculate how many bytes will fit into buffer */
    if (available > destBufferSize)
    {
        toRead = destBufferSize;
    }
    else
    {
        toRead = available;
    }

    if (pData->writeOffset > pData->readOffset)
    {
        /* Simply copy the contiguous data */
        RtlCopyMemory(destBuffer,
                      (PVOID)&srcBuffer[pData->readOffset],
                      (SIZE_T)toRead);

        pData->readOffset += toRead;
        pData->readOffset %= pData->bufferSize;
    }
    else
    {
        UINT32 tmp;
        UINT32 tmp2;
        tmp = pData->bufferSize - pData->readOffset;

        if (tmp >= toRead)
        {
            /* Copy contiguous data */
            RtlCopyMemory(destBuffer,
                          (PVOID)&srcBuffer[pData->readOffset],
                          (SIZE_T)toRead);

            pData->readOffset += toRead;
            pData->readOffset %= pData->bufferSize;
        }
        else
        {
            PCHAR dstBuffer = (PCHAR)destBuffer;
            /* Copy non-contiguous data */

            /* First copy */
            RtlCopyMemory(destBuffer,
                          (PVOID)&srcBuffer[pData->readOffset],
                          (SIZE_T)tmp);

            /* Second copy */
            RtlCopyMemory((PVOID)&dstBuffer[tmp],
                          (PVOID)srcBuffer,
                          (SIZE_T)toRead - tmp);

            pData->readOffset = toRead - tmp;
        }
    }

    return toRead;
}


/*
 * Writes global PCAP header to buffer.
 * Caller must have acquired buffer spin lock.
 */
__inline static VOID
DekuchampWriteGlobalHeader(PDEKUCHAMP_ROOTHUB_DATA pData)
{
    pcap_hdr_t header;

    header.magic_number = 0xA1B2C3D4;
    header.version_major = 2;
    header.version_minor = 4;
    header.thiszone = 0 /* Assume UTC */;
    header.sigfigs = 0;
    header.snaplen = pData->snaplen;
    header.network = DLT_DEKUCHAMP;

    ASSERT (DekuchampGetBufferFree(pData) >= sizeof(header));

    DekuchampBufferWrite(pData, (PVOID)&header, sizeof(header));
}

NTSTATUS DekuchampSetUpBuffer(PDEKUCHAMP_ROOTHUB_DATA pData,
                            UINT32 bytes)
{
    NTSTATUS  status;
    KIRQL     irql;
    PVOID     buffer;

    /* Minimum buffer size is 4 KiB, maximum 128 MiB */
    if (bytes < 4096 || bytes > 134217728)
    {
        return STATUS_INVALID_PARAMETER;
    }

    buffer = ExAllocatePoolWithTag(NonPagedPool,
                                   (SIZE_T) bytes,
                                   DEKUCHAMP_BUFFER_TAG);

    if (buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = STATUS_SUCCESS;
    KeAcquireSpinLock(&pData->bufferLock, &irql);
    if (pData->buffer == NULL)
    {
        pData->buffer = buffer;
        pData->bufferSize = bytes;
        pData->readOffset = 0;
        pData->writeOffset = 0;
        DekuchampWriteGlobalHeader(pData);
        DkDbgVal("Created new buffer", bytes);
    }
    else
    {
        UINT32 allocated = DekuchampGetBufferAllocated(pData);

        if (allocated >= bytes)
        {
            status = STATUS_BUFFER_TOO_SMALL;
            ExFreePool(buffer);
        }
        else
        {
            /* Copy (if any) unread data to new buffer */
            if (allocated > 0)
            {
                DekuchampBufferRead(pData, buffer, bytes);
            }

            /* Free the old buffer */
            ExFreePool(pData->buffer);
            pData->buffer = buffer;
            pData->bufferSize = bytes;
            pData->readOffset = 0;
            pData->writeOffset = allocated;
        }
    }

    KeReleaseSpinLock(&pData->bufferLock, irql);
    return status;
}

NTSTATUS DekuchampSetSnaplenSize(PDEKUCHAMP_ROOTHUB_DATA pData,
                               UINT32 bytes)
{
    NTSTATUS  status;
    KIRQL     irql;

    if (bytes == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    status = STATUS_SUCCESS;
    KeAcquireSpinLock(&pData->bufferLock, &irql);
    if (pData->buffer != NULL)
    {
        status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        pData->snaplen = bytes;
    }

    KeReleaseSpinLock(&pData->bufferLock, irql);
    return status;
}

/*
 * If there is buffer allocated for given control device, frees all
 * memory allocated to it, otherwise does nothing.
 */
VOID DekuchampBufferRemoveBuffer(PDEVICE_EXTENSION pDevExt)
{
    PDEVICE_EXTENSION      pRootExt;
    PDEKUCHAMP_ROOTHUB_DATA  pData;
    KIRQL                  irql;

    ASSERT(pDevExt->deviceMagic == DEKUCHAMP_MAGIC_CONTROL);

    pRootExt = (PDEVICE_EXTENSION)pDevExt->context.control.pRootHubObject->DeviceExtension;
    pData = pRootExt->context.usb.pDeviceData->pRootData;

    if (pData->buffer == NULL)
    {
        return;
    }

    /* Buffer found - free it */
    KeAcquireSpinLock(&pData->bufferLock, &irql);
    pData->readOffset = 0;
    pData->writeOffset = 0;
    ExFreePool((PVOID)pData->buffer);
    pData->buffer = NULL;
    KeReleaseSpinLock(&pData->bufferLock, irql);
}

/*
 * If there is buffer allocated for given control device, writes global
 * PCAP header to the buffer, otherwise does nothing.
 */
VOID DekuchampBufferInitializeBuffer(PDEVICE_EXTENSION pDevExt)
{
    PDEVICE_EXTENSION      pRootExt;
    PDEKUCHAMP_ROOTHUB_DATA  pData;
    KIRQL                  irql;

    ASSERT(pDevExt->deviceMagic == DEKUCHAMP_MAGIC_CONTROL);

    pRootExt = (PDEVICE_EXTENSION)pDevExt->context.control.pRootHubObject->DeviceExtension;
    pData = pRootExt->context.usb.pDeviceData->pRootData;

    if (pData->buffer == NULL)
    {
        return;
    }

    /* Buffer found - reset all data and write global PCAP header */
    KeAcquireSpinLock(&pData->bufferLock, &irql);
    pData->readOffset = 0;
    pData->writeOffset = 0;
    DekuchampWriteGlobalHeader(pData);
    KeReleaseSpinLock(&pData->bufferLock, irql);
}

NTSTATUS DekuchampBufferHandleReadIrp(PIRP pIrp,
                                    PDEVICE_EXTENSION pDevExt,
                                    PUINT32 pBytesRead)
{
    PDEVICE_EXTENSION      pRootExt;
    PDEKUCHAMP_ROOTHUB_DATA  pRootData;
    PVOID                  buffer;
    UINT32                 bufferLength;
    UINT32                 bytesRead;
    NTSTATUS               status;
    KIRQL                  irql;
    PIO_STACK_LOCATION     pStack = NULL;

    pStack = IoGetCurrentIrpStackLocation(pIrp);

    *pBytesRead = 0;

    if (pStack->Parameters.Read.Length == 0)
    {
        return STATUS_SUCCESS;
    }

    pRootExt = (PDEVICE_EXTENSION)pDevExt->context.control.pRootHubObject->DeviceExtension;
    pRootData = pRootExt->context.usb.pDeviceData->pRootData;

    if (pRootData->buffer == NULL)
    {
        return STATUS_UNSUCCESSFUL;
    }

    /*
     * Since control device has DO_DIRECT_IO bit set the MDL is already
     * probed and locked
     */
    buffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress,
                                          NormalPagePriority);

    if (buffer == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        bufferLength = MmGetMdlByteCount(pIrp->MdlAddress);
    }

    /* Get data from data queue, if there is no data we put
     * this IRP to Cancel-Safe queue and return status pending
     * otherwise complete this IRP then return SUCCESS
     */
    KeAcquireSpinLock(&pRootData->bufferLock, &irql);
    bytesRead = DekuchampBufferRead(pRootData,
                                  buffer, bufferLength);
    *pBytesRead = bytesRead;
    if (bytesRead == 0)
    {
        IoCsqInsertIrp(&pDevExt->context.control.ioCsq,
                       pIrp, NULL);
        KeReleaseSpinLock(&pRootData->bufferLock, irql);
        return STATUS_PENDING;
    }

    KeReleaseSpinLock(&pRootData->bufferLock, irql);
    return STATUS_SUCCESS;
}

/* called with pRootData->bufferLock held
 * releases pRootData->bufferLock before return
 */
static void DekuchampBufferCompletePendedReadIrp(PDEKUCHAMP_ROOTHUB_DATA pRootData, KIRQL irql)
{
    PDEVICE_EXTENSION  pControlExt;
    PIRP               pIrp = NULL;
    PVOID              buffer;
    UINT32             bufferLength;
    UINT32             bytes;

    pControlExt = (PDEVICE_EXTENSION)pRootData->controlDevice->DeviceExtension;

    ASSERT(pControlExt->deviceMagic == DEKUCHAMP_MAGIC_CONTROL);

    pIrp = IoCsqRemoveNextIrp(&pControlExt->context.control.ioCsq,
                                  NULL);
    if (pIrp == NULL)
    {
        KeReleaseSpinLock(&pRootData->bufferLock, irql);
        return;
    }

    /*
     * Only IRPs with non-zero buffer are being queued.
     *
     * Since control device has DO_DIRECT_IO bit set the MDL is already
     * probed and locked
     */
    buffer = MmGetSystemAddressForMdlSafe(pIrp->MdlAddress,
                                          NormalPagePriority);

    if (buffer == NULL)
    {
        pIrp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
        bytes = 0;
    }
    else
    {
        UINT32 bufferLength = MmGetMdlByteCount(pIrp->MdlAddress);

        if (bufferLength != 0)
        {
            bytes = DekuchampBufferRead(pRootData,
                                      buffer, bufferLength);
        }
        else
        {
            bytes = 0;
        }

        pIrp->IoStatus.Status = STATUS_SUCCESS;
    }

    pIrp->IoStatus.Information = (ULONG_PTR) bytes;
    /* release lock before completing the IRP! */
    KeReleaseSpinLock(&pRootData->bufferLock, irql);
    IoCompleteRequest(pIrp, IO_NO_INCREMENT);
}

__inline static VOID
DekuchampInitializePcapHeader(PDEKUCHAMP_ROOTHUB_DATA pData,
                            LARGE_INTEGER timestamp,
                            pcaprec_hdr_t *pcapHeader,
                            UINT32 bytes)
{
    pcapHeader->ts_sec = (UINT32)(timestamp.QuadPart/10000000-11644473600);
    pcapHeader->ts_usec = (UINT32)((timestamp.QuadPart%10000000)/10);

    /* Obey the snaplen limit */
    if (bytes > pData->snaplen)
    {
        pcapHeader->incl_len = pData->snaplen;
    }
    else
    {
        pcapHeader->incl_len = bytes;
    }
    pcapHeader->orig_len = bytes;
}

/* Caller must hold bufferLock
 *
 * payloadEntries is array of DEKUCHAMP_PAYLOAD_ENTRY with the last element being {0, NULL}
 */
static NTSTATUS
DekuchampBufferStorePacket(PDEKUCHAMP_ROOTHUB_DATA pRootData,
                         LARGE_INTEGER timestamp,
                         PDEKUCHAMP_BUFFER_PACKET_HEADER header,
                         PDEKUCHAMP_PAYLOAD_ENTRY payloadEntries)
{
    UINT32             bytes;
    UINT32             bytesFree;
    UINT32             tmp;
    pcaprec_hdr_t      pcapHeader;
    int                i;

    bytes = header->headerLen + header->dataLength;

    DekuchampInitializePcapHeader(pRootData, timestamp, &pcapHeader, bytes);

    /* pcapHeader.incl_len contains the number of bytes to write */
    bytes = pcapHeader.incl_len;

    /* Sanity check payload entries */
    if (bytes > (sizeof(pcaprec_hdr_t) + header->headerLen))
    {
        UINT32 bytesMissing = bytes - (sizeof(pcaprec_hdr_t) + header->headerLen);

        for (i = 0; (bytesMissing > 0) && (payloadEntries[i].buffer); i++)
        {
            bytesMissing -= min(payloadEntries[i].size, bytesMissing);
        }
        if (bytesMissing > 0)
        {
            DkDbgVal("Attempted to write invalid packet. Missing %d bytes of payload.",
                     bytesMissing);
            return STATUS_INVALID_PARAMETER;
        }
    }

    bytesFree = DekuchampGetBufferFree(pRootData);

    if ((pRootData->buffer == NULL) ||
        (bytesFree < sizeof(pcaprec_hdr_t)) ||
        ((bytesFree - sizeof(pcaprec_hdr_t)) < bytes))
    {
        DkDbgStr("No enough free space left.");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Write Packet Header */
    DekuchampBufferWriteUnsafe(pRootData,
                             (PVOID) &pcapHeader,
                             (UINT32) sizeof(pcaprec_hdr_t));

    /* Write DEKUCHAMP_BUFFER_PACKET_HEADER */
    tmp = min(bytes, (UINT32)header->headerLen);
    if (tmp > 0)
    {
        DekuchampBufferWriteUnsafe(pRootData,
                                 (PVOID) header,
                                 tmp);
    }
    bytes -= tmp;

    /* Write payload entries */
    for (i = 0; (bytes > 0) && (payloadEntries[i].buffer); i++)
    {
        tmp = min(bytes, payloadEntries[i].size);
        if (tmp > 0)
        {
            DekuchampBufferWriteUnsafe(pRootData,
                                     payloadEntries[i].buffer,
                                     tmp);
        }
        bytes -= tmp;
    }

    return STATUS_SUCCESS;
}

NTSTATUS DekuchampBufferWriteTimestampedPayload(PDEKUCHAMP_ROOTHUB_DATA pRootData,
                                              LARGE_INTEGER timestamp,
                                              PDEKUCHAMP_BUFFER_PACKET_HEADER header,
                                              PDEKUCHAMP_PAYLOAD_ENTRY payload)
{
    KIRQL                  irql;
    NTSTATUS               status;

    KeAcquireSpinLock(&pRootData->bufferLock, &irql);
    status = DekuchampBufferStorePacket(pRootData, timestamp, header, payload);
    if (NT_SUCCESS(status))
    {
        DekuchampBufferCompletePendedReadIrp(pRootData, irql);
    }
    else
    {
        KeReleaseSpinLock(&pRootData->bufferLock, irql);
    }

    return status;
}

NTSTATUS DekuchampBufferWritePayload(PDEKUCHAMP_ROOTHUB_DATA pRootData,
                                   PDEKUCHAMP_BUFFER_PACKET_HEADER header,
                                   PDEKUCHAMP_PAYLOAD_ENTRY payload)
{
    LARGE_INTEGER timestamp = DekuchampGetCurrentTimestamp();
    return DekuchampBufferWriteTimestampedPayload(pRootData, timestamp, header, payload);
}

NTSTATUS DekuchampBufferWriteTimestampedPacket(PDEKUCHAMP_ROOTHUB_DATA pRootData,
                                             LARGE_INTEGER timestamp,
                                             PDEKUCHAMP_BUFFER_PACKET_HEADER header,
                                             PVOID buffer)
{
    DEKUCHAMP_PAYLOAD_ENTRY  payload[2];

    payload[0].size   = header->dataLength;
    payload[0].buffer = buffer;
    payload[1].size   = 0;
    payload[1].buffer = NULL;

    return DekuchampBufferWriteTimestampedPayload(pRootData, timestamp, header, payload);
}

NTSTATUS DekuchampBufferWritePacket(PDEKUCHAMP_ROOTHUB_DATA pRootData,
                                  PDEKUCHAMP_BUFFER_PACKET_HEADER header,
                                  PVOID buffer)
{
    LARGE_INTEGER timestamp = DekuchampGetCurrentTimestamp();
    return DekuchampBufferWriteTimestampedPacket(pRootData, timestamp, header, buffer);
}
