#include "./so_scheduler.h"
#include "./utils.h"

#include "./pq.h"
#include "./sch_thread.h"
#include "./scheduler.h"

#include "./debug.h"

#include <semaphore.h>
#include <stdarg.h>
#include <stdlib.h>

#define IO_INIT_VEC_SIZE 4
#define IO_VEC_INC 2

#define SO_MAX_PRIO 5

/* static struct so_thread *curr_th; */
static struct sch *sch;
static void sch_plan(struct sch_thread *caller, struct sch_thread *child);

static void *so_fork_wrapper(void *arg);
static void so_free(void);

static void io_add(struct sch_thread *p, unsigned int io);
static int io_notify(unsigned int io);

static uint_fast64_t tick_lc(void);

DECL_PREFIX int so_init(unsigned int time_quantum, unsigned int io)
{
	size_t i;
	int ret;

	if (sch || !time_quantum || io > IO_NUM_MAX)
		return -1;

	sch = calloc(1, sizeof(struct sch));
	if (!sch)
		return -1;

	ret = pq_init(&sch->pq,
		sizeof(struct sch_thread),
		sch_thread_copy,
		sch_thread_compare_smaller,
		NULL);

	if (ret) {
		so_free();
		return -1;
	}

	sch->io.io_no = io;
	for (i = 0; i < io; ++i) {
		sch->io.v_sizes[0][i] = 0;
		sch->io.v_sizes[1][i] = IO_INIT_VEC_SIZE;
		sch->io.v[i] = calloc(IO_INIT_VEC_SIZE,
				sizeof(struct sch_thread *));
		if (!sch->io.v[i]) {
			so_free();
			return -1;
		}
	}

	sch->quantum = time_quantum;
	sch->running = NULL;
	sch->remaining_quantum = 0;
	pthread_mutex_init(&sch->lc_lock, NULL);

	/* scheduler thread ids vector init */
	sch->th_term.v = malloc(TH_TERM_INIT_SIZE
		* sizeof(struct sch_thread *));
	if (!sch->th_term.v) {
		so_free();
		return -1;
	}
	sch->th_term.end = 0;
	sch->th_term.allocated = TH_TERM_INIT_SIZE;
	pthread_mutex_init(&sch->th_term.lock, NULL);

	/* first val of logical clock */
	sch->lc = 1;

	return 0;
}

DECL_PREFIX tid_t so_fork(so_handler *func, unsigned int priority)
{
	struct sch_thread *nt;

	if (!sch || !func || priority > SO_MAX_PRIO)
		return INVALID_TID;

	/* allocations, initializations and defensive programing */
	nt = calloc(1, sizeof(*nt));
	if (!nt) {
		fprintf(stderr, "[WARN] Couldn't allocate in %s\n",
			__func__);
		return INVALID_TID;
	}
	nt->sem = malloc(sizeof(*nt->sem));
	if (!nt->sem) {
		fprintf(stderr, "[WARN] Couldn't allocate lock in %s\n",
			__func__);
		free(nt);
		return INVALID_TID;
	}
	nt->entry_time = tick_lc();
	nt->priority = priority;
	nt->func = func;
	if (sem_init(nt->sem, 0, 1)) {
		fprintf(stderr, "[WARN] Couldn't initialize lock in %s\n",
			__func__);
		free(nt->sem);
		free(nt);
		return INVALID_TID;
	}
	if (sem_wait(nt->sem)) {
		fprintf(stderr, "[WARN] Couldn't wait on semaphore in %s\n",
			__func__);
		sem_destroy(nt->sem);
		free(nt->sem);
		free(nt);
		return INVALID_TID;
	}
	if (pthread_create(&nt->id, NULL, so_fork_wrapper, nt)) {
		handle_error("[ERRO] Couldn't create thread in %s\n",
			__func__);
		return INVALID_TID;
	}
	/* put in the joining threads stack */
	sch_th_term_push_back(&sch->th_term, nt);

	/* reduce the remaining quantum */
	if (sch->running)
		--sch->remaining_quantum;

	/* check the scheduler */
	sch_plan(sch->running, nt);

	if (!sch->remaining_quantum)
		sch_plan(sch->running, NULL);

	return nt->id;
}

DECL_PREFIX int so_wait(unsigned int io)
{
	struct sch_thread *p;

	p = sch->running;

	if (!sch || io >= sch->io.io_no)
		return -1;

	io_add(p, io);

	sch_plan(NULL, NULL);
	if (sem_wait(p->sem))
		handle_error("[ERRO] sem_wait returned error in %s\n",
			__func__);

	return 0;
}

DECL_PREFIX int so_signal(unsigned int io)
{
	int ret;

	if (!sch || io >= sch->io.io_no)
		return -1;

	ret = io_notify(io);

	sch_plan(sch->running, NULL);

	return ret;
}

DECL_PREFIX void so_exec(void)
{
	if (!sch->running) {
		handle_error("[ERRO] called %s without a previous so_init "
			"call\n", __func__);
	}

	/* to do something */
	asm volatile("nop");

	/* reduce the remaining quantum */
	--sch->remaining_quantum;

	/* plan another thread if needed */
	if (!sch->remaining_quantum) {
		sch->running->entry_time = tick_lc();
		sch_plan(sch->running, NULL);
	}

}

DECL_PREFIX void so_end(void)
{
	int ret;
	struct sch_thread *t;

	if (!sch)
		return;

	while (sch_th_term_size(&sch->th_term)) {
		/* join */
		t = sch_th_term_pop_back(&sch->th_term);
		ret = pthread_join(t->id, NULL);
		if (ret) {
			fprintf(stderr, "[WARN] pthread_join retturned %d\n",
				ret);
		}
		/* free the closed thread */
		sch_thread_free(t);
	}

	so_free();
}

