/* list.h */

#ifndef _LIST_H_
#define _LIST_H_

#include "scdef.h"

typedef struct List {
  long head;
  struct List *tail;
} List;

typedef struct ObjList {
  void *head;
  struct ObjList *tail;
} ObjList;

typedef struct ObjListEnum {
  ObjList *next;
} ObjListEnum;

extern List *list_new();
extern List *list_insert(List *list, long item);
extern int list_find(List *list, long item);
extern List *list_remove(List *list, long item);
extern int list_length(const List *list);
extern void list_free(List *list);

extern ObjList *objlist_new();
extern int objlist_isempty(ObjList *list);
extern ObjList *objlist_insert(ObjList *list, void *item);
extern void *objlist_remove(ObjList *list, void *item, 
			    int (*eq_fn)(void *, void *));
extern void *objlist_find(ObjList *list, void *item, 
			  int (*eq_fn)(void *, void *));
extern void *objlist_swap(ObjList *list, void *olditem, void *newitem, 
			  int (*eq_fn)(void *, void *));
extern int objlist_length(const ObjList *list);
extern void objlist_free(ObjList *list);
extern void objlist_freeall(ObjList *list);
extern void objlist_freeall(ObjList *list);
extern ObjListEnum objlistenum_create(ObjList *list);
extern int objlistenum_isempty(ObjListEnum *lenum);
extern void *objlistenum_next(ObjListEnum *lenum);

#endif
