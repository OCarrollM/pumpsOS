#include "net.h"
#include "../arch/i386/e1000.h"
#include <stdio.h>
#include <string.h>

// network byte order = big-endian
uint16_t htons(uint16_t v) { return (uint16_t)((v << 8) | v >> 8); }
uint32_t htonl (uint32_t v) {
    return ((v & 0x000000FFu) << 24) | ((v & 0x0000FF00u) << 8) | ((v & 0x00FF0000u) >> 8) | ((v & 0xFF000000) >> 24);
}

// Packet layouts

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

typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t id;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;
}__attribute__((packed)) ip_header_t;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t id;
    uint16_t sequence;
}__attribute__((packed)) icmp_header_t;

// Checksum (1's compliment) sum of 16-bit words. Building over the packet
uint16_t net_checksum(const void* data, uint32_t len) {
    const uint8_t* p = (const uint8_t*)data;
    uint32_t sum = 0;
    while (len > 1) {
        sum += (uint32_t)((p[0] << 8) | p[1]);
        p += 2;
        len -= 2;
    }
    if (len) sum += (uint32_t)(p[0] << 8);
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    
    return htons((uint16_t)(~sum));
}

// ARP cache
#define ARP_CACHE_SIZE 8
typedef struct {
    uint32_t ip;
    uint8_t mac[6];
    bool valid;
} arp_entry_t;

static arp_entry_t arp_cache[ARP_CACHE_SIZE];

static void arp_cache_put(uint32_t ip, const uint8_t* mac) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            memcpy(arp_cache[i].mac, mac, 6);
            return;
        }
    }
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (!arp_cache[i].valid) {
            arp_cache[i].ip = ip;
            memcpy(arp_cache[i].mac, mac, 6);
            arp_cache[i].valid = true;
            return;
        }
    }
}

bool arp_lookup(uint32_t ip, uint8_t* mac_out) {
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && arp_cache[i].ip == ip) {
            memcpy(mac_out, arp_cache[i].mac, 6);
            return true;
        }
    }
    return false;
}

