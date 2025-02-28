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

extern FwTable gFwTable[]; // 65535*40

static int process_vxlan_packet(char *packet, int packet_len, uint8_t **retptr, uint32_t* target) {
    if (packet_len < sizeof(VxlanHeader)) {
        printf("VXLAN Invalid packet: size smaller than 8 bytes\n");
        return 0;
    }

    VxlanHeader *vxlan = (VxlanHeader *)packet;

    GeneveHeader *geneve = (GeneveHeader*)(packet + sizeof(VxlanHeader)  - AWS_GWLB_HDR_SIZE);

    uint16_t fwid = vxlan->fwid;
    memcpy(geneve, gFwTable[fwid].header, AWS_GWLB_HDR_SIZE);
    //memcpy(geneve->vni, vxlan->vni, 3); // copy vni, 3 bytes
    
    *retptr = (uint8_t * )geneve;
    *target = gFwTable[fwid].addr;
    return (packet_len + AWS_GWLB_HDR_SIZE - sizeof(VxlanHeader));
}

void * vxlan_worker(void * arg) {
    int sockfd = *((int *)arg);
    struct sockaddr_in geneve_addr;
    struct sockaddr_in client_addr;
    int geneve_add_len = sizeof(geneve_addr);
    int client_len = sizeof(client_addr);
    uint8_t packetbuf[BUFFER_LEN]; // enough for jumpo frame


    memset(&geneve_addr, 0, sizeof(geneve_addr));
    geneve_addr.sin_family = AF_INET;
    geneve_addr.sin_port = htons(PORT_GENEVE);

    int recv_len = 0;
    uint8_t * geneve_packet = NULL;
    int geneve_packet_len = 0;
    uint8_t * packet = &(packetbuf[128]);
    while (1) {    
        // recv packets, with 128 bytes offset
        recv_len = recvfrom(sockfd, packet, BUFFER_LEN, 0,
                            (struct sockaddr *)&client_addr, &client_len);
        if (recv_len < 0) {
            perror("Receive VXLAN packet failed");
            continue;
        }
        

        geneve_packet_len = process_vxlan_packet((char*)packet, recv_len, &geneve_packet, &(geneve_addr.sin_addr.s_addr));
        if (geneve_packet_len <= 8) {
            perror("Process VXLAN packet failed");
            continue;
        }

        geneve_addr.sin_family = AF_INET;
        geneve_addr.sin_port = htons(PORT_GENEVE);
        if (sendto(sockfd, geneve_packet, geneve_packet_len, 0, (struct sockaddr *)&geneve_addr, geneve_add_len) < 0) {
            perror("Send to Geneve failed");
        }
    }

    return NULL;
}

// the binding socket
static int vxland_binding() {
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("VXLAN Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INTERFACE_VXLAN;
    server_addr.sin_port = htons(PORT_VXLAN);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("VXLAN Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    return sockfd;
}

int vxland_init() {
    return vxland_binding();
}
