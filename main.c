#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <threads.h>
#include <pthread.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>

typedef struct {
    double time_mark;
    uint64_t recno;
} index_record;

typedef struct {
    uint64_t records;
    index_record * idx;
} index_hdr_s;

typedef struct {
    index_record * buf;
    int blockSize;
    int threadNum;
} thread_args;

typedef struct {
    int blockSize;
    int threads;
    char* file_name;
} initial_data;

extern int size;
extern int blocks;
extern int threads;

extern pthread_barrier_t barrier;
extern pthread_mutex_t mutex;
extern index_record* current_record;

int size;
int blocks;
int threads;

pthread_barrier_t barrier;                     // Барьер для синхронизации потоков, мьютекс и текущая запись.
pthread_mutex_t mutex;
index_record* current_record;

void generate(size_t file_size, char* file_name);
void read_file(char* file_name);
int comparator(const void* a, const void* b);
void* sorting(void* thread_arguments);
void* image_file(void* data);

void generate(size_t file_size, char* file_name) {
    if(file_name != NULL) {
        srand(time(NULL));                                                            //Инициализирует генератор псевдослучайных чисел с помощью текущего времени
        index_hdr_s header;                                                           //Эта структура содержит кол-во записей и массив записей типа index_record
        header.records = file_size;                                               // Получение размера файлы.
        if (header.records % 256 != 0) {
            printf("Should be dividable by 256.\n");
            exit(0);
        }
        header.idx = (index_record *)malloc(header.records * sizeof(index_record));   // Выделение места.
        for (int i = 0; i < header.records; i++) {
            header.idx[i].recno = i + 1;                                              // Генерация структур.
            header.idx[i].time_mark = 15020 + rand() % (60378 - 15020 + 1);
            header.idx[i].time_mark +=
                    0.5 * ((rand() % 24) * 60 * 60 + (rand() % 60) * 60 + rand() % 60) / (12 * 60 * 60);
        }

        FILE *f = fopen(file_name, "wb");
        if (f == NULL)
            printf("Error while creating file.\n");
        else {
            fwrite(&header.records, sizeof(header.records), 1, f);
            fwrite(header.idx, sizeof(index_record), header.records, f);               // Запись в файл.
        }
        fclose(f);
        free(header.idx);
    } else
        printf("File name error.\n");
}

void read_file(char* file_name) {
    if(file_name != NULL) {
        FILE *f = fopen(file_name, "rb");                                             // Открыть файл.
        if(f==NULL) {
            printf("Error while open file.\n");
            exit(0);
        }
        index_hdr_s* data = (index_hdr_s*)malloc(sizeof(index_hdr_s));
        if(!fread(&data->records, sizeof(uint64_t), 1, f)) {                        // Чтение шапки.
            printf("Error while reading records.\n");
            fclose(f);
            exit(0);
        }
        data->idx = (index_record*)malloc(data->records*sizeof(index_record));
        if(!fread(data->idx, sizeof(index_record), data->records, f)) {             // Чтение массива.
            printf("Error while reading idx.\n");
            fclose(f);
            exit(0);
        }

        //Выводим значение time_mark каждой записи
        for(int i = 0; i<data->records; i++)
            printf("%lf\n", data->idx[i].time_mark);
        fclose(f);
    }
}

int comparator(const void* a, const void* b) {           // Сравнение двух объектов для qsort.
    if (((index_record*)a)->time_mark < ((index_record*)b)->time_mark)
        return -1;
    else if(((index_record*)a)->time_mark > ((index_record*)b)->time_mark)
        return 1;
    else
        return 0;
}