// Helper functions
static void print_ip(uint32_t ip) {
    printf("%d.%d.%d.%d", (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF);
}

static void print_mac(const uint8_t* m) {
    printf("%02x:%02x:%02x:%02x:%02x:%02x", m[0], m[1], m[2], m[3], m[4], m[5]);
}

// Fill in ethernet header
static void eth_build(uint8_t* frame, const uint8_t* dst, uint16_t type) {
    eth_header_t* eth = (eth_header_t*)frame;
    memcpy(eth->dst, dst, 6);
    memcpy(eth->src, e1000_mac(), 6);
    eth->ethertype = htons(type);
}

// arpin all over the place

void arp_send_request(uint32_t target_ip) {
    uint8_t frame[sizeof(eth_header_t) + sizeof(arp_packet_t)];
    memset(frame, 0, sizeof(frame));

    uint8_t broadcast[6];
    memset(broadcast, 0xFF, 6);
    eth_build(frame, broadcast, ETHERTYPE_ARP);

    arp_packet_t* arp = (arp_packet_t*)(frame + sizeof(eth_header_t));
    arp->htype = htons(1);
    arp->ptype = htons(ETHERTYPE_IP);
    arp->hlen = 6;
    arp->plen = 4;
    arp->oper = htons(1);
    memcpy(arp->sha, e1000_mac(), 6);
    arp->spa = htonl(NET_OUR_IP);
    arp->tpa = htonl(target_ip);

    printf("NET ARP: Who has ");
    print_ip(target_ip);
    printf("?\n");

    e1000_send(frame, sizeof(frame));
}

static void arp_send_reply(const arp_packet_t* req) {
    uint8_t frame[sizeof(eth_header_t) + sizeof(arp_packet_t)];
    memset(frame, 0, sizeof(frame));
    eth_build(frame, req->sha, ETHERTYPE_ARP);

    arp_packet_t* arp = (arp_packet_t*)(frame + sizeof(eth_header_t));
    arp->htype = htons(1);
    arp->ptype = htons(ETHERTYPE_IP);
    arp->hlen = 6;
    arp->plen = 4;
    arp->oper = htons(2);
    memcpy(arp->sha, e1000_mac(), 6);
    arp->spa = htonl(NET_OUR_IP);
    memcpy(arp->tha, req->sha, 6);
    arp->tpa = req->spa;

    e1000_send(frame, sizeof(frame));
}

static void arp_handle(const uint8_t* frame, uint16_t len) {
    if (len < sizeof(eth_header_t) + sizeof(arp_packet_t)) return;
    const arp_packet_t* arp = (const arp_packet_t*)(frame + sizeof(eth_header_t));

    uint32_t spa = ntohl(arp->spa);
    uint32_t tpa = ntohl(arp->tpa);
    uint16_t oper = ntohs(arp->oper);

    // Learn from it all
    arp_cache_put(spa, arp->sha);

    if (oper == 1 && tpa == NET_OUR_IP) {
        printf("NET ARP request for us -> replying\n");
        arp_send_reply(arp);
    } else if (oper == 2) {
        printf("NET ARP reply: ");
        print_ip(spa);
        printf(" is ");
        print_mac(arp->sha);
        printf("\n");
    }
}

// IP stuf
static uint16_t ip_id_counter = 0;
static bool ip_send(uint32_t dst_ip, uint8_t protocol, uint8_t* frame, uint16_t payload_len) {
    uint8_t dst_mac[6];

    uint32_t next_hop = ((dst_ip & 0xFFFFFF00u) == (NET_OUR_IP & 0xFFFFFF00u)) ? dst_ip : NET_GATEWAY_IP;

    if (!arp_lookup(next_hop, dst_mac)) {
        printf("NET no ARP entry for ");
        print_ip(next_hop);
        printf(" -- send an ARP request first please\n");
        return false;
    }

    eth_build(frame, dst_mac, ETHERTYPE_IP);

    ip_header_t* ip = (ip_header_t*)(frame + sizeof(eth_header_t));
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_length = htons(sizeof(ip_header_t) + payload_len);
    ip->id = htons(ip_id_counter++);
    ip->flags_fragment = 0;
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->checksum = 0;
    ip->src_ip = htonl(NET_OUR_IP);
    ip->dst_ip = htonl(dst_ip);
    ip->checksum = net_checksum(ip, sizeof(ip_header_t));

    uint16_t total = sizeof(eth_header_t) + sizeof(ip_header_t) + payload_len;

    return e1000_send(frame, total);
}

// I unironically love how networking works in the first place

// ICMP
#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY 0
#define ICMP_PAYLOAD_LEN 32

static uint16_t icmp_sequence = 0;

bool icmp_send_echo(uint32_t dst_ip) {
    uint8_t frame[sizeof(eth_header_t) + sizeof(ip_header_t) + sizeof(icmp_header_t) + ICMP_PAYLOAD_LEN];
    memset(frame, 0, sizeof(frame));

    icmp_header_t* icmp = (icmp_header_t*)(frame + sizeof(eth_header_t) + sizeof(ip_header_t));
    icmp->type = ICMP_ECHO_REQUEST;
    icmp->code = 0;
    icmp->id = htons(0x1234);
    icmp->sequence = htons(icmp_sequence++);

    uint8_t* payload = (uint8_t*)icmp + sizeof(icmp_header_t);
    for (int i = 0; i < ICMP_PAYLOAD_LEN; i++) {
        payload[i] = (uint8_t)('a' + (i % 26));
    }

    icmp->checksum = 0;
    icmp->checksum = net_checksum(icmp, sizeof(icmp_header_t) + ICMP_PAYLOAD_LEN);

    // printf("NET ping ");
    // print_ip(dst_ip);
    // printf(" seq=%d\n", ntohs(icmp->sequence));

    return ip_send(dst_ip, IP_PROTO_ICMP, frame, sizeof(icmp_header_t) + ICMP_PAYLOAD_LEN);
}

// reply to an echo request
static void icmp_send_reply(uint32_t dst_ip, const icmp_header_t* req, uint16_t icmp_len) {
    uint8_t frame[sizeof(eth_header_t) + sizeof(ip_header_t) + 1500];
    if (icmp_len > 1500) return;
    memset(frame, 0, sizeof(frame));

    icmp_header_t* icmp = (icmp_header_t*)(frame + sizeof(eth_header_t) + sizeof(ip_header_t));
    memcpy(icmp, req, icmp_len);
    icmp->type = ICMP_ECHO_REPLY;
    icmp->checksum = 0;
    icmp->checksum = net_checksum(icmp, icmp_len);

    ip_send(dst_ip, IP_PROTO_ICMP, frame, icmp_len);
}

static void icmp_handle(uint32_t src_ip, const icmp_header_t* icmp, uint16_t icmp_len) {
    if (icmp->type == ICMP_ECHO_REPLY) {
        //printf("NET ping reply from ");
        //print_ip(src_ip);
        //printf(" seq=%d\n", ntohs(icmp->sequence));
    } else if (icmp->type == ICMP_ECHO_REQUEST) {
        // printf("NET ping request from ");
        // print_ip(src_ip);
        // printf(" -> replying\n");
        icmp_send_reply(src_ip, icmp, icmp_len);
    }
}

// UDP
typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t len;
    uint16_t checksum;
}__attribute__((packed)) udp_header_t; // 8 bytes in total, small

