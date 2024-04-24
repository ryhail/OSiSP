#include <stdio.h>
#include <stdlib.h>

typedef struct node {
   pthread_t thread;
    struct node* next;
} node;

void push(node**, pthread_t);
pthread_t pop(node **head);
void show_list(node**);
void clear_list(node**);