void* sorting(void* thread_arguments) {
    thread_args* args = (thread_args*)thread_arguments;          // Получение параметров файла.
    pthread_barrier_wait(&barrier);

    printf("Qsort in %d thread.\n", args->threadNum); // Сортировка блоков qsort.
    while(current_record < args->buf + size) {
        pthread_mutex_lock(&mutex);
        if(current_record < args->buf + size) {
            index_record *temp = current_record;
            current_record += args->blockSize;
            pthread_mutex_unlock(&mutex);
            qsort(temp, args->blockSize, sizeof(index_record), comparator);
        } else {
            pthread_mutex_unlock(&mutex);
            pthread_barrier_wait(&barrier);
            break;
        }
    }
    printf("Merging in %d thread.\n", args->threadNum);
    int mergeStep = 2;
    // merge
    while(mergeStep<=blocks) {                       // Слияние отсортированных блоков.
        pthread_barrier_wait(&barrier);
        current_record = args->buf;
        while (current_record < args->buf + size) {
            pthread_mutex_lock(&mutex);
            if(current_record < args->buf + size) {
                index_record *temp = current_record;
                current_record += mergeStep * args->blockSize;
                pthread_mutex_unlock(&mutex);
                int bufSize = (mergeStep / 2) * args->blockSize;
                index_record *left = (index_record *) malloc(bufSize * sizeof(index_record));
                memcpy(left, temp, (mergeStep / 2) * args->blockSize*sizeof(index_record));
                index_record *right = (index_record *) malloc(bufSize * sizeof(index_record));
                memcpy(right, temp + (mergeStep / 2) * args->blockSize,(mergeStep / 2) * args->blockSize*sizeof(index_record));

                int i = 0, j = 0;
                while (i < bufSize && j < bufSize) {
                    if (left[i].time_mark < right[j].time_mark) {
                        temp[i+j].time_mark = left[i].time_mark;
                        temp[i+j].recno = left[i].recno;
                        i++;
                    } else {
                        temp[i+j].time_mark = right[j].time_mark;
                        temp[i+j].recno = right[j].recno;
                        j++;
                    }
                }
                if (i == bufSize) {
                    while (j > bufSize) {
                        temp[i+j].time_mark = right[j].time_mark;
                        temp[i+j].recno = right[j].recno;
                        j++;
                    }
                }
                if (j == bufSize) {
                    while (i < bufSize) {
                        temp[i+j].time_mark = left[i].time_mark;
                        temp[i+j].recno = left[i].recno;
                        i++;
                    }
                }
            } else {
                pthread_mutex_unlock(&mutex);
                break;
            }
        }
        mergeStep*=2;
    }
    pthread_mutex_unlock(&mutex);
    pthread_barrier_wait(&barrier);
}

void *image_file(void* data) {
    printf("Imaging data\n");
    initial_data* init_data = (initial_data*)data;

    FILE* f = fopen(init_data->file_name, "rb+");       // Открыть файл.
    if(f==NULL){
        printf("Error while open file.\n");
        return 0;
    }

    fseek(f, 0, SEEK_END);
    size_t fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    int fd = fileno(f);
    void * buf;                                 // Отобразить файл в память.
    if((buf = mmap(NULL, fsize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0))== MAP_FAILED) {
        printf("Error while creating mapping, %d\n", errno);
        exit(errno);
    }

    buf+=sizeof(uint64_t);                      // Переход к первой структуре.

    current_record = (index_record *)buf;
    printf("Thead opening\n");
    pthread_t threadsId[init_data->threads - 1];      // Запуск потоков.
    for(int i = 1; i < init_data->threads; i++) {
        thread_args* args = (thread_args*)malloc(sizeof(thread_args));
        args->blockSize = init_data->blockSize;
        args->threadNum = i;
        args->buf = (index_record*)buf;

        if(pthread_create(&threadsId[i-1], NULL, sorting, args) != 0) {
            printf("Error while creating %d thread.\n", i);
            exit(0);
        }
    }
    thread_args* args = (thread_args*)malloc(sizeof(thread_args));
    args->blockSize = init_data->blockSize;
    args->threadNum = 0;
    args->buf = (index_record*)buf;
    sorting(args);

    for(int i = 1; i < init_data->threads; i++)
        pthread_join(threadsId[i-1], NULL);     // Ожидание завершения.
    printf("Threads closed\n");

    munmap(buf, fsize);
    fclose(f);
    printf("Memory freed\n");
}

int main(int argc, char* argv[]) {
    size = atoi(argv[1]);
    blocks = atoi(argv[2]);
    threads = atoi(argv[3]);
    char *file_name = argv[4];

    generate(size, file_name);
    read_file(file_name);

    pthread_barrier_init(&barrier, NULL, threads);
    pthread_mutex_init(&mutex, NULL);
    initial_data *cd = (initial_data *) malloc(sizeof(initial_data));

    cd->blockSize = size / blocks;
    cd->threads = threads;
    cd->file_name = argv[4];

    pthread_t pthreadId;
    pthread_create(&pthreadId, NULL, image_file, cd);

    pthread_join(pthreadId, NULL);
    pthread_barrier_destroy(&barrier);
    pthread_mutex_destroy(&mutex);
    printf("Programm ended\n");

    read_file(file_name);
    return 0;
}