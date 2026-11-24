/*
 * Copyright (c) 2013-2019 Tomasz Moń <desowin@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef DEKUCHAMP_H
#define DEKUCHAMP_H

#ifdef __cplusplus
extern "C" {
#endif

#include <usb.h>

typedef struct
{
    UINT32  size;
} DEKUCHAMP_IOCTL_SIZE, *PDEKUCHAMP_IOCTL_SIZE;

#pragma pack(push)
#pragma pack(1)
/* DEKUCHAMP_ADDRESS_FILTER is parameter structure to IOCTL_DEKUCHAMP_START_FILTERING. */
typedef struct _DEKUCHAMP_ADDRESS_FILTER
{
    /* Individual device filter bit array. USB standard assigns device
     * numbers 1 to 127 (0 is reserved for initial configuration).
     *
     * If address 0 bit is set, then we will automatically capture from
     * newly connected devices.
     *
     * addresses[0] - 0 - 31
     * addresses[1] - 32 - 63
     * addresses[2] - 64 - 95
     * addresses[3] - 96 - 127
     */
    UINT32 addresses[4];

    /* Filter all devices */
    BOOLEAN filterAll;
} DEKUCHAMP_ADDRESS_FILTER, *PDEKUCHAMP_ADDRESS_FILTER;
#pragma pack(pop)

#define IOCTL_DEKUCHAMP_SETUP_BUFFER \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x800, METHOD_BUFFERED, FILE_READ_ACCESS)

#define IOCTL_DEKUCHAMP_START_FILTERING \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_DEKUCHAMP_STOP_FILTERING \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_ACCESS | FILE_WRITE_ACCESS)

#define IOCTL_DEKUCHAMP_GET_HUB_SYMLINK \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x803, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_DEKUCHAMP_SET_SNAPLEN_SIZE \
    CTL_CODE(FILE_DEVICE_UNKNOWN, 0x804, METHOD_BUFFERED, FILE_READ_ACCESS)

/* USB packets, beginning with a Dekuchamp header */
#define DLT_DEKUCHAMP         249

#pragma pack(push, 1)
typedef struct pcap_hdr_s {
    UINT32 magic_number;   /* magic number */
    UINT16 version_major;  /* major version number */
    UINT16 version_minor;  /* minor version number */
    INT32  thiszone;       /* GMT to local correction */
    UINT32 sigfigs;        /* accuracy of timestamps */
    UINT32 snaplen;        /* max length of captured packets, in octets */
    UINT32 network;        /* data link type */
} pcap_hdr_t;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct pcaprec_hdr_s {
    UINT32 ts_sec;         /* timestamp seconds */
    UINT32 ts_usec;        /* timestamp microseconds */
    UINT32 incl_len;       /* number of octets of packet saved in file */
    UINT32 orig_len;       /* actual length of packet */
} pcaprec_hdr_t;
#pragma pack(pop)

/* All multi-byte fields are stored in .pcap file in little endian */

#define DEKUCHAMP_TRANSFER_ISOCHRONOUS 0
#define DEKUCHAMP_TRANSFER_INTERRUPT   1
#define DEKUCHAMP_TRANSFER_CONTROL     2
#define DEKUCHAMP_TRANSFER_BULK        3
#define DEKUCHAMP_TRANSFER_IRP_INFO    0xFE
#define DEKUCHAMP_TRANSFER_UNKNOWN     0xFF

/* info byte fields:
 * bit 0 (LSB) - when 1: PDO -> FDO
 * bits 1-7: Reserved
 */
#define DEKUCHAMP_INFO_PDO_TO_FDO  (1 << 0)

#pragma pack(push, 1)
typedef struct
{
    USHORT       headerLen; /* This header length */
    UINT64       irpId;     /* I/O Request packet ID */
    USBD_STATUS  status;    /* USB status code (on return from host controller) */
    USHORT       function;  /* URB Function */
    UCHAR        info;      /* I/O Request info */

    USHORT       bus;       /* bus (RootHub) number */
    USHORT       device;    /* device address */
    UCHAR        endpoint;  /* endpoint number and transfer direction */
    UCHAR        transfer;  /* transfer type */

    UINT32       dataLength;/* Data length */
} DEKUCHAMP_BUFFER_PACKET_HEADER, *PDEKUCHAMP_BUFFER_PACKET_HEADER;
#pragma pack(pop)

/* Dekuchamp versions before 1.5.0.0 recorded control transactions as two
 * or three pcap packets:
 *   * DEKUCHAMP_CONTROL_STAGE_SETUP with 8 bytes USB SETUP data
 *   * Optional DEKUCHAMP_CONTROL_STAGE_DATA with either DATA OUT or IN
 *   * DEKUCHAMP_CONTROL_STAGE_STATUS without data on IRP completion
 *
 * Such capture was considered unnecessary complex. Due to that, since
 * Dekuchamp 1.5.0.0, the control transactions are recorded as two packets:
 *   * DEKUCHAMP_CONTROL_STAGE_SETUP with 8 bytes USB SETUP data and
 *     optional DATA OUT
 *   * DEKUCHAMP_CONTROL_STAGE_COMPLETE without payload or with the DATA IN
 *
 * The merit behind this change was that Wireshark dissector, since the
 * very first time when Wireshark understood Dekuchamp format, was really
 * expecting the DEKUCHAMP_CONTROL_STAGE_SETUP to contain SETUP + DATA OUT.
 * Even when they Wireshark doesn't recognize DEKUCHAMP_CONTROL_STAGE_COMPLETE
 * it will still process the payload correctly.
 */
#define DEKUCHAMP_CONTROL_STAGE_SETUP    0
#define DEKUCHAMP_CONTROL_STAGE_DATA     1
#define DEKUCHAMP_CONTROL_STAGE_STATUS   2
#define DEKUCHAMP_CONTROL_STAGE_COMPLETE 3

#pragma pack(push, 1)
typedef struct
{
    DEKUCHAMP_BUFFER_PACKET_HEADER  header;
    UCHAR                         stage;
} DEKUCHAMP_BUFFER_CONTROL_HEADER, *PDEKUCHAMP_BUFFER_CONTROL_HEADER;
#pragma pack(pop)

/* Note about isochronous packets:
 *   packet[x].length, packet[x].status and errorCount are only relevant
 *   when DEKUCHAMP_INFO_PDO_TO_FDO is set
 *
 *   packet[x].length is not used for isochronous OUT transfers.
 *
 * Buffer data is attached to:
 *   * for isochronous OUT transactions (write to device)
 *       Requests (DEKUCHAMP_INFO_PDO_TO_FDO is not set)
 *   * for isochronous IN transactions (read from device)
 *       Responses (DEKUCHAMP_INFO_PDO_TO_FDO is set)
 */
#pragma pack(push, 1)
typedef struct
{
    ULONG        offset;
    ULONG        length;
    USBD_STATUS  status;
} DEKUCHAMP_BUFFER_ISO_PACKET, *PDEKUCHAMP_BUFFER_ISO_PACKET;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct
{
    DEKUCHAMP_BUFFER_PACKET_HEADER  header;
    ULONG                         startFrame;
    ULONG                         numberOfPackets;
    ULONG                         errorCount;
    DEKUCHAMP_BUFFER_ISO_PACKET     packet[1];
} DEKUCHAMP_BUFFER_ISOCH_HEADER, *PDEKUCHAMP_BUFFER_ISOCH_HEADER;
#pragma pack(pop)

#ifdef __cplusplus
}
#endif

#endif /* DEKUCHAMP_H */
