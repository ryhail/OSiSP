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
int current_size = 5;

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
    int
    consumed,
    produced;
    node_ring* head,
    *tail;
} ring;
void add_node(ring** queue);


void construct_ring(ring** queue) {
    (*queue) = (ring*)calloc(1, sizeof(ring));
    for(int i = 0; i < current_size; i++) {
        add_node(queue);
    }
    (*queue)->tail = (*queue)->head;
}

void add_node(ring** queue) {
    node_ring *new_node = (node_ring *) calloc(1, sizeof(node_ring));
    if ((*queue)->head == NULL ||
        (*queue)->tail == NULL) {
        (*queue)->head = new_node;
        new_node->next = new_node;
        new_node->prev = new_node;
        (*queue)->tail = new_node;
        return;
    }
    if((*queue)->head->next == (*queue)->head) {
        (*queue)->head->next = new_node;
        (*queue)->head->prev = new_node;
        new_node->prev = (*queue)->head;
        new_node->next = (*queue)->head;
        return;
    }
    node_ring *ptr = (*queue)->head->prev;
    new_node->prev = ptr;
    new_node->next = (*queue)->head;
    ptr->next = new_node;
    (*queue)->head->prev = new_node;
}

bool delete_node(ring** queue) {
    if(*queue == NULL) {
        printf("Ring has no nodes\n");
        exit(1);
    }
    if((*queue)->head == NULL) {
        printf("Ring has no nodes\n");
        exit(1);
    }
    bool flag;
    if ((*queue)->tail == (*queue)->head) {
        if ((*queue)->head == (*queue)->head->next) {
            flag = (*queue)->head->filled;
            free((*queue)->head);
            free(*queue);
            *queue = NULL;
        }else {
            node_ring* buffer = (*queue)->head;
            (*queue)->head->next->prev = (*queue)->head->prev;
            (*queue)->head->prev->next = (*queue)->head->next;
            (*queue)->head = (*queue)->tail = (*queue)->head->next;
            flag = buffer->filled;
            free(buffer);
        }
        return flag;
    }
    if ((*queue)->head->next == (*queue)->head->prev) {
        (*queue)->tail = (*queue)->tail->prev;
        flag = (*queue)->tail->next->filled;
        free((*queue)->tail->next);
        (*queue)->tail->next = (*queue)->tail->prev = (*queue)->tail;
        return flag;
    }
    node_ring* buffer = (*queue)->tail;
    (*queue)->tail->next->prev = (*queue)->tail->prev;
    (*queue)->tail->prev->next = (*queue)->tail->next;
    (*queue)->tail = (*queue)->tail->next;
    flag = buffer->filled;
    free(buffer);
    return flag;
}

void insert_msg(ring** queue, msg* message) {
    (*queue)->tail->message = *message;
    (*queue)->tail->filled = true;
    (*queue)->tail = (*queue)->tail->next;
    (*queue)->produced++;
}

msg get_msg(ring** queue) {
    if(!(*queue)->head->filled) {
        printf("No messages to get\n");
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
