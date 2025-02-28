#ifndef __DEF_H
#define __DEF_H

#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <sched.h>

// from kernel include/net/vxlan.h
typedef struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint8_t r2:3;
    uint8_t iflag:1; // must be 1
    uint8_t r1:4;
#else
    uint8_t r1:4;
    uint8_t iflag:1; // must be 1
    uint8_t r2:3;
#endif
    uint8_t rsvd; // reserved 
    /*****  START: special handling for geneve to vxlan */
    uint16_t fwid; // reserved 24bits
    /*****  END: special handling for geneve to vxlan */
    uint8_t vni[3];
    uint8_t r3;
} VxlanHeader;

// from kernel include/net/geneve.h
typedef struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
    uint8_t opt_len:6;
    uint8_t ver:2;
    uint8_t rsvd1:6;
    uint8_t cflag:1;
    uint8_t oflag:1;
#else
    uint8_t ver:2;
    uint8_t opt_len:6;
    
    uint8_t oflag:1;
    uint8_t cflag:1;
    uint8_t rsvd1:6;
#endif
    uint16_t proto_type;
    uint8_t vni[3];
    uint8_t rsvd2;
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

typedef struct {
    uint32_t target;
    int sockfd;
} GeneveThreadArg; 

#define INTERFACE_VXLAN INADDR_ANY
#define PORT_VXLAN 4789

#define INTERFACE_GENEVE INADDR_ANY
#define PORT_GENEVE 6081 

#define AWS_GWLB_HDR_SIZE 40 // (8+2)*4

#define BUFFER_LEN 9000 // enough for jumpo frame

typedef struct {
    uint8_t header[AWS_GWLB_HDR_SIZE];
    uint32_t addr;
} FwTable;

#include <stdatomic.h>
static inline uint32_t atomic_increase_fetch(atomic_int *val) {
    return __atomic_add_fetch(val, 1, __ATOMIC_SEQ_CST);
}

static inline uint32_t atomic_fetch_increase(atomic_int *val) {
    return __atomic_fetch_add(val, 1, __ATOMIC_SEQ_CST);
}

#endif