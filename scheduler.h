#ifndef SCHEDULER_H
#define SCHEDULER_H

#include <pthread.h>
#include <stdarg.h>

#include "./pq.h"
#include "./sch_thread.h"

#define IO_NUM_MAX 256U

struct io_wait {
	uint_fast16_t io_no;
	struct sch_thread **v[IO_NUM_MAX];
	/*
	 * [0] - end
	 * [1] - allocated
	 */
	size_t v_sizes[2][IO_NUM_MAX];
};

struct sch {
	struct pq pq;
	struct sch_thread *running;
	size_t remaining_quantum;
	struct io_wait io;

	struct th_term th_term;

	size_t quantum;

	uint_fast64_t lc;
	pthread_mutex_t lc_lock;
};

#endif /* SCHEDULER_H */
