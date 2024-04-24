#include "list.h"

void push(node** head, pid_t pid) {
    if(*head == NULL) {
        (*head) = (node*)malloc(1 * sizeof(node));
        (*head)->next = NULL;
        (*head)->pid = pid;
        return;
    }
    node* ptr = (*head);
    while(ptr->next != NULL) {
        ptr = ptr->next;
    }
    node* new_node = (node*) malloc(1*sizeof(node));
    ptr->next = new_node;
    new_node->pid = pid;
    new_node->next = NULL;
}

pid_t pop(node** head) {
    pid_t pid;
    if((*head) == NULL)
        return 0;
    node* ptr = *head;
    if(ptr->next == NULL) {
        pid = ptr->pid;
        free(ptr);
        *head = NULL;
        return pid;
    }
    while(ptr->next->next != NULL)
        ptr = ptr->next;
    pid = ptr->next->pid;
    free(ptr->next);
    ptr->next = NULL;
    return pid;
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
        printf("%d\n", ptr->pid);
        ptr = ptr->next;
    }
}
