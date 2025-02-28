#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <pthread.h>

#include "def.h"

FwTable gFwTable[0xFF]; // 65535*40
atomic_int gFwIndex = 0; // current index of gHashTable

static uint16_t push_geneve_hdr(GeneveHeader *header, uint32_t target_addr) {
    uint16_t current = atomic_fetch_increase(&gFwIndex) && 0xFF; // booking one buffer
    memcpy(gFwTable[current].header, (uint8_t *)header, AWS_GWLB_HDR_SIZE);
    gFwTable[current].addr = target_addr;
    return current;
}

static int process_geneve_packet(char *packet, int packet_len, uint32_t target_addr, uint8_t **retptr) {
    if (packet_len < sizeof(GeneveHeader)) {
        printf("Geneve Invalid packet: size smaller than 8 bytes\n");
        return 0;
    }

    GeneveHeader *header = (GeneveHeader *)packet;

    int total_header_len = sizeof(GeneveHeader) + header->opt_len*4;

    VxlanHeader *vxlan = (VxlanHeader *)(packet + total_header_len - sizeof(VxlanHeader));

    uint16_t fwid = push_geneve_hdr(header, target_addr);

    memcpy(vxlan->vni, header->vni, 3); // copy vni, 3 bytes
    vxlan->fwid = fwid;
    
    *retptr = (uint8_t * )vxlan ;
    return (packet_len - (total_header_len-sizeof(VxlanHeader)));
}

void * geneve_worker(void * arg) {
    // init vxlan target
    GeneveThreadArg * gta = (GeneveThreadArg *)arg;

    int sockfd = gta->sockfd;
    struct sockaddr_in vxlan_addr;
    struct sockaddr_in client_addr;
    int vxlan_add_len = sizeof(vxlan_addr);
    int client_len = sizeof(client_addr);
    uint8_t packet[BUFFER_LEN]; // enough for jumpo frame

    memset(&vxlan_addr, 0, sizeof(vxlan_addr));
    vxlan_addr.sin_family = AF_INET;
    vxlan_addr.sin_addr.s_addr = gta->target;
    vxlan_addr.sin_port = htons(PORT_VXLAN);

    int recv_len;
    uint8_t * vxlan_packet = NULL;
    int vxlan_packet_len = 0;
    while (1) {
        
        // recv packets
        recv_len = recvfrom(sockfd, packet, BUFFER_LEN, 0,
                            (struct sockaddr *)&client_addr, &client_len);
        if (recv_len < 0) {
            perror("Receive Geneve packet failed");
            continue;
        }
        
        vxlan_packet_len = process_geneve_packet((char*)packet, recv_len, client_addr.sin_addr.s_addr, &vxlan_packet);
        if (vxlan_packet_len <= 8) {
            perror("Process Geneve packet failed");
            continue;
        }

        if (sendto(sockfd, vxlan_packet, vxlan_packet_len, 0, (struct sockaddr *)&vxlan_addr, vxlan_add_len) < 0) {
            perror("Send VXLAN failed");
        }
    }

    return NULL;
}

// the binding socket
static int geneve_binding() {
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Geneve Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INTERFACE_GENEVE;
    server_addr.sin_port = htons(PORT_GENEVE);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Geneve Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    return sockfd;
}

int geneve_init() {
    return geneve_binding();
}
