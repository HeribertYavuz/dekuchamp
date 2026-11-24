/*
 * Copyright (c) 2013-2019 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#include "DekuchampMain.h"
#include "DekuchampTables.h"

#define DEKUCHAMP_TABLE_TAG ' BAT'

typedef struct _DEKUCHAMP_INTERNAL_ENDPOINT_INFO
{
    RTL_SPLAY_LINKS        links;
    LIST_ENTRY             entry;
    DEKUCHAMP_ENDPOINT_INFO  info;
} DEKUCHAMP_INTERNAL_ENDPOINT_INFO, *PDEKUCHAMP_INTERNAL_ENDPOINT_INFO;

VOID DekuchampRemoveEndpointInfo(IN PRTL_GENERIC_TABLE table,
                               IN USBD_PIPE_HANDLE handle)
{
    DEKUCHAMP_INTERNAL_ENDPOINT_INFO  info;
    BOOLEAN                         deleted;

    info.info.handle = handle;

    deleted = RtlDeleteElementGenericTable(table, &info);

    if (deleted == TRUE)
    {
        DkDbgVal("Successfully removed", handle);
    }
    else
    {
        DkDbgVal("Failed to remove", handle);
    }
}

VOID DekuchampAddEndpointInfo(IN PRTL_GENERIC_TABLE table,
                            IN PUSBD_PIPE_INFORMATION pipeInfo,
                            IN USHORT deviceAddress)
{
    DEKUCHAMP_INTERNAL_ENDPOINT_INFO  info;
    BOOLEAN                         new;
    PDEKUCHAMP_INTERNAL_ENDPOINT_INFO pInfo;

    info.info.handle          = pipeInfo->PipeHandle;
    info.info.type            = pipeInfo->PipeType;
    info.info.endpointAddress = pipeInfo->EndpointAddress;
    info.info.deviceAddress   = deviceAddress;

    pInfo = RtlInsertElementGenericTable(table,
                                 (PVOID)&info,
                                 sizeof(DEKUCHAMP_INTERNAL_ENDPOINT_INFO),
                                 &new);

    if ((new == FALSE) && (pInfo != NULL))
    {
        DkDbgStr("Element already exists in table, updating entry");
        pInfo->info.type            = pipeInfo->PipeType;
        pInfo->info.endpointAddress = pipeInfo->EndpointAddress;
        pInfo->info.deviceAddress   = deviceAddress;
    }
}

/*
 * Retrieves the DEKUCHAMP_ENDPOINT_INFO information from endpoint table.
 *
 * Returns NULL when no such endpoint information was found
 */
PDEKUCHAMP_ENDPOINT_INFO DekuchampGetEndpointInfo(IN PRTL_GENERIC_TABLE table,
                                              IN USBD_PIPE_HANDLE handle)
{
    PDEKUCHAMP_INTERNAL_ENDPOINT_INFO pInfo;
    DEKUCHAMP_INTERNAL_ENDPOINT_INFO  info;

    info.info.handle = handle;

    pInfo = RtlLookupElementGenericTable(table, &info);

    if (pInfo == NULL)
    {
        return NULL;
    }

    return &pInfo->info;
}

RTL_GENERIC_FREE_ROUTINE DekuchampFreeRoutine;
static VOID
DekuchampFreeRoutine(IN PRTL_GENERIC_TABLE table,
                   IN PVOID buffer)
{
    ExFreePool(buffer);
}

RTL_GENERIC_ALLOCATE_ROUTINE DekuchampAllocateRoutine;
static PVOID
DekuchampAllocateRoutine(IN PRTL_GENERIC_TABLE table,
                       IN CLONG size)
{
    return ExAllocatePoolWithTag(NonPagedPool,
                                 size,
                                 DEKUCHAMP_TABLE_TAG);
}

