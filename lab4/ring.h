#include <stdbool.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include "unistd.h"
#include "sys/types.h"
#ifndef LLAB4_RING_H
#define LLAB4_RING_H
#define MAX_SIZE 5
typedef struct message {
    u_int8_t type;
    u_int16_t hash;
    u_int8_t size;
    u_int8_t data[256];
} msg;
typedef struct node_ring {
    struct node_ring *next,
            *prev;
    msg message;
    bool filled;
} node_ring;
typedef struct ring {
    int32_t shmid,
    consumed,
    produced;
    node_ring* head,
    *tail;
} ring;
void add_node(ring** queue);


void construct_ring(ring** queue) {
    int shmid = shm_open("MQUEUE", O_CREAT | O_RDWR, S_IRUSR | S_IWUSR | S_IXUSR);
    ftruncate(shmid, sizeof(ring) + (sizeof(node_ring)* MAX_SIZE));
    (*queue) = mmap (NULL, sizeof(ring) + (sizeof(node_ring)* MAX_SIZE), PROT_WRITE, MAP_SHARED, shmid, 0);
    (*queue)->head = (node_ring*)&((char*)*queue)[sizeof(ring)];
    (*queue)->tail = (*queue)->head;
    (*queue)->head->next=(*queue)->tail;
    (*queue)->head->prev=(*queue)->tail;
    for(int i = 0; i < MAX_SIZE-1; i++) {
        add_node(queue);
    }
    (*queue)->tail = (*queue)->head;
}

void unmap_ring(ring** queue) {
    shm_unlink("MQUEUE");
    munmap((*queue), sizeof(ring) + (sizeof(node_ring)* MAX_SIZE));
}


void add_node(ring** queue) {
    node_ring* curr = (node_ring*)&((char*)(*queue)->tail)[sizeof(node_ring)];
    (*queue)->tail->next = curr;
    curr->next = (*queue)->head;
    curr->prev = (*queue)->tail;
    (*queue)->tail = curr;
}

void insert_msg(ring** queue, msg* message) {
    if((*queue)->tail->filled) {
        printf("No space\n");
    }
    (*queue)->tail->message = *message;
    (*queue)->tail->filled = true;
    (*queue)->tail = (*queue)->tail->next;
    (*queue)->produced++;
}

msg get_msg(ring** queue) {
    if(!(*queue)->head->filled) {
        printf("No smessages to get\n");
    }
    msg message = (*queue)->head->message;
    (*queue)->head->filled = false;
    (*queue)->head = (*queue)->head->next;
    (*queue)->consumed++;
    return message;
}

msg generate_msg(char type) {
    msg new_msg;
    new_msg.type = type;
    new_msg.hash = 0;
    void* msg;
    int size_int = 0;
    while(size_int == 0) {
        size_int = rand() % 257;
    }
    new_msg.size = size_int;
    if(new_msg.size == 0)
        size_int = 256;
    else
        size_int = ((new_msg.size + 3)*4)/4;
    int k;
    for(k = 0; k < size_int; k++) {
        new_msg.data[k] = rand() % 42 + 48;
    }
    for(; k < 256; k++) {
        new_msg.data[k] = '\0';
    }
    for(int i = 0; i < size_int; i++) {
        new_msg.hash += new_msg.data[i];
    }
    new_msg.hash /= (size_int | new_msg.type);

    return new_msg;
}

int count_busy(ring** queue) {
    int counter = 0;
    node_ring* ptr = (*queue)->head;
    while(ptr != (*queue)->tail && ptr->filled) {
        counter++;
        ptr = ptr->next;
    }
    return counter;
}

#endif //LLAB4_RING_H
