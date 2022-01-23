#include "./pq.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INIT_SIZE 64
#define SIZE_INC_MUL 2

/* Function to return the index of the parent node of a given node */
static int parent(int i);
/* Function to return the index of the left child of the given node */
static int leftChild(int i);
/* Function to return the index of the right child of the given node */
static int rightChild(int i);
/* Function to shift up the node in order to maintain the heap property */
static void shiftUp(struct pq *pq, int i);
/* Function to shift down the node in order to maintain the heap property */
static void shiftDown(struct pq *pq, int i);

int pq_init(struct pq *pq, const size_t sizeof_elem,
		void *(*d_copy)(const void *src),
		int (*d_compare_smaller)(void *a, void *b),
		void (*d_free)(void *a))
{
	if (!pq || !sizeof_elem || !d_compare_smaller)
		return -1;

	pq->allocated = INIT_SIZE;
	pq->size = -1;
	pq->d_copy = d_copy;
	pq->d_compare = d_compare_smaller;
	pq->d_free = d_free;
	pq->v = calloc(pq->allocated, sizeof(void *));
	if (!pq->v) {
		return -1;
	}

	return 0;
}

int pq_insert(struct pq *pq, const void *data)
{
	void *np;

	if (!pq || !data)
		return -1;

	np = pq->d_copy(data);
	if (!np)
		return -1;

	return pq_emplace(pq, np);
}

int pq_emplace(struct pq *pq, void *data)
{
		void **p;

	if (!pq || !data)
		return -1;

	++pq->size;
	if ((size_t)pq->size >= pq->allocated) {
		p = (void **)realloc(pq->v,
				pq->allocated * SIZE_INC_MUL * sizeof(void *));
		if (!p) {
			--pq->size;
			return -1;
		}
		pq->allocated *= SIZE_INC_MUL;
		pq->v = p;
	}

	pq->v[pq->size] = data;

	/* Shift Up to maintain heap property */
	shiftUp(pq, pq->size);

	return 0;
}

void *pq_peek(const struct pq *pq)
{
	void *p;

	if (!pq)
		return NULL;

	if (pq->size >= 0)
		p = pq->d_copy(pq->v[0]);
	else
		p = NULL;

	return p;
}

void *pq_peek_p(const struct pq *pq)
{
	void *p;

	if (!pq)
		return NULL;

	if (pq->size >= 0)
		p = pq->v[0];
	else
		p = NULL;

	return p;
}

void *pq_pop(struct pq *pq, const int return_data)
{
	void *n;
	void **p;

	if (!pq)
		return NULL;

	if (pq->size < 0)
		return NULL;

	n = pq->v[0];

	pq->v[0] = pq->v[pq->size];
	pq->v[pq->size] = NULL;
	--pq->size;

	if (pq->size < (int)pq->allocated / (SIZE_INC_MUL * SIZE_INC_MUL)
			&& pq->size > INIT_SIZE) {
		p = (void **)realloc(
				pq->v,
				pq->allocated / SIZE_INC_MUL * sizeof(void *));
		if (p) {
			pq->v = p;
			pq->allocated /= SIZE_INC_MUL;
		}
	}

	shiftDown(pq, 0);

	if (!return_data) {
		if (pq->d_free)
			pq->d_free(n);
		n = NULL;
	}
	return n;
}

void pq_free(struct pq *pq)
{
	int i;

	if (!pq)
		return;

	if (pq->d_free) {
		for (i = 0; i <= pq->size; ++i) {
			pq->d_free(pq->v[i]);
		}
	}

	free(pq->v);
}

int parent(int i)
{
	return (i - 1) / 2;
}

int leftChild(int i)
{
	return ((2 * i) + 1);
}

int rightChild(int i)
{
	return ((2 * i) + 2);
}

void shiftUp(struct pq *pq, int i)
{
	void *temp;

	while (i > 0 && pq->d_compare(pq->v[parent(i)], pq->v[i])) {

		/* Swap parent and current node */
		temp = pq->v[i];
		pq->v[i] = pq->v[parent(i)];
		pq->v[parent(i)] = temp;

		/* Update i to parent of i */
		i = parent(i);
	}
}

void shiftDown(struct pq *pq, int i)
{
	int l, r, max_index;
	void *temp;

	max_index = i;

	/* Left Child */
	l = leftChild(i);

	if (l <= pq->size && !pq->d_compare(pq->v[l], pq->v[max_index]))
		max_index = l;


	/* Right Child */
	r = rightChild(i);

	if (r <= pq->size && !pq->d_compare(pq->v[r], pq->v[max_index]))
		max_index = r;


	/* If i not same as max_index */
	if (i != max_index) {
		temp = pq->v[i];
		pq->v[i] = pq->v[max_index];
		pq->v[max_index] = temp;

		shiftDown(pq, max_index);
	}
}

#undef INIT_SIZE
#undef SIZE_INC_MUL
