#ifndef SCH_THREAD_H
#define SCH_THREAD_H

#include <pthread.h>
#include <semaphore.h>
#include <stdint.h>

#include "./so_scheduler.h"

#define TH_TERM_INIT_SIZE 64
#define TH_TERM_SIZE_INC_MUL 2

struct sch_thread {
	pthread_t id;
	uint_fast64_t entry_time;
	uint_fast8_t priority;
	sem_t *sem;
	so_handler *func;
};

struct th_term {
	struct sch_thread **v;
	size_t end;
	size_t allocated;
	pthread_mutex_t lock;
};

void *sch_thread_copy(const void *src);
int sch_thread_compare_smaller(void *_a, void *_b);
void sch_thread_free(void *a);

int sch_th_term_push_back(struct th_term *th_term, struct sch_thread *t);
struct sch_thread *sch_th_term_pop_back(struct th_term *th_term);
int sch_th_term_size(const struct th_term *th_term);

#endif /* SCH_THREAD_H */
