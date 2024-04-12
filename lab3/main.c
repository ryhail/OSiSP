#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdbool.h>
#include "list.h"

#define TIMER 100
typedef struct pair{
    int first;
    int second;
} pair;

node* list = NULL;
pair stat_array[4];
pair cur_pair;
bool taking_stats = true;
bool stat_allowed = false;
size_t current_stat = 0;
bool main_flag = true;
bool alarm_set = false;

int child_counter = 0;
void create_proc();
void kill_last_child();
void child_main();
void shoot_stat();
void show_stat();
void kill_all();
void forbid_stat();
void allow_stat();
void forbid_showing_stat_all(int choice);
void allow_showing_stat_all(int choice);
void get_stat_of_process(int num);

int main() {
    int choice;
    signal(SIGUSR1, allow_stat);
    signal(SIGUSR2, forbid_stat);
    do {
        choice = getchar();
        switch(choice) {
            case '+':
                create_proc();
                break;
            case '-':
                kill_last_child();
                break;
            case 'l':
                printf("All processes:\n");
                show_list(&list);
                break;
            case 'k':
                kill_all();
                break;
            case 's':
                printf("Input number of process or 0 to forbid all\n");
                while(!(scanf("%d", &choice) && choice <= child_counter && choice >= 0)) {
                    printf("Invalid input\n");
                    rewind(stdin);
                }
                forbid_showing_stat_all(choice);
                break;
            case 'g':
                if(alarm_set) {
                    alarm(0);
                    alarm_set = false;
                }
                printf("Input number of process or 0 to allow all\n");
                while(!(scanf("%d", &choice) && choice <= child_counter && choice >= 0)) {
                    printf("Invalid input\n");
                    rewind(stdin);
                }
                allow_showing_stat_all(choice);
                break;
            case 'p':
                scanf("%d", &choice);
                if(!(choice <= child_counter && choice >= 0)) {
                    printf("Invalid input\n");
                    fflush(stdin);
                    break;
                }
                get_stat_of_process(choice);
                break;
            case 'q':
                main_flag = false;
                kill_all();
                clear_list(&list);
                break;
            default:
                break;
        }
        getchar();
    } while(main_flag);
    return 0;
}

void get_stat_of_process(int num) {
    int buf = num-1;
    node* ptr = list;
    while(buf != 0) {
        ptr = ptr->next;
        buf--;
    }
    forbid_showing_stat_all(0);
    allow_showing_stat_all(num);
    signal(SIGALRM, allow_showing_stat_all);
    alarm(5);
    alarm_set = true;
}

void forbid_showing_stat_all(int choice) {
    node* ptr = list;
    int i = choice - 1;
    if(choice == 0) {
        while (ptr != NULL) {
            kill(ptr->pid, SIGUSR2);
            ptr = ptr->next;
            i--;
        }
        printf("Stats forbidden for all\n");
    }
    else {
        while(i != 0) {
            ptr = ptr->next;
            i--;
        }
        kill(ptr->pid, SIGUSR2);
        printf("Stats forbidden for %d process\n", choice);
    }

}

void allow_showing_stat_all(int choice) {
    node* ptr = list;
    if(alarm_set) {
        choice = 0;
        alarm_set = false;
    }
    int i = choice - 1;
    if(choice == 0) {
        while (ptr != NULL) {
            kill(ptr->pid, SIGUSR1);
            ptr = ptr->next;
            i--;
        }
        printf("Stats allowed for all\n");
    }
    else {
        while(i != 0) {
            ptr = ptr->next;
            i--;
        }
        kill(ptr->pid, SIGUSR1);
        printf("Stats allowed for %d process\n", choice);
    }
}

void forbid_stat() {
    stat_allowed = false;
}

void allow_stat() {
    stat_allowed = true;
}
void create_proc() {
    pid_t pid;
    pid = fork();
    switch(pid) {
        case -1:
            perror("fork");
            exit(1);
        case 0:
            signal(SIGINT, show_stat);
            signal(SIGALRM, shoot_stat);
            child_main();
            break;
        default:
            printf("Child created successfully PID: %d\n", pid);
            push(&list, pid);
            child_counter++;
            break;
    }
}

void kill_last_child() {
    pid_t pid_to_kill = pop(&list);
    if(pid_to_kill == 0) {
        printf("No children to kill(((\n");
        return;
    }
    kill(pid_to_kill, SIGKILL);
    printf("Child with PID %d killed successfully!\n", pid_to_kill);
    printf("Children remaining: %d\n", --child_counter);
}

void child_main() {
    do {
        for(size_t i = 0; i < 101; i++) {
            if(current_stat == 4)
                current_stat = 0;
            ualarm(TIMER, 0);
            while (taking_stats) {
                cur_pair.first = 0;
                cur_pair.second = 0;
                cur_pair.first = 1;
                cur_pair.second = 1;
            }
            taking_stats = true;
        }
        if(stat_allowed)
            show_stat();
        sleep(5);
    } while(1);
}

void shoot_stat() {
    stat_array[current_stat++] = cur_pair;
    taking_stats = false;
}

void show_stat() {
    printf("Statistic for child %d:\n", getpid());
    for(int i = 0; i < 4; i++) {
        printf("{%d, %d} ", stat_array[i].first, stat_array[i].second);
    }
    printf("\n");
}

void kill_all() {
    while(list != NULL) {
        kill(pop(&list), SIGKILL);
    }
    printf("All processes were terminated\n");
}
