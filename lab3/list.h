#include <stdio.h>
#include <stdlib.h>

typedef struct node {
    pid_t pid;
    struct node* next;
} node;

void push(node**, pid_t);
pid_t pop(node**);
void show_list(node**);
void clear_list(node**);
