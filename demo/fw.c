#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/udp.h>

#define GENEVE_HEADER_BASE_SIZE 8 // Geneve base size is  8 bytes

// from kernel include/net/geneve.h
typedef struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    // first 32 bits
    uint8_t opt_len:6;
    uint8_t ver:2;
    uint8_t rsvd1:6;
    uint8_t cflag:1;
    uint8_t oflag:1;
#else
    // 2nd 32 bits
    uint8_t ver:2;
    uint8_t opt_len:6;
    
    uint8_t oflag:1;
    uint8_t cflag:1;
    uint8_t rsvd1:6;
#endif
    // 2nd 32 bits
    uint16_t proto_type;
    uint8_t rsvd2;
    uint8_t vni[3];
} GeneveHeader;

typedef struct {
    uint16_t class;
    
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint8_t type:7;
    uint8_t cflag:1;
    
    uint8_t len:5;
    uint8_t rflag:3;
#else
    uint8_t cflag:1;
    uint8_t type:7;
    
    uint8_t rflag:3;
    uint8_t len:5;
#endif
} GeneveOpts;

void process_geneve_packet(const char *packet, int packet_len) {
    if (packet_len < sizeof(GeneveHeader)) {
        printf("Invalid packet: size smaller than 8 bytes\n");
        return;
    }

    GeneveHeader *header = (GeneveHeader *)packet;

    int total_header_len = sizeof(GeneveHeader) + header->opt_len*4;

    if (packet_len < total_header_len) {
        printf("Invalid packet: not enough len\n");
        return;
    }

    printf("Geneve Header：\n");
    printf("  Ver: %u\n", header->ver);
    printf("  Opt Len: %u\n", header->opt_len);
    printf("  OAM: %u\n", header->oflag);
    printf("  Critical: %u\n", header->cflag);
    printf("  Proto Type: 0x%02X\n", ntohs(header->proto_type));
    printf("  VNI (Virtual Network Identifier): 0x%08x%08x%08x\n", header->vni[0], header->vni[1], header->vni[2]);

    // process options
    if (header->opt_len > 0) {
        int total_bytes = header->opt_len * 4; //optlen is 4-bytes multi
        const uint8_t *options = (const uint8_t *)(packet + sizeof(GeneveHeader));
        int pos = 0;

        while (pos < total_bytes) {
            GeneveOpts * opt = (GeneveOpts *) (options+pos);

            printf("  OPTIONS CLASS: 0x%x\n", ntohs(opt->class));
            printf("    OPTIONS cflag: %u\n", opt->cflag);
            printf("    OPTIONS type: 0x%x\n", opt->type);
            printf("    OPTIONS len: %u\n", opt->len);
            pos += sizeof(GeneveOpts); // forwarding

            if (opt->len > 0) {
                uint8_t * val = (uint8_t *)(options+pos);
                printf("    OPTIONS data:");
                for (int i=0; i<opt->len*4; i+=4){
                    printf(" 0x%08X", *((unsigned int*)(val+i)));
                }
                printf("\n");
                pos += opt->len*4;
            }
        }
    }
    printf("\n");
}

static struct sockaddr_in server_addr;

// create UDP socket listener
int create_udp_socket(uint16_t port) {
    int sockfd;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Bind failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    return sockfd;
}

int main(int argc, char * argv[]) {
    int sockfd;
    uint8_t geneve_packet[1500];
    struct sockaddr_in target_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    ssize_t recv_len;
    struct in_addr clientIpAddr;


    // listening udp 6081
    sockfd = create_udp_socket(6081);

    while (1) {
        // recv packets
        recv_len = recvfrom(sockfd, geneve_packet, sizeof(geneve_packet), 0,
                            (struct sockaddr *)&client_addr, &client_len);
        if (recv_len < 0) {
            perror("Receive failed");
            continue;
        }
        
        process_geneve_packet((char*)geneve_packet, recv_len);

        // must reply to UDP 6081 port
        client_addr.sin_port = htons(6081);

        if (sendto(sockfd, geneve_packet, recv_len, 0, (struct sockaddr *)&client_addr, client_len) < 0) {
            perror("Send failed");
        }
    }

    close(sockfd);
    return 0;
}
