#include "list.h"

void push(node** head, pthread_t thread) {
    if(*head == NULL) {
        (*head) = (node*)malloc(1 * sizeof(node));
        (*head)->next = NULL;
        (*head)->thread = thread;
        return;
    }
    node* ptr = (*head);
    while(ptr->next != NULL) {
        ptr = ptr->next;
    }
    node* new_node = (node*) malloc(1*sizeof(node));
    ptr->next = new_node;
    new_node->thread = thread;
    new_node->next = NULL;
}

pthread_t pop(node** head) {
    pthread_t thread;
    if((*head) == NULL)
        return 0;
    node* ptr = *head;
    if(ptr->next == NULL) {
        thread = ptr->thread;
        free(ptr);
        *head = NULL;
        return thread;
    }
    while(ptr->next->next != NULL)
        ptr = ptr->next;
    thread = ptr->next->thread;
    free(ptr->next);
    ptr->next = NULL;
    return thread;
}

void clear_list(node** head) {
    node* ptr = *head;
    while(ptr != NULL) {
        node* buf = ptr;
        ptr = ptr->next;
        free(buf);
    }
    *head = NULL;
}

void show_list(node** head) {
    node* ptr = *head;
    if(ptr == NULL) {
        printf("Empty\n");
    }
    while(ptr != NULL) {
        printf("%lu\n", ptr->thread);
        ptr = ptr->next;
    }
}
