#pragma once

struct queue;
typedef struct queue queue;

int queue_new(queue **qp, int max_size);
int queue_put(queue *q, char *data);
int queue_take(queue *q, char **data);
void queue_free(queue *q);
