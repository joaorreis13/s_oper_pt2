#include <pthread.h>
#include <stdlib.h>

typedef struct queue {
	size_t size;
	size_t max_size;
	size_t head, tail;
	char **data;
	pthread_mutex_t mut;
	pthread_cond_t notFull, notEmpty;
} queue;

int queue_new(queue **qp, int max_size) {
	if (!qp)
		return -1;
	queue *q = malloc(sizeof(*q));
	if (!q)
		return -1;
	q->size = q->head = q->tail = 0;
	q->max_size = max_size;
	q->data = malloc(sizeof(*q->data) * max_size);
	if (!q->data) {
		free(q);
		return -1;
	}
	pthread_mutex_init(&q->mut, NULL);
	pthread_cond_init(&q->notEmpty, NULL);
	pthread_cond_init(&q->notFull, NULL);
	*qp = q;
	return 0;
}

int queue_put(queue *q, char *data) {
	if (!q || !data)
		return -1;
	pthread_mutex_lock(&q->mut);
	while (q->size == q->max_size)
		pthread_cond_wait(&q->notFull, &q->mut);
	q->data[q->tail] = data;
	q->tail = (q->tail + 1) % q->max_size;
	++q->size;
	pthread_mutex_unlock(&q->mut);
	pthread_cond_signal(&q->notEmpty);
	return 0;
}

int queue_take(queue *q, char **data) {
	if (!q || !data)
		return -1;
	pthread_mutex_lock(&q->mut);
	while (!q->size)
		pthread_cond_wait(&q->notEmpty, &q->mut);
	*data = q->data[q->head];
	q->head = (q->head + 1) % q->max_size;
	--q->size;
	pthread_mutex_unlock(&q->mut);
	pthread_cond_signal(&q->notFull);
	return 0;
}

void queue_free(queue *q) {
	if (!q) {
		free(q->data);
		pthread_mutex_destroy(&q->mut);
		pthread_cond_destroy(&q->notEmpty);
		pthread_cond_destroy(&q->notFull);
	}
	free(q);
	return;
}
