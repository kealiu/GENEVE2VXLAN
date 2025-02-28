#define _GNU_SOURCE // 为了使用 CPU_SET 和 sched_setaffinity

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sched.h>
#include <errno.h>
#include <string.h>

#include "def.h"

extern int geneve_init();
extern void * geneve_worker(void * arg);

extern int vxland_init();
extern void * vxlan_worker(void * arg);

static int vxlan_sockfd;
static int geneve_sockfd;
static uint32_t vxlan_target;

void *thread_function(void *arg) {
    int thread_id = *(int *)arg;
    int cpu_id = sched_getcpu();

    printf("Thread %d running in CPU %d .\n", thread_id, cpu_id);
    if (cpu_id%2 == 0) {
        GeneveThreadArg arg = {
            .target = vxlan_target,
            .sockfd = geneve_sockfd
        };
        geneve_worker((void*)(&arg));
    } else {
        vxlan_worker((void*)(&vxlan_sockfd));
    }

    pthread_exit(NULL);
}

int main() {
    int num_cpus = sysconf(_SC_NPROCESSORS_ONLN); // get CPU num
    if (num_cpus <= 0) {
        perror("sysconf _SC_NPROCESSORS_ONLN failure");
        return 1;
    }
    pthread_t threads[num_cpus];
    int thread_ids[num_cpus];

    geneve_sockfd = geneve_init();
    vxlan_sockfd = vxland_init();
    
    struct in_addr addr;
    inet_pton(AF_INET, "127.0.0.1", &addr);
    vxlan_target = addr.s_addr;

    for (int i = 0; i < num_cpus; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, thread_function, &thread_ids[i]) != 0) {
            perror("pthread_create failure!");
            return 1;
        }

        // binding to CPU
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(i, &cpuset);

        if (pthread_setaffinity_np(threads[i], sizeof(cpu_set_t), &cpuset) != 0) {
            fprintf(stderr, "pthread_setaffinity_np %d to CPU %d failure: %s\n", i, i, strerror(errno));
            return 1;
        }
    }

    // waiting all thread exit
    for (int i = 0; i < num_cpus; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("pthread_join failure");
            return 1;
        }
    }

    printf("all thread exited\n");

    return 0;
}
