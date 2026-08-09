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
#define IP_PROTO_TCP 6
#define IP_PROTO_UDP 17

#define DNS_SERVER_IP 0x0A000203u
#define DNS_PORT 53

typedef enum {
    TCP_CLOSED = 0,
    TCP_SYN_SENT,
    TCP_ESTABLISHED,
    TCP_FIN_WAIT_1,
    TCP_FIN_WAIT_2,
    TCP_TIME_WAIT,
    TCP_CLOSE_WAIT,
    TCP_LAST_ACK,
} tcp_state_t;

uint16_t htons(uint16_t v);
uint32_t htonl(uint32_t v);
#define ntohs(v) htons(v)
#define ntohl(v) htonl(v)

typedef void (*udp_handler_t)(uint32_t src_ip, uint16_t src_port, const uint8_t* data, uint16_t len);
typedef void (*tcp_recv_handler_t)(const uint8_t* data, uint16_t len);
typedef void (*dns_handler_t)(const char* name, uint32_t ip);

uint16_t net_checksum(const void* data, uint32_t len); // 1s compliment checksum
void arp_send_request(uint32_t target_ip); // broadcast the arp
bool arp_lookup(uint32_t ip, uint8_t* mac_out); // look up a cached mac for an ip
bool icmp_send_echo(uint32_t dest_ip); // Send an ICMP Echo request
void net_ping(uint32_t dest_ip); // Safe call
void net_poll(void); // Poll request
bool udp_bind(uint16_t port, udp_handler_t handler);
bool udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, const void* data, uint16_t len);
bool tcp_connect(uint32_t dst_ip, uint16_t dst_port, tcp_recv_handler_t on_data);
bool tcp_send(const void* data, uint16_t len);
void tcp_close(void);
void dns_init(void);
bool dns_resolve(const char* name, dns_handler_t handler);
tcp_state_t tcp_get_state(void);


#endif