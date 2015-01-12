#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include <time.h>

#define ERROR(str) { fprintf(stderr, "%s: %s\n", str, strerror(errno)); exit(1); }
#define CLIENTS_TO_SPAWN 50
#define MAX_SPAWN_INTERVAL 300000000
#define MAX_EAT_TIME 950000000

struct restaurant {
	int waiting;
	int eating;
	int must_wait;
};

const char *shm_name = "/ramen_restaurant_shm";
const char *queue_sem_name = "/ramen_restaurant_queue_sem";
const char *global_sem_name = "/ramen_restaurant_global_sem";

int clients_to_spawn = CLIENTS_TO_SPAWN;

void wait(sem_t *sem) {
	if(sem_wait(sem) < 0) {
		ERROR("Semaphore wait error");
	}
}

void post(sem_t *sem) {
	if(sem_post(sem) < 0) {
		ERROR("Semaphore post error");
	}
}

void client_function(int number) {
	sem_t *queue_sem = sem_open(queue_sem_name, 0);
	if(queue_sem == SEM_FAILED) {
		ERROR("Failed to open semaphore");
	}
	sem_t *global_sem = sem_open(global_sem_name, 0);
	if(global_sem == SEM_FAILED) {
		ERROR("Failed to open semaphore");
	}
	int shm = shm_open(shm_name, O_RDWR, 0);
	if(shm < 0) {
		ERROR("Failed to open shared memory");
	}
	struct restaurant *restaurant_p = (struct restaurant *) mmap(0,
		sizeof(struct restaurant), PROT_READ|PROT_WRITE, MAP_SHARED, shm, 0);
	if(restaurant_p == MAP_FAILED) {
		ERROR("mmap error");
	}
	
	char buffer[128];
	wait(global_sem);
		sprintf(buffer, "Client #%d coming to restaurant.\n", number);
		write(STDOUT_FILENO, buffer, strlen(buffer));
		restaurant_p->waiting++;
	post(global_sem);

	wait(queue_sem);
	
	wait(global_sem);
		restaurant_p->waiting--;
		restaurant_p->eating++;
		if(restaurant_p->eating == 5) {
			restaurant_p->must_wait = 1;
		}
		sprintf(buffer, "Client #%d starts eating (%d/5).\n",
					number, restaurant_p->eating);
		write(STDOUT_FILENO, buffer, strlen(buffer));
	post(global_sem);
	
	struct timespec eat_time;
	eat_time.tv_sec = 0;
	eat_time.tv_nsec = rand() % MAX_EAT_TIME;
	nanosleep(&eat_time, NULL);
	wait(global_sem);
		sprintf(buffer, "Client #%d finished eating.\n", number);
		write(STDOUT_FILENO, buffer, strlen(buffer));
		restaurant_p->eating--;
		if(restaurant_p->must_wait == 0) {
			post(queue_sem);
		}
		else if(restaurant_p->eating == 0) {
			restaurant_p->must_wait = 0;
			for(int i = 0; i < 5; i++) {
				post(queue_sem);
			}
		}
	post(global_sem);
	
	if(number == CLIENTS_TO_SPAWN) { // UNLINK SHARED OBJECTS
		if(sem_unlink(queue_sem_name) < 0) ERROR("sem_unlink error");
		if(sem_unlink(global_sem_name) < 0) ERROR("sem_unlink error");
		if(shm_unlink(shm_name) < 0) ERROR("shm_unlink error");
	}	
}

void spawn_clients(int next_client_number) {
	if(clients_to_spawn <= 0) {
		sleep(3);
		return;
	}
	struct timespec wait_time;
    wait_time.tv_sec = 0;
    wait_time.tv_nsec = rand() % MAX_SPAWN_INTERVAL;
    nanosleep(&wait_time, NULL);
    clients_to_spawn--;
    if(fork()) {
    	spawn_clients(next_client_number + 1);
    }
    else {
    	client_function(next_client_number);
    }
}

int main() {
	sem_unlink(queue_sem_name);
	sem_unlink(global_sem_name);
	srand(time(NULL));

	sem_t *queue_sem = sem_open(queue_sem_name, O_CREAT|O_EXCL, 0644, 5);
	if(queue_sem == SEM_FAILED) {
		ERROR("Failed to create semaphore");
	}
	sem_t *global_sem = sem_open(global_sem_name, O_CREAT|O_EXCL, 0644, 1);
	if(global_sem == SEM_FAILED) {
		ERROR("Failed to create semaphore");
		
	}
	int shm = shm_open(shm_name, O_CREAT|O_RDWR, 0644);
	if(shm < 0) {
		ERROR("Failed to create shared memory");
	}
	if(ftruncate(shm, sizeof(struct restaurant)) < 0) {
		ERROR("Failed to set shared memory length");
	}
	struct restaurant *restaurant_p = (struct restaurant *) mmap(0,
		sizeof(struct restaurant), PROT_READ|PROT_WRITE, MAP_SHARED, shm, 0);
	if(restaurant_p == MAP_FAILED) {
		ERROR("mmap error");
	}
	restaurant_p->waiting = 0;
	restaurant_p->eating = 0;
	restaurant_p->must_wait = 0;
	
	spawn_clients(1);
	
	if(sem_close(global_sem) < 0) ERROR("sem_close error");
	if(sem_close(queue_sem) < 0) ERROR("sem_close error");
	
	return 0;
}