void so_free(void)
{
	size_t i;

	if (!sch)
		return;

	/* free the priority queue */
	pq_free(&sch->pq);

	if (sch->running)
		fprintf(stderr, "still a running thread\n");

	/* free the io vector */
	for (i = 0; i < sch->io.io_no; ++i) {
		if (sch->io.v[i])
			free(sch->io.v[i]);
		else
			break;
	}

	/* free vector of terminated threads */
	if (sch_th_term_size(&sch->th_term))
		fprintf(stderr, "[WARN] %s called but sch->th_term not "
			"empty\n", __func__);
	free(sch->th_term.v);

	free(sch);

	sch = NULL;
}

void sch_plan(struct sch_thread *caller, struct sch_thread *child)
{
	struct sch_thread *t;

	if (!sch) {
		handle_error("[ERRO] Scheduler not initialized in %s\n",
			__func__);
	}

	if (!caller && !child) {
		/* a thread terminated */
		t = pq_pop(&sch->pq, 1);
		if (!t) {
			sch->running = NULL;
			return;
		}

		sch->running = t;
		sch->remaining_quantum = sch->quantum;
		t->entry_time = tick_lc();

		if (sem_post(t->sem))
			handle_error(
				"[ERRO] sem_post returned error in %s on"
				" line %d\n", __func__, __LINE__);
	} else if (caller && !child) {
		/* a thread finished his quantum */
		t = pq_peek_p(&sch->pq);

		if (!t) {
			caller->entry_time = tick_lc();
			sch->running = caller;
			sch->remaining_quantum = sch->quantum;
		} else {
			if (!sch_thread_compare_smaller(caller, t)) {
				/* we keep caller running */
				sch->remaining_quantum = sch->quantum;
				caller->entry_time = tick_lc();
				sch->running = caller;
			} else {
				/* we change context */
				t = pq_pop(&sch->pq, 1);
				pq_emplace(&sch->pq, caller);

				sch->remaining_quantum = sch->quantum;
				t->entry_time = tick_lc();
				sch->running = t;

				/* we start the new thread */
				if (sem_post(sch->running->sem))
					handle_error(
						"[ERRO] sem_post returned "
						"error in %s on line"
						" %d\n", __func__, __LINE__);
				/* and block thisone */
				if (sem_wait(caller->sem))
					handle_error(
						"[ERRO] sem_post returned "
						"error in %s on line"
						" %d\n", __func__, __LINE__);
			}
		}
	} else if (caller && child) {
		/* called from a fork */
		if (!sch_thread_compare_smaller(caller, child)) {
			/* we keep caller running */
			pq_emplace(&sch->pq, child);

			caller->entry_time = tick_lc();
			sch->running = caller;
		} else {
			/* we change context */
			pq_emplace(&sch->pq, caller);

			sch->remaining_quantum = sch->quantum;
			child->entry_time = tick_lc();
			sch->running = child;

			/* we start the new thread */
			if (sem_post(sch->running->sem)) {
				handle_error(
					"[ERRO] sem_post returned error in "
					"%s on line %d\n", __func__, __LINE__);
			}
			/* and block thisone */
			if (sem_wait(caller->sem)) {
				handle_error(
					"[ERRO] sem_post returned error in "
					"%s on line %d\n", __func__, __LINE__);
			}
		}
	} else if (!caller && child) {
		/* the first call to fork from main */
		sch->remaining_quantum = sch->quantum;
		child->entry_time = tick_lc();
		sch->running = child;

		if (sem_post(sch->running->sem)) {
			handle_error(
				"[ERRO] sem_post returned error in %s on"
				" line %d\n", __func__, __LINE__);
		}
	} else {
		printf("Warning, we reached the else on line %d\n", __LINE__);
	}
}

void *so_fork_wrapper(void *arg)
{
	struct sch_thread *p;

	p = (struct sch_thread *)arg;
	if (sem_wait(p->sem)) {
		handle_error(
			"[ERRO] sem_wait returned error in %s for"
			" thread %ld\n", __func__, p->id);
	}

	/* start work */
	p->func(p->priority);

	/* do cleanup */
	sch_plan(NULL, NULL);

	pthread_exit(NULL);
}

void io_add(struct sch_thread *p, unsigned int io)
{
	struct sch_thread **ret;

	if (sch->io.v_sizes[0][io] == sch->io.v_sizes[1][io]) {
		ret = realloc(sch->io.v[io], sch->io.v_sizes[1][io]
			* IO_VEC_INC);
		if (!ret) {
			handle_error("[ERRO] Could not realloc in %s\n",
				__func__);
		}
		sch->io.v[io] = ret;
		sch->io.v_sizes[1][io] *= IO_VEC_INC;
	}
	sch->io.v[io][sch->io.v_sizes[0][io]] = p;
	++sch->io.v_sizes[0][io];
}

int io_notify(unsigned int io)
{
	int ret;
	size_t i;

	ret = sch->io.v_sizes[0][io];
	for (i = 0; i < sch->io.v_sizes[0][io]; ++i)
		pq_emplace(&sch->pq, sch->io.v[io][i]);

	sch->io.v_sizes[0][io] = 0;

	return ret;
}

uint_fast64_t tick_lc(void)
{
	uint_fast64_t ret;

	if (!sch)
		return 0;

	pthread_mutex_lock(&sch->lc_lock);
	ret = ++sch->lc;
	pthread_mutex_unlock(&sch->lc_lock);

	return ret;
}

#undef IO_INIT_VEC_SIZE
#undef IO_VEC_INC
#undef SO_MAX_PRIO
