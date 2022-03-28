/*
  IceCC. These are generic linked-list routines.
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

/* $Id: list.c,v 1.8 2001/01/14 05:30:46 jp Exp $ */

#include <stdlib.h>
#include "scdef.h"
#include "list.h"

/* some basic routines to work with linked-lists */

/* ----- numeric lists ----- */

/* empty list is null */
List *list_new() {
  return NULL;
}

/* remember, new list head is returned! */
List *list_insert(List *list, long item) {
  List *newnode = calloc(1, sizeof(List));

  /* empty list is NULL */
  if (list == NULL) {
    newnode->head = item;
    return newnode;
  }

  /* else switch the first head with the new one */
  newnode->head = list->head;
  list->head = item;
  newnode->tail = list->tail;
  list->tail = newnode;
  return list;
}

/* removes the first occurance of item in list,
   returns the new head of the list (may not be
   the same as before, so reassign it) */
List *list_remove(List *list, long item) {
  List *prev = NULL, *head = list;

  if (list == NULL)
    return NULL;

  while (list != NULL) {
    if (list->head == item) {
      if (prev == NULL) {
	head = list->tail;
	free(list);
	return head;
      }
      prev->tail = list->tail;
      free(list);
      return head;
    }
    prev = list;
    list = list->tail;
  }
  return head;
}

/* returns true if item is in the list */
int list_find(List *list, long item) {
  if (list == NULL)
    return 0;

  while (list != NULL) {
    if (list->head == item)
      return 1;
    list = list->tail;
  }
  return 0;
}

int list_length(const List *list) {
  if (list == NULL)
    return 0;
  return 1 + list_length(list->tail);
}

void list_free(List *list) {
  if (list == NULL) return;
  list_free(list->tail);
  free(list);
}

/* ----- objlist routines ----- */

ObjList *objlist_new() {
  return calloc(1, sizeof(ObjList));
}

int objlist_isempty(ObjList *list) {
  return list->head == NULL && list->tail == NULL;
}

/* add a new item in front, returns pointer
   to the front (as of now, it does not change
   from insert to insert */
ObjList *objlist_insert(ObjList *list, void *item) {
  ObjList *newnode;

  /* if list is empty it has a sentinel node, 
     fill the sentinel node */
  if (list->head == NULL) {
    list->head = item;
    return list;
  }

  newnode = malloc(sizeof(ObjList));
  /* switch the first head with the new one */
  newnode->head = list->head;
  list->head = item;
  newnode->tail = list->tail;
  list->tail = newnode;
  return list;
}

/* This method is kinda messy because the objlist interface
   I made is poorly designed. Oh well. :P -- only removes the
   first occurance of item in list. Use the find function to
   see if anymore still exist */
void *objlist_remove(ObjList *list, void *item, 
		     int (*eq_fn)(void *, void *)) {
  ObjList *prev = NULL;

  if (list == NULL || list->head == NULL)
    return NULL;

  while (list != NULL) {
    if (eq_fn(list->head, item)) {
      void *result = list->head;
      /* if its the first item ... */
      if (prev == NULL) {
	/* only node in list, make it an empty sentinel node */
	if (list->tail == NULL)
	  list->head = NULL;
	/* more nodes in list, move the obj in the next node to
	   the front, free the next node, and reconnect this one
	   to the next */
	else {
	  ObjList *newtail = list->tail->tail;
	  list->head = list->tail->head;
	  free(list->tail);
	  list->tail = newtail;
	}
      }
      /* else not the first, so just remove this node
	 and reconnect the prev with the next one */
      else {
	prev->tail = list->tail;
	free(list);
      }
      return result;
    }
    prev = list;
    list = list->tail;
  }
  return NULL;
}

/* find the item in the list that is equal to
   item by eq_fn and return its pointer (so you
   can manipulate it) */
void *objlist_find(ObjList *list, void *item, 
		   int (*eq_fn)(void *, void *))
{
  if (list == NULL || list->head == NULL)
    return NULL;

  while (list != NULL) {
    if (eq_fn(list->head, item))
      return list->head;
    list = list->tail;
  }
  return NULL;
}

/* if we find olditem in the list, swap it with newitem,
   and return the pointer to the old one */
void *objlist_swap(ObjList *list, void *olditem, void *newitem, 
		   int (*eq_fn)(void *, void *))
{
  if (list == NULL || list->head == NULL)
    return NULL;
  
  while (list != NULL) {
    if (eq_fn(list->head, olditem)) {
      void *result = list->head;
      list->head = newitem;
      return result;
    }
    list = list->tail;
  }
  return NULL;
}

int objlist_length(const ObjList *list) {
  if (list == NULL || list->head == NULL)
    return 0;
  return 1 + objlist_length(list->tail);
}

void objlist_free(ObjList *list) {
  if (list == NULL) return;
  objlist_free(list->tail);
  free(list);
}

void objlist_freeall(ObjList *list) {
  if (list == NULL) return;
  objlist_freeall(list->tail);
  if (list->head != NULL) free(list->head);
  free(list);
}

/* not a pointer */
ObjListEnum objlistenum_create(ObjList *list) {
  ObjListEnum lenum;
  lenum.next = list;
  return lenum;
}

/* returns next object or NULL if empty */
void *objlistenum_next(ObjListEnum *lenum) {
  if (lenum->next == NULL || lenum->next->head == NULL)
    return NULL;
  else {
    void *result = lenum->next->head;
    lenum->next = lenum->next->tail;
    return result;
  }
}