RTL_GENERIC_COMPARE_ROUTINE DekuchampCompareEndpointInfo;
static RTL_GENERIC_COMPARE_RESULTS
DekuchampCompareEndpointInfo(IN PRTL_GENERIC_TABLE table,
                           IN PVOID first,
                           IN PVOID second)
{
    PDEKUCHAMP_INTERNAL_ENDPOINT_INFO left = first;
    PDEKUCHAMP_INTERNAL_ENDPOINT_INFO right = second;

    if (left->info.handle < right->info.handle)
    {
        return GenericLessThan;
    }
    else if (left->info.handle == right->info.handle)
    {
        return GenericEqual;
    }
    else
    {
        return GenericGreaterThan;
    }
}

VOID DekuchampFreeEndpointTable(IN PRTL_GENERIC_TABLE table)
{
    PVOID element;

    DkDbgStr("Free endpoint data");

    /* Delete all the elements from table */
    while (NULL != (element = RtlGetElementGenericTable(table, 0)))
    {
        RtlDeleteElementGenericTable(table, element);
    }

    /* Delete table structure */
    ExFreePool(table);
}

/*
 * Initializes endpoint table.
 * Returns NULL if there are no sufficient resources availble.
 *
 * Returned table must be freed using DekuchampFreeEndpointTable()
 */
PRTL_GENERIC_TABLE DekuchampInitializeEndpointTable(IN PVOID context)
{
    PRTL_GENERIC_TABLE table;

    DkDbgStr("Initialize endpoint table");

    table = (PRTL_GENERIC_TABLE)
                ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(RTL_GENERIC_TABLE),
                                      DEKUCHAMP_TABLE_TAG);

    if (table == NULL)
    {
        DkDbgStr("Unable to allocate endpoint table");
        return table;
    }

    RtlInitializeGenericTable(table,
                              DekuchampCompareEndpointInfo,
                              DekuchampAllocateRoutine,
                              DekuchampFreeRoutine,
                              context);

    return table;
}

BOOLEAN DekuchampRetrieveEndpointInfo(IN PDEKUCHAMP_DEVICE_DATA pDeviceData,
                                    IN USBD_PIPE_HANDLE handle,
                                    PDEKUCHAMP_ENDPOINT_INFO pInfo)
{
    KIRQL irql;
    PDEKUCHAMP_ENDPOINT_INFO info;
    BOOLEAN found = FALSE;

    KeAcquireSpinLock(&pDeviceData->tablesSpinLock, &irql);
    info = DekuchampGetEndpointInfo(pDeviceData->endpointTable, handle);
    if (info != NULL)
    {
        found = TRUE;
        memcpy(pInfo, info, sizeof(DEKUCHAMP_ENDPOINT_INFO));
    }
    KeReleaseSpinLock(&pDeviceData->tablesSpinLock, irql);

    if (found == TRUE)
    {
        DkDbgVal("Found endpoint info", handle);
        DkDbgVal("", pInfo->type);
        DkDbgVal("", pInfo->endpointAddress);
        DkDbgVal("", pInfo->deviceAddress);
    }
    else
    {
        DkDbgVal("Unable to find endpoint info", handle);
    }

    return found;
}


typedef struct _DEKUCHAMP_INTERNAL_URB_IRP_INFO
{
    RTL_SPLAY_LINKS        links;
    LIST_ENTRY             entry;
    DEKUCHAMP_URB_IRP_INFO   info;
} DEKUCHAMP_INTERNAL_URB_IRP_INFO, *PDEKUCHAMP_INTERNAL_URB_IRP_INFO;

VOID DekuchampRemoveURBIRPInfo(IN PRTL_GENERIC_TABLE table,
                             IN PIRP irp)
{
    DEKUCHAMP_INTERNAL_URB_IRP_INFO   info;
    BOOLEAN                         deleted;

    info.info.irp = irp;

    deleted = RtlDeleteElementGenericTable(table, &info);

    if (deleted == TRUE)
    {
        DkDbgVal("Successfully removed irp from table", irp);
    }
}

