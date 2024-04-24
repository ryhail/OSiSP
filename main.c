#include <stdio.h>
#include "unistd.h"
#include "ring.h"
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include "list.h"

sem_t* mutex;
sem_t* added;
sem_t* empty;
bool main_flag = true;

void producer(ring** queue);
void consumer(ring** queue);
bool check_hash(msg message);
void stop_proc() {
    main_flag = false;
}
void kill_all(node**, node**);

int main() {
    signal(SIGUSR1, stop_proc);

    char choice;
    node* list_producers = NULL;
    node* list_consumers = NULL;

    shm_unlink("MQUEUE");
    sem_unlink("sem_mutex");
    sem_unlink("sem_added");
    sem_unlink("sem_empty");

    mutex = sem_open("sem_mutex",O_CREAT, 0777, 1);
    added = sem_open("sem_added", O_CREAT, 0777, 0);
    empty = sem_open("sem_empty", O_CREAT, 0777, MAX_SIZE);

    ring* mqueue;
    construct_ring(&mqueue);
    printf("Current shmid: %d\n", mqueue->shmid);
    do {
        choice = getchar();
        switch (choice) {
            case 'p': {
                pid_t pid = fork();
                if (pid == 0) {
                    producer(&mqueue);
                } else {
                    printf("New producer with PID %d created\n", pid);
                    push(&list_producers, pid);
                }
                break;
            }
            case 'c': {
                pid_t pid = fork();
                if(pid == 0) {
                    consumer(&mqueue);
                } else {
                    printf("New consumer with PID %d created\n", pid);
                    push(&list_consumers, pid);
                }
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
            case 'k': {
                choice = getchar();
                switch (choice) {
                    case 'a':
                        kill_all(&list_consumers, &list_producers);
                        break;
                    case 'c': {
                        pid_t pid = pop(&list_consumers);
                        if(pid == 0) {
                            printf("There is no consumers\n");
                            break;
                        }
                        printf("Terminating consumer process %d\n", pid);
                        kill(pid, SIGUSR1);
                        break;
                    }
                    case 'p': {
                        pid_t pid = pop(&list_producers);
                        if(pid == 0) {
                            printf("There is no producers\n");
                            break;
                        }
                        printf("Terminating producer process %d\n", pid);
                        kill(pid, SIGUSR1);
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
                sem_wait(mutex);
                printf("Ring size: %d\nMessages in queue: %d\n", MAX_SIZE, count_busy(&mqueue));
                sem_post(mutex);
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
    unmap_ring(&mqueue);

    sem_unlink("sem_mutex");
    sem_unlink("sem_added");
    sem_unlink("sem_empty");
    return 0;
}

void producer(ring** queue) {
    do {
        sem_wait(empty);
        sem_wait(mutex);
        sleep(2);
        msg new_message = generate_msg('A');
        insert_msg(queue, &new_message);
        sem_post(mutex);
        sem_post(added);
        printf("\nAdded new message from producer %d\n"\
        "Total messages produced %d\nMessage hash: %d\n\n", getpid(), (*queue)->produced, new_message.hash);
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

void consumer(ring** queue) {
    do {
        sem_wait(added);
        sem_wait(mutex);
        sleep(2);
        msg message = get_msg(queue);
        sem_post(mutex);
        sem_post(empty);
        if(check_hash(message))
            printf("\nConsumed new message by consumer %d\nTotal messages consumed %d\nMessage: %s\n\n", getpid(), (*queue)->consumed, message.data);
        else
            printf("Error occurred consuming message\nPID: %d\nMessage with hash: %d\n", getpid(), message.hash);
    } while (main_flag);
}

void kill_all(node** cons, node** prod) {
    pid_t pid;
    while((pid = pop(cons)) != 0) {
        kill(pid, SIGUSR1);
    }
    while((pid = pop(prod)) != 0) {
        kill(pid, SIGUSR1);
    }
    printf("All processes terminated\n");
}