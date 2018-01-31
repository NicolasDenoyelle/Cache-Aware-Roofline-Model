#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include "./list.h"

#define malloc_chk(ptr, size) do{					\
    ptr = malloc(size);							\
    if(ptr == NULL){							\
      perror("malloc");							\
      exit(EXIT_FAILURE);}} while(0)

#define realloc_chk(ptr, size) do{		\
    if((ptr = realloc(ptr,size)) == NULL){	\
      perror("realloc");			\
      exit(EXIT_FAILURE);}} while(0)

/* An list of objects */
list 
new_list(size_t elem_size, unsigned max_elem, void (*delete_element)(void*)){
  unsigned i;
  list l;
  malloc_chk(l, sizeof(*l));
  malloc_chk(l->cell, elem_size*max_elem);
  for(i=0;i<max_elem;i++){
    l->cell[i] = NULL;
  }
  l->length = 0;
  l->allocated_length = max_elem;
  l->delete_element = delete_element;
  l->is_sublist = 0;
  return l;
}

list sub_list(list l, int start_index, unsigned length){
  list sub;
  malloc_chk(sub, sizeof(*sub));
  sub->allocated_length = l->allocated_length-start_index;
  sub->length = length;
  sub->cell = &(l->cell[start_index]);
  sub->delete_element = NULL;
  sub->is_sublist = 1;  
  return sub;
}

list list_dup(list l)
{
  unsigned i;
  list copy;
  copy = new_list(sizeof(*(l->cell)), l->allocated_length, l->delete_element);
  memcpy(copy->cell, l->cell,sizeof(*(l->cell))*l->length);
  copy->length = l->length;
  for(i=copy->length; i<l->allocated_length; i++)
    copy->cell[i] = NULL;
  return copy;
}

char ** list_to_char(list l){
  unsigned i;
  char ** ret;
  malloc_chk(ret, sizeof(*ret) * l->length);
  for(i=0;i<l->length; i++){ret[i] = (char*)list_get(l,i);}
  return ret;
}

void 
delete_list(list l){
  unsigned i;
  if(l==NULL) return;
  if(!l->is_sublist){
    if(l->delete_element!=NULL){
      for(i=0;i<l->length;i++){
	if(l->cell[i]!=NULL)
	  l->delete_element(l->cell[i]);
      }
    }
    free(l->cell);
  }
  free(l);
}

void 
empty_list(list l){
  unsigned i;
  if(l==NULL)
    return;
  if(l->delete_element!=NULL){
    for(i=0;i<l->length;i++){
      if(l->cell[i]!=NULL)
	l->delete_element(l->cell[i]);
    }
  }
  l->length = 0;
}

static void list_chk_length(list l, unsigned length){
  if(length>=l->allocated_length){
    while((l->allocated_length*=2)<length);
    realloc_chk(l->cell,sizeof(*(l->cell))*l->allocated_length);
    memset(&(l->cell[l->length]), 0, sizeof(*l->cell) * (l->allocated_length-l->length));
  }
}

void * list_get(list l, unsigned i){
  if(l==NULL){return NULL;}
  if(i>=l->length){return NULL;}
  else{return l->cell[i];}
}

void ** list_get_data(list l){
  return l->cell;
}

void * list_set(list l, unsigned i, void * element){
  void * ret;
  list_chk_length(l,i);
  ret = l->cell[i];
  l->cell[i] = element;
  if(l->length<=i){l->length = i+1;}
  return ret;
}

inline unsigned list_length(list l){
  return l->length;
}

inline void list_push(list l, void * element){
  list_set(l,l->length,element);
}

void * list_pop(list l){
  if(l->length == 0){return NULL;}
  l->length--;
  return l->cell[l->length];
}

void list_insert(list l, unsigned i, void * element){
  if(i>=l->length){return;}
  list_chk_length(l, l->length+1);
  l->length++;
  memmove(&l->cell[i+1], &l->cell[i], (l->length-i-1)*sizeof(*l->cell));
  l->cell[i] = element;
}

void * list_remove(list l, int i){
  void * ret;
  if(i<0 || (unsigned)i >= l->length){return NULL;}
  else{ret = l->cell[i];}
  if(l->length){
    memmove(&l->cell[i], &l->cell[i+1], (l->length-i-1)*sizeof(*l->cell));
    l->length--;
  }	
  return ret;
}

int list_find(list l, void * key, int (* compare)(const void*, const void*)){
  void* found = bsearch(&key,l->cell, l->length, sizeof(*(l->cell)), (__compar_fn_t)compare);
  if(found == NULL){return -1;}
  return ((intptr_t)found - (intptr_t)l->cell) / sizeof(*(l->cell));
}

int list_find_unsorted(list l, void * key){
  unsigned i;
  for(i = 0; i<list_length(l); i++){
    if(list_get(l, i) == key){return (int)i;}
  }
  return -1;
}


unsigned list_insert_sorted(list l, void * element, int (* compare)(const void*, const void*)){
  unsigned insert_index;
  unsigned left_bound = 0,right_bound = l->length-1;
  int comp;

  if(l->length == 0){list_push(l, element); return 0;}
  if(l->length == 1){
    insert_index = compare(&element, &l->cell[0]) > 0 ? 1 : 0;
    goto insertion;
  }
  
  insert_index = 1+left_bound + (right_bound-left_bound)/2;
  
  do{
    comp = compare(&element, &l->cell[insert_index]);
    if(comp > 0){left_bound = insert_index;}
    else if(comp < 0){right_bound = insert_index;}
    else{break;}
    insert_index = 1+left_bound + (right_bound-left_bound)/2;
  } while(insert_index < right_bound);

insertion:
  if(insert_index >= l->length){
    list_push(l, element);
    return l->length-1;
  }
  else{
    list_insert(l, insert_index, element);
    return insert_index;
  }
}

void list_reduce(list l, void * result, void * reduction(void * a, void * b)){
  if(l->length == 0){return;}
  unsigned i;
  for(i=0;i<l->length;i++){reduction(result, l->cell[i]);}
}

inline void list_sort(list l, int (* compare)(const void*, const void*)){
  qsort(l->cell, l->length, sizeof(*(l->cell)), (__compar_fn_t)compare);
}

