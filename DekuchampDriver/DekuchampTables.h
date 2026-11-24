/*
 * Copyright (c) 2013-2019 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0
 */

#ifndef DEKUCHAMP_TABLES_H
#define DEKUCHAMP_TABLES_H

#include "DekuchampMain.h"

typedef struct _DEKUCHAMP_ENDPOINT_INFO
{
    /* handle is used as a key */
    USBD_PIPE_HANDLE  handle;
    USBD_PIPE_TYPE    type;
    UCHAR             endpointAddress;
    USHORT            deviceAddress;
} DEKUCHAMP_ENDPOINT_INFO, *PDEKUCHAMP_ENDPOINT_INFO;

VOID DekuchampRemoveEndpointInfo(IN PRTL_GENERIC_TABLE table,
                               IN USBD_PIPE_HANDLE handle);
VOID DekuchampAddEndpointInfo(IN PRTL_GENERIC_TABLE table,
                            IN PUSBD_PIPE_INFORMATION pipeInfo,
                            IN USHORT deviceAddress);

PDEKUCHAMP_ENDPOINT_INFO DekuchampGetEndpointInfo(IN PRTL_GENERIC_TABLE table,
                                              IN USBD_PIPE_HANDLE handle);

VOID DekuchampFreeEndpointTable(IN PRTL_GENERIC_TABLE table);
PRTL_GENERIC_TABLE DekuchampInitializeEndpointTable(IN PVOID context);


BOOLEAN DekuchampRetrieveEndpointInfo(IN PDEKUCHAMP_DEVICE_DATA pDeviceData,
                                    IN USBD_PIPE_HANDLE handle,
                                    PDEKUCHAMP_ENDPOINT_INFO pInfo);

typedef struct _DEKUCHAMP_URB_IRP_INFO
{
    /* IRP pointer is used as a key */
    PIRP          irp;
    /* Data collected when the URB was travelling from FDO to PDO */
    LARGE_INTEGER timestamp;
    USBD_STATUS   status;
    USHORT        function;
    UCHAR         info;      /* I/O Request info */
    USHORT        bus;       /* bus (RootHub) number */
    USHORT        device;    /* device address */
} DEKUCHAMP_URB_IRP_INFO, *PDEKUCHAMP_URB_IRP_INFO;

VOID DekuchampRemoveURBIRPInfo(IN PRTL_GENERIC_TABLE table,
                             IN PIRP irp);
VOID DekuchampAddURBIRPInfo(IN PRTL_GENERIC_TABLE table,
                          IN PDEKUCHAMP_URB_IRP_INFO irpinfo);

VOID DekuchampFreeURBIRPInfoTable(IN PRTL_GENERIC_TABLE table);
PRTL_GENERIC_TABLE DekuchampInitializeURBIRPInfoTable(IN PVOID context);

BOOLEAN DekuchampObtainURBIRPInfo(IN PDEKUCHAMP_DEVICE_DATA pDeviceData,
                                IN PIRP irp,
                                PDEKUCHAMP_URB_IRP_INFO pInfo);

#endif /* DEKUCHAMP_TABLES_H */
