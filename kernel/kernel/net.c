#include "net.h"
#include "../arch/i386/e1000.h"
#include <stdio.h>
#include <string.h>

#define ETHERTYPE_ARP 0x0806
#define ETHERTYPE_IP 0x0800

// network byte order = big-endian
static uint16_t htons(uint16_t v) { return (uint16_t)((v << 8) | (v >> 8)); }
static uint32_t htonl (uint32_t v) {
    return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) | ((v & 0x00FF0000u) >> 8) | ((v & 0xFF000000) >> 24);
}

typedef struct {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t ethertype;
}__attribute__((packed)) eth_header_t;

typedef struct {
    uint16_t htype;
    uint16_t ptype;
    uint8_t hlen;
    uint8_t plen;
    uint16_t oper;
    uint8_t sha[6];
    uint32_t spa;
    uint8_t tha[6];
    uint32_t tpa;
}__attribute__((packed)) arp_packet_t;

void arp_send_request(uint32_t target_ip) {
    uint8_t frame[sizeof(eth_header_t) + sizeof(arp_packet_t)];
    memset(frame, 0, sizeof(frame));

    eth_header_t* eth = (eth_header_t*)frame;
    memset(eth->dst, 0xFF, 6);
    memcpy(eth->src, e1000_mac(), 6);
    eth->ethertype = htons(ETHERTYPE_ARP);

    arp_packet_t* arp = (arp_packet_t*)(frame + sizeof(eth_header_t));
    arp->htype = htons(1);
    arp->ptype = htons(ETHERTYPE_IP);
    arp->hlen = 6;
    arp->plen = 4;
    arp->oper = htons(1);
    memcpy(arp->sha, e1000_mac(), 6);
    arp->spa = htonl(NET_OUR_IP);
    memset(arp->tha, 0, 6);
    arp->tpa = htonl(target_ip);

    printf("NET ARP: who has %d.%d.%d.%d?\n", (target_ip >> 24) & 0xFF, (target_ip >> 16) & 0xFF, (target_ip >> 8) & 0xFF, target_ip & 0xFF);
    e1000_send(frame, sizeof(frame));
}

static void print_mac(const uint8_t* m) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
}

void net_poll(void) {
    static uint8_t frame[E1000_FRAME_MAX];
    uint16_t len;

    while (e1000_poll_frame(frame, &len)) {
        if (len < sizeof(eth_header_t)) continue;
        eth_header_t* eth = (eth_header_t*)frame;
        uint16_t type = htons(eth->ethertype);

        printf("NET RX %d bytes from ", len);
        print_mac(eth->src);
        printf(" type=0x%x", type);

        if (type == ETHERTYPE_ARP && len >= sizeof(eth_header_t) + sizeof(arp_packet_t)) {
            arp_packet_t* arp = (arp_packet_t*)(frame + sizeof(eth_header_t));
            uint16_t oper = htons(arp->oper);
            uint32_t spa = htonl(arp->spa);
            printf(" ARP %s %d.%d.%d.%d is ", oper == 2 ? "reply:" : "request:", (spa >> 24) & 0xFF, (spa >> 16) & 0xFF, (spa >> 8) & 0xFF, spa & 0xFF);
            print_mac(arp->sha);
        }
        printf("\n");
    }
}