/*
  IceCC. This file implements a generic queue 
  Copyright (C) 2000-2001 Jeffrey Pang <jp@magnus99.dhs.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. */

/* $Id: queue.c,v 1.9 2001/01/14 05:30:46 jp Exp $ */

#include <stdlib.h>
#include "scdef.h"
#include "queue.h"

Queue *queue_new() {
  Queue *q = (Queue *)malloc(sizeof(Queue));
  q->front = NULL;
  q->end = NULL;
  return q;
}

void queue_insert(Queue *q, long item) {
  List *l = (List *)malloc(sizeof(List));
  l->head = item;
  l->tail = NULL;
  if (q->front == NULL) {
    q->front = q->end = l;
    return;
  }
  q->end->tail = l;
  q->end = l;
}

long queue_remove(Queue *q) {
  List *l;
  uint16 result;
  if (q->front == NULL) return NULL_UINT16;
  l = q->front;
  result = l->head;
  q->front = q->front->tail;
  free(l);
  return result;
}

int queue_isempty(Queue *q) {
  return q->front == NULL;
}

void queue_free(Queue *q) {
  while (!queue_isempty(q))
    queue_remove(q);
  free(q);
}

ObjQueue *objqueue_new() {
  ObjQueue *q = (ObjQueue *)malloc(sizeof(ObjQueue));
  q->front = NULL;
  q->end = NULL;
  return q;
}

void objqueue_insert(ObjQueue *q, void *item) {
  ObjList *l = (ObjList *)malloc(sizeof(ObjList));
  l->head = item;
  l->tail = NULL;
  if (q->front == NULL) {
    q->front = q->end = l;
    return;
  }
  q->end->tail = l;
  q->end = l;
}

void objqueue_putback(ObjQueue *q, void *item) {
  ObjList *l = (ObjList *)malloc(sizeof(ObjList));
  l->head = item;
  l->tail = q->front;
  q->front = l;
  if (q->end == NULL)
    q->end = l;
}

void *objqueue_remove(ObjQueue *q) {
  ObjList *l;
  void *result;
  if (q->front == NULL) return NULL;
  l = q->front;
  result = l->head;
  q->front = q->front->tail;
  free(l);
  return result;
}

int objqueue_isempty(ObjQueue *q) {
  return q->front == NULL;
}

void objqueue_freeall(ObjQueue *q) {
  while (!objqueue_isempty(q))
    free(objqueue_remove(q));
  free(q);
}

void objqueue_free(ObjQueue *q) {
  while (!objqueue_isempty(q))
    objqueue_remove(q);
  free(q);
}
