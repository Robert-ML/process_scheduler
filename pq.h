#ifndef PQ_H
#define PQ_H

#include <stddef.h>

struct pq {
	void **v;
	size_t allocated;
	int size;
	// size_t elem_size;
	void *(*d_copy)(const void *src);
	int (*d_compare)(void *a, void *b);
	void (*d_free)(void *a);
};

/* int (*d_compare_smaller)(void *a, void *b) returns 1 if a < b */
int pq_init(struct pq *pq, const size_t sizeof_elem,
		void *(*d_copy)(const void *src),
		int (*d_compare_smaller)(void *a, void *b),
		void (*d_free)(void *a));
int pq_insert(struct pq *pq, const void *data);
int pq_emplace(struct pq *pq, void *data);
/* returns of copy of max data */
void *pq_peek(const struct pq *pq);
/* return opinter to the top */
void *pq_peek_p(const struct pq *pq);
/* removes max, if (return_data) return data; else free(data); */
void *pq_pop(struct pq *pq, const int return_data);
void pq_free(struct pq *pq);

#endif /* PQ_H */