VOID DekuchampAddURBIRPInfo(IN PRTL_GENERIC_TABLE table,
                          IN PDEKUCHAMP_URB_IRP_INFO irpinfo)
{
    DEKUCHAMP_INTERNAL_URB_IRP_INFO   info;
    BOOLEAN                         new;

    info.info = *irpinfo;

    RtlInsertElementGenericTable(table,
                                 (PVOID)&info,
                                 sizeof(DEKUCHAMP_INTERNAL_URB_IRP_INFO),
                                 &new);

    if (new == FALSE)
    {
        DkDbgVal("Element already exists in table", irpinfo->irp);
    }
}

static PDEKUCHAMP_URB_IRP_INFO
DekuchampGetURBIRPInfo(IN PRTL_GENERIC_TABLE table,
                     IN PIRP irp)
{
    PDEKUCHAMP_INTERNAL_URB_IRP_INFO  pInfo;
    DEKUCHAMP_INTERNAL_URB_IRP_INFO   info;

    info.info.irp = irp;

    pInfo = RtlLookupElementGenericTable(table, &info);

    if (pInfo == NULL)
    {
        return NULL;
    }

    return &pInfo->info;
}

VOID DekuchampFreeURBIRPInfoTable(IN PRTL_GENERIC_TABLE table)
{
    PVOID element;

    DkDbgStr("Free URB irp data");

    /* Delete all the elements from table */
    while (NULL != (element = RtlGetElementGenericTable(table, 0)))
    {
        RtlDeleteElementGenericTable(table, element);
    }

    /* Delete table structure */
    ExFreePool(table);
}

RTL_GENERIC_COMPARE_ROUTINE DekuchampCompareURBIRPInfo;
static RTL_GENERIC_COMPARE_RESULTS
DekuchampCompareURBIRPInfo(IN PRTL_GENERIC_TABLE table,
                         IN PVOID first,
                         IN PVOID second)
{
    PDEKUCHAMP_INTERNAL_URB_IRP_INFO left = first;
    PDEKUCHAMP_INTERNAL_URB_IRP_INFO right = second;

    if (left->info.irp < right->info.irp)
    {
        return GenericLessThan;
    }
    else if (left->info.irp == right->info.irp)
    {
        return GenericEqual;
    }
    else
    {
        return GenericGreaterThan;
    }
}

PRTL_GENERIC_TABLE DekuchampInitializeURBIRPInfoTable(IN PVOID context)
{
    PRTL_GENERIC_TABLE table;

    DkDbgStr("Initialize URB irp table");

    table = (PRTL_GENERIC_TABLE)
                ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(RTL_GENERIC_TABLE),
                                      DEKUCHAMP_TABLE_TAG);

    if (table == NULL)
    {
        DkDbgStr("Unable to allocate URB irp table");
        return table;
    }

    RtlInitializeGenericTable(table,
                              DekuchampCompareURBIRPInfo,
                              DekuchampAllocateRoutine,
                              DekuchampFreeRoutine,
                              context);

    return table;
}

/* Obtains the URB IRP info from the URB IRP info table
 *
 * If the value was present in the table, it will be removed.
 *
 * Returns TRUE if irp was found in table, FALSE otherwise.
 */
BOOLEAN DekuchampObtainURBIRPInfo(IN PDEKUCHAMP_DEVICE_DATA pDeviceData,
                                IN PIRP irp,
                                PDEKUCHAMP_URB_IRP_INFO pInfo)
{
    KIRQL irql;
    PDEKUCHAMP_URB_IRP_INFO info;
    BOOLEAN found = FALSE;

    KeAcquireSpinLock(&pDeviceData->tablesSpinLock, &irql);
    info = DekuchampGetURBIRPInfo(pDeviceData->URBIrpTable, irp);
    if (info != NULL)
    {
        found = TRUE;
        memcpy(pInfo, info, sizeof(DEKUCHAMP_URB_IRP_INFO));
        DekuchampRemoveURBIRPInfo(pDeviceData->URBIrpTable, irp);
    }
    KeReleaseSpinLock(&pDeviceData->tablesSpinLock, irql);

    if (found == TRUE)
    {
        DkDbgVal("Found URB irp info", irp);
        DkDbgVal("", pInfo->function);
    }

    return found;
}