#define UDP_MAX_BINDINGS 8
#define UDP_MAX_PAYLOAD 1024

typedef struct {
    uint16_t port;
    udp_handler_t handler;
    bool used;
} udp_binding_t; // Binder

static udp_binding_t udp_bindings[UDP_MAX_BINDINGS];

bool udp_bind(uint16_t port, udp_handler_t handler) {
    for (int i = 0; i < UDP_MAX_BINDINGS; i++) {
        if (udp_bindings[i].used && udp_bindings[i].port == port) {
            return false;
        }
    }
    for (int i = 0; i < UDP_MAX_BINDINGS; i++) {
        if (!udp_bindings[i].used) {
            udp_bindings[i].port = port;
            udp_bindings[i].handler = handler;
            udp_bindings[i].used = true;
            printf("NET UDP bound to port %d\n", port);
            return true;
        }
    }
    return false;
}

bool udp_send(uint32_t dst_ip, uint16_t src_port, uint16_t dst_port, const void* data, uint16_t len) {
    if (len > UDP_MAX_PAYLOAD) return false;

    uint8_t frame[sizeof(eth_header_t) + sizeof(ip_header_t) + sizeof(udp_header_t) + UDP_MAX_PAYLOAD];
    memset(frame, 0, sizeof(eth_header_t) + sizeof(ip_header_t) + sizeof(udp_header_t));
    udp_header_t* udp = (udp_header_t*)(frame + sizeof(eth_header_t) + sizeof(ip_header_t));

    udp->src_port = htons(src_port);
    udp->dst_port = htons(dst_port);
    udp->len = htons(sizeof(udp_header_t) + len);
    udp->checksum = 0;
    memcpy((uint8_t*)udp + sizeof(udp_header_t), data, len);

    return ip_send(dst_ip, IP_PROTO_UDP, frame, sizeof(udp_header_t) + len);
}

static void udp_handle(uint32_t src_ip, const udp_header_t* udp, uint16_t udp_len) {
    if (udp_len < sizeof(udp_header_t)) return;

    uint16_t dst_port = ntohs(udp->dst_port);
    uint16_t src_port = ntohs(udp->src_port);

    // Trust headers own length
    uint16_t stated = ntohs(udp->len);
    if (stated > udp_len) stated = udp_len;
    uint16_t payload_len = stated - sizeof(udp_header_t);
    const uint8_t* payload = (const uint8_t*)udp + sizeof(udp_header_t);

    for (int i = 0; i < UDP_MAX_BINDINGS; i++) {
        if (udp_bindings[i].used && udp_bindings[i].port == dst_port) {
            udp_bindings[i].handler(src_ip, src_port, payload, payload_len);
            return;
        }
    }

    printf("NET UDP to unbound port %d from ", dst_port);
    print_ip(src_ip);
    printf(":%d (%d bytes)\n", src_port, payload_len);
}

// TCP

#define TCP_FIN (1 << 0)
#define TCP_SYN (1 << 1)
#define TCP_RST (1 << 2)
#define TCP_PSH (1 << 3)
#define TCP_ACK (1 << 4)

typedef struct {
    uint16_t src_port;
    uint16_t dst_port;
    uint32_t seq;
    uint32_t ack;
    uint8_t data_offset;
    uint8_t flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urgent;
}__attribute__((packed)) tcp_header_t;

#define TCP_WINDOW 4096

static struct {
    tcp_state_t state;
    uint32_t remote_ip;
    uint32_t remote_port;
    uint16_t local_port;
    uint32_t snd_nxt;
    uint32_t snd_una;
    uint32_t rcv_nxt;
    tcp_recv_handler_t on_data;
} tcb;

tcp_state_t tcp_get_state(void) { return tcb.state; }

