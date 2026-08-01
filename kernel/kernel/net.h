// network glue for ARP requests
#ifndef KERNEL_NET_H
#define KERNEL_NET_H

#include <stdint.h>
#include <stdbool.h>

#define NET_OUR_IP 0x0A00020Fu
#define NET_GATEWAY_IP 0x0A000202u
#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_IP 0x0800
#define IP_PROTO_ICMP 1
#define IP_PROTO_UDP 17

uint16_t htons(uint16_t v);
uint32_t htonl(uint32_t v);
#define ntohs(v) htons(v)
#define ntohl(v) htonl(v)

uint16_t net_checksum(const void* data, uint32_t len); // 1s compliment checksum
void arp_send_request(uint32_t target_ip); // broadcast the arp
bool arp_lookup(uint32_t ip, uint8_t* mac_out); // look up a cached mac for an ip
bool icmp_send_echo(uint32_t dest_ip); // Send an ICMP Echo request
void net_ping(uint32_t dest_ip); // Safe call
void net_poll(void); // Poll request

#endif