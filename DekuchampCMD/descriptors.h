/*
 * Copyright (c) 2013 Tomasz Moñ <desowin@gmail.com>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#ifndef DEKUCHAMP_DESCRIPTORS_H
#define DEKUCHAMP_DESCRIPTORS_H

#include "iocontrol.h"

void *descriptors_generate_pcap(const char *filter, int *pcap_length, PDEKUCHAMP_ADDRESS_FILTER addresses);
void descriptors_free_pcap(void *pcap);

#endif /* DEKUCHAMP_DESCRIPTORS_H */