static uint16_t tcp_checksum(uint32_t src_ip, uint32_t dst_ip, const void* tcp_seg, uint16_t tcp_len) {
    uint32_t sum = 0;
    const uint8_t* p = (const uint8_t*)tcp_seg;

    // pseudo header
    sum += (src_ip >> 16) & 0xFFFF;
    sum += src_ip & 0xFFFF;
    sum += (dst_ip >> 16) & 0xFFFF;
    sum += dst_ip & 0xFFFF;
    sum += IP_PROTO_TCP;
    sum += tcp_len;

    // segment
    uint16_t len = tcp_len;
    while (len > 1) {
        sum += (uint32_t)((p[0] << 8) | p[1]);
        p += 2;
        len -= 2;
    }
    if (len) sum += (uint32_t)(p[0] << 8);

    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return htons((uint16_t)(~sum));
}

// Build and send a segment
static bool tcp_transmit(uint8_t flags, const void* payload, uint16_t len) {
    uint8_t frame[sizeof(eth_header_t) + sizeof(ip_header_t) + sizeof(tcp_header_t) + 1024];
    if (len > 1024) return false;

    memset(frame, 0, sizeof(eth_header_t) + sizeof(ip_header_t) + sizeof(tcp_header_t));
    tcp_header_t* tcp = (tcp_header_t*)(frame + sizeof(eth_header_t) + sizeof(ip_header_t));

    tcp->src_port = htons(tcb.local_port);
    tcp->dst_port = htons(tcb.remote_port);
    tcp->seq = htonl(tcb.snd_nxt);
    tcp->ack = htonl(tcb.rcv_nxt);
    tcp->data_offset = (sizeof(tcp_header_t) / 4) << 4;
    tcp->flags = flags;
    tcp->window = htons(TCP_WINDOW);
    tcp->urgent = 0;

    if (payload && len) {
        memcpy((uint8_t*)tcp + sizeof(tcp_header_t), payload, len);
    }

    uint16_t seg_len = sizeof(tcp_header_t) + len;
    tcp->checksum = 0;
    tcp->checksum = tcp_checksum(NET_OUR_IP, tcb.remote_ip, tcp, seg_len);

    printf("[TCP] transmitting %d bytes, flags=0x%x\n", seg_len, flags);
    bool ok = ip_send(tcb.remote_ip, IP_PROTO_TCP, frame, seg_len);
    printf("[TCP] ip_send returned %d\n", ok);
    return ok;
}

bool tcp_connect(uint32_t dst_ip, uint16_t dst_port, tcp_recv_handler_t on_data) {
    if (tcb.state != TCP_CLOSED) {
        printf("TCP already in use\n");
        return false;
    }

    memset(&tcb, 0, sizeof(tcb));
    tcb.remote_ip = dst_ip;
    tcb.remote_port = dst_port;
    tcb.local_port = 49152 + (ip_id_counter & 0x3FF);
    tcb.snd_nxt = 12345;
    tcb.snd_una = tcb.snd_nxt;
    tcb.rcv_nxt = 0;
    tcb.on_data = on_data;
    tcb.state = TCP_SYN_SENT;

    printf("[TCP] connecting to %d.%d.%d.%d:%d from port %d\n",
        (dst_ip >> 24) & 0xFF, (dst_ip >> 16) & 0xFF,
        (dst_ip >> 8) & 0xFF, dst_ip & 0xFF, dst_port, tcb.local_port);

    return tcp_transmit(TCP_SYN, 0, 0);
}

bool tcp_send(const void* data, uint16_t len) {
    if (tcb.state != TCP_ESTABLISHED) {
        printf("TCP not established\n");
        return false;
    }
    return tcp_transmit(TCP_ACK | TCP_PSH, data, len);
}

void tcp_close(void) {
    if (tcb.state == TCP_ESTABLISHED) {
        tcp_transmit(TCP_ACK | TCP_FIN, 0, 0);
        tcb.state = TCP_FIN_WAIT_1;
    } else if (tcb.state == TCP_CLOSE_WAIT) {
        tcp_transmit(TCP_ACK | TCP_FIN, 0, 0);
        tcb.state = TCP_LAST_ACK;
    }
}

