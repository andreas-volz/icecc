/* queue.h */

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include "scdef.h"
#include "list.h"

typedef struct Queue {
  List *front;
  List *end;
} Queue;

typedef struct ObjQueue {
  ObjList *front;
  ObjList *end;
} ObjQueue;

/* exported declarations */
extern Queue *queue_new();
extern void queue_insert(Queue *q, long item);
extern long queue_remove(Queue *q);
extern int queue_isempty(Queue *q);
extern void queue_free(Queue *q);
extern ObjQueue *objqueue_new();
extern void objqueue_insert(ObjQueue *q, void *item);
extern void objqueue_putback(ObjQueue *q, void *item);
extern void *objqueue_remove(ObjQueue *q);
extern int objqueue_isempty(ObjQueue *q);
extern void objqueue_freeall(ObjQueue *q);
extern void objqueue_free(ObjQueue *q);

#endif
