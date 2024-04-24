#include <stdio.h>
#include "unistd.h"
#include "ring.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <threads.h>
#include <pthread.h>
#include "list.h"

sem_t* added;
sem_t* empty;
pthread_mutex_t mutex;
ring* mqueue;
int prod_skip = 0;
int cons_skip = 0;
thread_local bool main_flag = true;

void *producer();
void *consumer();
bool check_hash(msg message);
void stop_thread() {
    main_flag = false;
}
void kill_all(node**, node**);


int main() {

    char choice;
    node* list_producers = NULL;
    node* list_consumers = NULL;

    sem_unlink("sem_added");
    sem_unlink("sem_empty");

    pthread_mutex_init(&mutex, NULL);
    added = sem_open("sem_added", O_CREAT, 0777, 0);
    empty = sem_open("sem_empty", O_CREAT, 0777, current_size);


    construct_ring(&mqueue);
    do {
        choice = getchar();
        switch (choice) {
            case 'p': {
                pthread_t new_producer;
                pthread_create(&new_producer, NULL, producer, NULL);
                printf("New producer with thread id %lu created\n", new_producer);
                push(&list_producers, new_producer);
                break;
            }
            case 'c': {
                pthread_t new_consumer;
                pthread_create(&new_consumer, NULL, consumer, NULL);
                printf("New consumer with thread id %lu created\n", new_consumer);
                push(&list_consumers, new_consumer);
                break;
            }
            case 'l': {
                printf("List of producers:\n");
                show_list(&list_producers);
                printf("List of consumers\n");
                show_list(&list_consumers);
                printf("\n");
                break;
            }
            case '+': {
                pthread_mutex_lock(&mutex);
                current_size++;
                printf("Added space in queue, current size: %d\n", current_size);
                add_node(&mqueue);
                sem_post(empty);
                pthread_mutex_unlock(&mutex);
                break;
            }
            case '-': {
                pthread_mutex_lock(&mutex);
                printf("New queue size: %d\n", --current_size);
                bool flag = delete_node(&mqueue);
                if (flag) {
                    cons_skip++;
                } else {
                    prod_skip++;
                }
                pthread_mutex_unlock(&mutex);
                break;
            }
            case 'k': {
                choice = getchar();
                switch (choice) {
                    case 'a':
                        kill_all(&list_consumers, &list_producers);
                        break;
                    case 'c': {
                        pthread_t thread = pop(&list_consumers);
                        if(thread == 0) {
                            printf("There is no consumers\n");
                            break;
                        }
                        printf("Terminating consumer process %lu\n", thread);
                        pthread_kill(thread, SIGUSR1);
                        break;
                    }
                    case 'p': {
                        pthread_t thread = pop(&list_producers);
                        if(thread == 0) {
                            printf("There is no producers\n");
                            break;
                        }
                        printf("Terminating producer process %lu\n", thread);
                        pthread_kill(thread, SIGUSR1);
                        break;
                    }
                    default:
                        break;
                }
                break;
            }
            case 'q':
                main_flag = false;
                break;
            case 'i': {
                pthread_mutex_lock(&mutex);
                printf("Ring size: %d\nMessages in queue: %d\nMessages consumed: %d\nMessages produced: %d\n", current_size, count_busy(&mqueue), mqueue->consumed, mqueue->produced);
                pthread_mutex_unlock(&mutex);
                break;
            }
            default:
                printf("Incorrect input\n");
                break;
        }
        getchar();
    } while (main_flag);

    clear_list(&list_producers);
    clear_list(&list_consumers);

    sem_unlink("sem_mutex");
    sem_unlink("sem_added");
    sem_unlink("sem_empty");
    pthread_mutex_destroy(&mutex);
    return 0;
}

void * producer() {
    signal(SIGUSR1, stop_thread);
    do {
        sem_wait(empty);
        pthread_mutex_lock(&mutex);
        if (prod_skip != 0) {
            prod_skip--;
            pthread_mutex_unlock(&mutex);
            continue;
        }
        msg new_message = generate_msg('A');
        insert_msg(&mqueue, &new_message);
        pthread_mutex_unlock(&mutex);
        sem_post(added);
        printf("\nAdded new message from producer %lu\n"\
        "Total messages produced %d\nMessage hash: %d\n\n", thrd_current(), mqueue->produced, new_message.hash);
        sleep(2);
    } while (main_flag);
}

bool check_hash(msg message) {
    int size_int;
    u_int16_t _hash = 0;
    if(message.size == 0) {
        size_int = 256;
    } else
        size_int = ((message.size + 3)*4)/4;
    for(int i = 0; i < size_int; i++) {
        _hash += message.data[i];
    }
    _hash /= (size_int | message.type);
    if(_hash == message.hash) {
        return true;
    } else
        return false;
}

void * consumer() {
    do {
        sem_wait(added);
        pthread_mutex_lock(&mutex);
        if (cons_skip != 0) {
            cons_skip--;
            pthread_mutex_unlock(&mutex);
            continue;
        }
        msg message = get_msg(&mqueue);
        pthread_mutex_unlock(&mutex);
        sem_post(empty);
        if(check_hash(message))
            printf("\nConsumed new message by consumer %lu\nTotal messages consumed %d\nMessage: %s\n\n", thrd_current(), mqueue->consumed, message.data);
        else
            printf("Error occurred consuming message\nTID: %lu\nMessage with hash: %d\n", thrd_current(), message.hash);
        sleep(2);
    } while (main_flag);
}

void kill_all(node** cons, node** prod) {
    pthread_t pthread;
    while((pthread = pop(cons)) != 0) {
        pthread_kill(pthread, SIGUSR1);
    }
    while((pthread = pop(prod)) != 0) {
        pthread_kill(pthread, SIGUSR1);
    }
    printf("All processes terminated\n");
}