static void tcp_handle(uint32_t src_ip, const tcp_header_t* tcp, uint16_t seg_len) {
    printf("[TCP] seg from %x flags=0x%x len=%d\n", src_ip, tcp->flags, seg_len);
    if (seg_len < sizeof(tcp_header_t)) return;
    if (src_ip != tcb.remote_ip) return;
    if (ntohs(tcp->dst_port) != tcb.local_port) return;

    uint8_t flags = tcp->flags;
    uint32_t seq = ntohl(tcp->seq);
    uint32_t ack = ntohl(tcp->ack);
    uint16_t hdr_len = (tcp->data_offset >> 4) * 4;

    if (hdr_len > seg_len) return;
    uint16_t data_len = seg_len - hdr_len;
    const uint8_t* data = (const uint8_t*)tcp + hdr_len;

    if (flags & TCP_RST) {
        printf("TCP connection reset\n");
        tcb.state = TCP_CLOSED;
        return;
    }

    switch (tcb.state) {
        case TCP_SYN_SENT:
            if ((flags & TCP_SYN) && (flags & TCP_ACK)) {
                tcb.rcv_nxt = seq + 1;
                tcb.snd_una = ack;
                tcb.state = TCP_ESTABLISHED;
                tcp_transmit(TCP_ACK, 0, 0);
                printf("TCP Established\n");
            }
            break;
        case TCP_ESTABLISHED:
            if (flags & TCP_ACK) tcb.snd_una = ack;
            if (data_len && seq == tcb.rcv_nxt) {
                tcb.rcv_nxt += data_len;
                if (tcb.on_data) tcb.on_data(data, data_len);
                tcp_transmit(TCP_ACK, 0, 0);
            }
            if (flags & TCP_FIN) {
                tcb.rcv_nxt++;
                tcp_transmit(TCP_ACK, 0, 0);
                tcb.state = TCP_CLOSE_WAIT;
                printf("TCP peer closed\n");
            }
            break;
        case TCP_FIN_WAIT_1:
            if (flags & TCP_ACK) tcb.state = TCP_FIN_WAIT_2;
            if (flags & TCP_FIN) {
                tcb.rcv_nxt++;
                tcp_transmit(TCP_ACK, 0, 0);
                tcb.state = TCP_TIME_WAIT;
                printf("TCP CLOSED\n");
            }
            break;
        case TCP_FIN_WAIT_2:
            if (flags & TCP_FIN) {
                tcb.rcv_nxt++;
                tcp_transmit(TCP_ACK, 0, 0);
                tcb.state = TCP_TIME_WAIT;
                printf("TCP Closed\n");
            }
            break;
        case TCP_LAST_ACK:
            if (flags & TCP_ACK) {
                tcb.state = TCP_CLOSED;
                printf("TCP Closed\n");
            }
            break;
    }
}

// IP

static void ip_handle(const uint8_t* frame, uint16_t len) {
    if (len < sizeof(eth_header_t) + sizeof(ip_header_t)) return;
    const ip_header_t* ip = (const ip_header_t*)(frame + sizeof(eth_header_t));
    //printf("NET IP proto=%d dst=%x ours=%x\n", ip->protocol, ntohl(ip->dst_ip), NET_OUR_IP);

    if ((ip->version_ihl >> 4) != 4) return;
    uint32_t header_len = (ip->version_ihl & 0x0F) * 4;
    uint32_t dst = ntohl(ip->dst_ip);
    uint32_t src = ntohl(ip->src_ip);

    if (dst != NET_OUR_IP) return;

    uint16_t total = ntohs(ip->total_length);
    if (total < header_len) return;
    uint16_t payload_len = total - header_len;

    const uint8_t* payload = (const uint8_t*)ip + header_len;

    if (ip->protocol == IP_PROTO_ICMP) {
        if (payload_len < sizeof(icmp_header_t)) return;
        icmp_handle(src, (const icmp_header_t*)payload, payload_len);
    } else if (ip->protocol == IP_PROTO_UDP) {
        udp_handle(src, (const udp_header_t*)payload, payload_len);
    } else if (ip->protocol == IP_PROTO_TCP) {
        tcp_handle(src, (const tcp_header_t*)payload, payload_len);
    }
}



// General polling
void net_ping(uint32_t dst_ip) {
    uint8_t mac[6];
    if (!arp_lookup(dst_ip, mac)) {
        arp_send_request(dst_ip);
        return;
    }
    icmp_send_echo(dst_ip);
}

void net_poll(void) {
    static uint8_t frame[E1000_FRAME_MAX];
    uint16_t len;

    while (e1000_poll_frame(frame, &len)) {
        if (len < sizeof(eth_header_t)) continue;
        eth_header_t* eth = (eth_header_t*)frame;
        uint16_t type = htons(eth->ethertype);

        if (type == ETHERTYPE_ARP) {
            arp_handle(frame, len);
        } else if (type == ETHERTYPE_IP) {
            ip_handle(frame, len);
        }
    }
}