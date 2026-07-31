// network glue for ARP requests
#ifndef KERNEL_NET_H
#define KERNEL_NET_H

#include <stdint.h>

#define NET_OUR_IP 0x0A00020F
#define NET_GATEWAY_IP 0x0A000202

void arp_send_request(uint32_t target_ip);
void net_poll(void);

#endif