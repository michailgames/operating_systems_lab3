#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

#define WRITERS_PERCENT 33
#define MAX_SPAWN_INTERVAL 50000
#define N 20
#define M 1200
#define WRITER 1
#define READER 2
#define ERROR(str) { fprintf(stderr, "%s: %s\n", str, strerror(errno)); exit(1); }

int readers_counter = 0;
pthread_mutex_t counter_mutex, read_mutex, write_mutex;

int types[M];
long waiting_times[M];
pthread_t threads[M];
int indices[M];

int writers_created = 0;
int readers_created = 0;
long longest_writer_waiting = 0;
long longest_reader_waiting = 0;

void play_with_books(int i, struct timeval *t1) {
    struct timeval t2;
    gettimeofday(&t2, NULL);
    waiting_times[i] = 1000000 * (t2.tv_sec - t1->tv_sec) +
            (t2.tv_usec - t1->tv_usec);
    
    struct timespec play_time;
    play_time.tv_sec = 0;
    play_time.tv_nsec = 10000 + rand() % 90000;
    nanosleep(&play_time, NULL);
}

void writer(int i) {
    types[i] = WRITER;
    struct timeval t1;
    gettimeofday(&t1, NULL);
    pthread_mutex_lock(&read_mutex);
    pthread_mutex_lock(&write_mutex);
    pthread_mutex_unlock(&read_mutex);
    
    play_with_books(i, &t1);
    
    pthread_mutex_unlock(&write_mutex);
}

void reader(int i) {
    types[i] = READER;
    struct timeval t1;
    gettimeofday(&t1, NULL);
    pthread_mutex_lock(&read_mutex);
        pthread_mutex_lock(&counter_mutex);
            readers_counter++;
            if(readers_counter == 1) {
                pthread_mutex_lock(&write_mutex);
            }
        pthread_mutex_unlock(&counter_mutex);
    pthread_mutex_unlock(&read_mutex);
    
    play_with_books(i, &t1);
    
    pthread_mutex_lock(&counter_mutex);
        readers_counter--;
        if(readers_counter == 0) {
            pthread_mutex_unlock(&write_mutex);
        }
    pthread_mutex_unlock(&counter_mutex);
}

void *thread_action(void *thread_index_ptr) {
    int i = *((int *) thread_index_ptr);
	if(rand() % 100 < WRITERS_PERCENT) {
	    writer(i);
	}
	else {
	    reader(i);
	}
	return NULL;
}

int main() {
    srand(time(NULL));
    if(pthread_mutex_init(&counter_mutex, NULL)) {
        ERROR("Mutex initialization failed");
    }
    if(pthread_mutex_init(&read_mutex, NULL)) {
        ERROR("Mutex initialization failed");
    }
    if(pthread_mutex_init(&write_mutex, NULL)) {
        ERROR("Mutex initialization failed");
    }
    
    for(int i = 0; i < M; i++) {
        indices[i] = i;
    }
    struct timespec wait_time;
    int n = 0;
    while(n < M) {
        wait_time.tv_sec = 0;
        wait_time.tv_nsec = rand() % MAX_SPAWN_INTERVAL;
        nanosleep(&wait_time, NULL);
        for(int i = 0; i < N; i++) {
            if(pthread_create(&threads[n], NULL, thread_action, &indices[n])) {
        		ERROR("Failed to create thread");
        	}
        	n++;
        }
    }
    
    for(int i = 0; i < M; i++) {
        if(pthread_join(threads[i], NULL)) {
			ERROR("Failed to join thread");
		}
        if(types[i] == WRITER) {
            writers_created++;
            if(waiting_times[i] > longest_writer_waiting) {
                longest_writer_waiting = waiting_times[i];
            }
        }
        else if(types[i] == READER) {
            readers_created++;
            if(waiting_times[i] > longest_reader_waiting) {
                longest_reader_waiting = waiting_times[i];
            }
        }
    }
    printf("%d writers created.\n", writers_created);
    printf("%d readers created.\n", readers_created);
    printf("Longest waiting writer: %ld usec.\n", longest_writer_waiting);
    printf("Longest waiting reader: %ld usec.\n", longest_reader_waiting);
    
    pthread_mutex_destroy(&counter_mutex);
    pthread_mutex_destroy(&read_mutex);
    pthread_mutex_destroy(&write_mutex);
    return 0;
}
