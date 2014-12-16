#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/time.h>
#include <errno.h>
#include <time.h>

#define MAX_GOODS 100000
#define N 128
#define ERROR(str) { fprintf(stderr, "%s: %s\n", str, strerror(errno)); exit(1); }

int hungers[8];
int produced[8];
int indices[8];
int produced_total = 0;
int goods_in_shop = 0;
pthread_mutex_t produce_mutex, consume_mutex;
pthread_cond_t place_in_shop;

void random_delay() {
    struct timespec wait_time;
    wait_time.tv_sec = 0;
    wait_time.tv_nsec = rand() % 10;
    nanosleep(&wait_time, NULL);
}

void *producer(void *producer_index_ptr) {
    int i = *((int *) producer_index_ptr);
    printf("Producer #%d started.\n", i);
    int should_produce = 1;
    while(should_produce) {
        random_delay();   
        pthread_mutex_lock(&produce_mutex);
            while(goods_in_shop >= N) {
                pthread_cond_wait(&place_in_shop, &produce_mutex);
            }
            if(produced_total >= MAX_GOODS) {
                should_produce = 0;
            }
            else { 
                pthread_mutex_lock(&consume_mutex);           
                    produced[i]++;
                    produced_total++;
                    goods_in_shop++;
                pthread_mutex_unlock(&consume_mutex);
            }
        pthread_mutex_unlock(&produce_mutex);
    }
    printf("Producer #%d finished, produced: %d goods.\n", i, produced[i]);
    return NULL;
}

void *consumer(void *consumer_index_ptr) {
    int i = *((int *) consumer_index_ptr);
    printf("Consumer #%d started, target: %d goods.\n", i, hungers[i]);
    while(hungers[i] > 0) {
        random_delay();
        pthread_mutex_lock(&consume_mutex);           
            hungers[i]--;
            goods_in_shop--;
            pthread_cond_signal(&place_in_shop);
        pthread_mutex_unlock(&consume_mutex);
    }
    printf("Consumer #%d finished.\n", i);
    return NULL;
}

int main() {
    srand(time(NULL));
    if(pthread_mutex_init(&produce_mutex, NULL)) {
        ERROR("Mutex initialization failed");
    }
    if(pthread_mutex_init(&consume_mutex, NULL)) {
        ERROR("Mutex initialization failed");
    }
    if(pthread_cond_init(&place_in_shop, NULL)) {
        ERROR("Pthread_cond initialization failed");
    }
    
    pthread_t threads[16];   
    int hunger_left = MAX_GOODS;
    for(int i = 0; i < 8; i++) {
        if(pthread_create(&threads[i], NULL, producer, &indices[i])) {
    		ERROR("Failed to create thread");
    	}
    	if(i == 7) {
    	    hungers[i] = hunger_left;
    	}
    	else {
        	int v = (MAX_GOODS / 48);
        	int h = rand() % (2*v) + (MAX_GOODS / 8) - v;
        	hungers[i] = h;
        	hunger_left -= h;
        }
        indices[i] = i;
        produced[i] = 0;
    	if(pthread_create(&threads[i+8], NULL, consumer, &indices[i])) {
    		ERROR("Failed to create thread");
    	}
    }
    for(int i = 0; i < 16; i++) {
        if(pthread_join(threads[i], NULL)) {
			ERROR("Failed to join thread");
		}
    }
    pthread_mutex_destroy(&produce_mutex);
    pthread_mutex_destroy(&consume_mutex);
    pthread_cond_destroy(&place_in_shop);
    printf("Total production: %d goods.\n", produced_total);
    printf("Left in shop: %d goods.\n", goods_in_shop);
    return 0;
}
