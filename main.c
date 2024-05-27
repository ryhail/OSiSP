#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>

#define STRING_MAX_LEN 80
#define NUM_OF_RECS 10
#define FILE_NAME "data.bin"
typedef struct record_s {
    char name[STRING_MAX_LEN];
    char address[STRING_MAX_LEN];
    u_int8_t semester;
}record_t;

int descriptor = 0;
bool FLAG_EDIT = false;

bool is_equal(const record_t* first_rec, const record_t* second_rec);
bool record_copy(record_t* dest, const record_t* source);
void all_records();
bool get_record(size_t num_rec, record_t* record);
void modify(size_t num_rec, record_t* record, record_t* record_backup);
bool put_record(record_t* record_to_save, const record_t* base_record, const record_t* record_backup, size_t num_rec);

int main(int argc, char* argv[])
{
    bool flag_continue = true;
    record_t REC;
    record_t REC_SAVE;
    record_t REC_NEW;
    size_t NUM_REC = 0;

    descriptor = open(FILE_NAME, O_RDWR);

    if (descriptor == -1) {
        printf("ERROR: Wrong file name.\n");
        exit(0);
    }

    do {
        printf("1.Display all records.\n");
        printf("2.Modify record.\n");
        printf("3.Get num_record.\n");
        printf("4.Put last modified record.\n");
        char ch = getchar();
        switch(ch) {
            case '1': {
                all_records();
                break;
            }
            case '2' : {
                printf("Enter the num of record: ");
                scanf("%lu", &NUM_REC);
                modify(NUM_REC, &REC, &REC_SAVE);
                break;
            }
            case '3' : {
                record_t record;
                size_t num;
                printf("Enter the num of record: ");
                scanf("%lu", &num);
                get_record(num, &record);
                printf("%lu. Name: %s, Address: %s, Num of semester: %hhu\n", num, record.name, record.address, record.semester);
                break;
            }
            case '4' : {
                if (!FLAG_EDIT) {
                    printf("No entry has been changed!\n");
                }else {
                    bool flag_put = put_record(&REC_NEW, &REC, &REC_SAVE, NUM_REC);
                    if (flag_put == false) {
                        printf("The data has been changed by someone, please repeat the editing operation again.\n");
                        modify(NUM_REC, &REC, &REC_SAVE);
                    }
                }
                break;
            }
            default : { flag_continue = false; break; }
        }
        getchar();
    }while(flag_continue);

    int status_close = close(descriptor);
    if (status_close == -1) {
        printf("Unable to close file.\n");
        exit(0);
    }

    close(descriptor);

    return 0;
}

bool is_equal(const record_t* first_rec, const record_t* second_rec) {
    if (first_rec == NULL || second_rec == NULL)
        exit(-100);
    if (strcmp(first_rec->name, second_rec->name) == 0 &&
        strcmp(first_rec->address, second_rec->address) == 0 &&
        first_rec->semester == second_rec->semester)
        return true;
    return false;
}

bool record_copy(record_t* dest, const record_t* source) {
    if (dest == NULL || source == NULL)
        exit(-1);
    strncpy(dest->name, source->name, STRING_MAX_LEN);
    strncpy(dest->address, source->address, STRING_MAX_LEN);
    dest->semester = source->semester;
    return true;
}


void all_records() {
    struct flock parameters;
    parameters.l_type = F_RDLCK;
    parameters.l_whence = SEEK_SET;
    parameters.l_start = 0;
    parameters.l_len = 0;
    if(fcntl(descriptor, F_SETLKW, &parameters) <0)
        perror("F_SETLKW");
    record_t buffer;
    for (size_t i = 0; i < NUM_OF_RECS; ++i) {
        read(descriptor, &buffer, sizeof(record_t));
        printf("%lu. Name: %s, Address: %s, Num of semester: %hhu\n", i+1, buffer.name, buffer.address, buffer.semester);
    }
    sleep(5);
    parameters.l_type = F_UNLCK;
    if(fcntl(descriptor, F_SETLKW, &parameters) < 0)
        perror("F_SETLKW");
    lseek(descriptor, 0, SEEK_SET);
}

bool get_record(size_t num_rec, record_t* record) {
    if (num_rec > NUM_OF_RECS || num_rec == 0) {
        return false;
    }
    struct flock parameters;
    parameters.l_type = F_RDLCK;
    parameters.l_whence = SEEK_SET;
    parameters.l_start = (num_rec - 1) * sizeof(record_t);
    parameters.l_len = sizeof(record_t);
    if(fcntl(descriptor, F_SETLKW, &parameters) <0)
        perror("F_SETLKW");
    lseek(descriptor, (num_rec - 1) * sizeof(record_t), SEEK_SET);
    read(descriptor, record, sizeof(record_t));
    //sleep(10);
    parameters.l_type = F_UNLCK;
    if(fcntl(descriptor, F_SETLKW, &parameters) < 0)
        perror("F_SETLKW");
    lseek(descriptor, 0, SEEK_SET);
    return true;
}

void modify(size_t num_rec, record_t* record, record_t* record_backup) {
    bool flag_exist = get_record(num_rec, record);
    if (!flag_exist)
        return;
    record_copy(record_backup, record);
    bool flag_continue = true;
    getchar();
    do {
        printf("1.Edit name.\n");
        printf("2.Edit address.\n");
        printf("3.Edit num of semester.\n");
        printf("4.Display edit record.\n");
        printf("5.Exit.\n");
        char ch = getchar();
        char BUFFER[STRING_MAX_LEN];
        switch(ch) {
            case '1': {
                FLAG_EDIT = true;
                printf("Enter the name: ");
                scanf("%s", BUFFER);
                strncpy(record->name, BUFFER, STRING_MAX_LEN);
                fflush(stdin);
                break;
            }
            case '2': {
                FLAG_EDIT = true;
                printf("Enter the address: ");
                scanf("%s", BUFFER);
                strncpy(record->address, BUFFER, STRING_MAX_LEN);
                fflush(stdin);
                break;
            }
            case '3': {
                FLAG_EDIT = true;
                u_int8_t sem;
                printf("Enter the num of semester: ");
                scanf("%hhu", &sem);
                record->semester = sem;
                break;
            }
            case '4' : {
                printf("%lu. Name: %s, Address: %s, Num of semester: %hhu\n", num_rec, record->name, record->address, record->semester);
                break;
            }
            case '5' : {
                flag_continue = false;
                break;
            }
            default: {
                break;
            }
        }
        getchar();
    }while(flag_continue);
    if (is_equal(record, record_backup)) {
        FLAG_EDIT = false;
    }
}

bool put_record(record_t* record_to_save, const record_t* base_record, const record_t* record_backup, size_t num_rec) {
    struct flock parameters;
    parameters.l_type = F_WRLCK;
    parameters.l_whence = SEEK_SET;
    parameters.l_start = (num_rec - 1) * sizeof(record_t);
    parameters.l_len = sizeof(record_t);
    if(fcntl(descriptor, F_SETLKW, &parameters) < 0)
        perror("F_SETLKW");
    parameters.l_type = F_UNLCK;
    get_record(num_rec, record_to_save);
    if (!is_equal(record_backup, record_to_save)) {
        fcntl(descriptor, F_SETLK, &parameters);
        return false;
    }
    lseek(descriptor, (num_rec - 1) * sizeof(record_t), SEEK_SET);
    write(descriptor, base_record, sizeof(record_t));
    if(fcntl(descriptor, F_SETLKW, &parameters) < 0)
        perror("F_SETLKW");
    lseek(descriptor, 0, SEEK_SET);
    FLAG_EDIT = false;
    return true;
}
