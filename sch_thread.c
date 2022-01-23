#include "./sch_thread.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *sch_thread_copy(const void *src)
{
	struct sch_thread *n;

	n = malloc(sizeof(struct sch_thread));
	if (!n)
		return NULL;

	memcpy(n, src, sizeof(struct sch_thread));
	return n;
}

int sch_thread_compare_smaller(void *_a, void *_b)
{
	struct sch_thread *a;
	struct sch_thread *b;

	a = (struct sch_thread *)_a;
	b = (struct sch_thread *)_b;

	if (a->priority == b->priority)
		return a->entry_time > b->entry_time;
	else
		return a->priority < b->priority;

}

void sch_thread_free(void *a)
{
	sem_destroy(((struct sch_thread *)a)->sem);
	free(((struct sch_thread *)a)->sem);
	free(a);
}

int sch_th_term_push_back(struct th_term *th_term, struct sch_thread *t)
{
	struct sch_thread **ret;

	pthread_mutex_lock(&th_term->lock);
	/* make space if needed */
	if (th_term->end == th_term->allocated) {
		ret = realloc(th_term->v, th_term->allocated
			* TH_TERM_SIZE_INC_MUL * sizeof(struct sch_thread *));
		if (!ret) {
			fprintf(stderr, "[WARN] Could not reallocate in %s\n",
				__func__);
			pthread_mutex_unlock(&th_term->lock);
			return -1;
		}
		th_term->v = ret;
		th_term->allocated *= TH_TERM_SIZE_INC_MUL;
	}

	th_term->v[th_term->end] = t;
	++th_term->end;

	pthread_mutex_unlock(&th_term->lock);
	return 0;
}

struct sch_thread *sch_th_term_pop_back(struct th_term *th_term)
{
	struct sch_thread *ret;

	pthread_mutex_lock(&th_term->lock);
	if (th_term->end == 0) {
		ret = 0;
	} else {
		--th_term->end;
		ret = th_term->v[th_term->end];
	}
	pthread_mutex_unlock(&th_term->lock);

	return ret;
}

int sch_th_term_size(const struct th_term *th_term)
{
	return th_term->end;
}
