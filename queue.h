#pragma once

typedef struct queue {
	size_t size;
	size_t max_size;
	size_t head, tail;
	char **data;
	pthread_mutex_t mut;
	pthread_cond_t notFull, notEmpty;
} queue;

int queue_new(queue **qp, int max_size);
int queue_put(queue *q, char *data);
int queue_take(queue *q, char **data);
void queue_free(queue *q);
