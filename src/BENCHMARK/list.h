#ifndef LIST_H
#define LIST_H

/************************************************ List utils **************************************************/
typedef struct list{
  void **  cell;
  unsigned length;
  unsigned allocated_length;
  int      is_sublist;
  void     (*delete_element)(void*);
} * list;

list         new_list          (size_t elem_size, unsigned max_elem, void (*delete_element)(void*));
list         sub_list          (list, int start_index, unsigned length);
list         list_dup          (list);
void         delete_list       (list);
void         empty_list        (list);
unsigned     list_length       (list);
void *       list_get          (list, unsigned);
void **      list_get_data     (list list);
void *       list_set          (list, unsigned, void *);
void *       list_pop          (list);
void         list_push         (list, void *);
void *       list_remove       (list, int);
void         list_insert       (list, unsigned, void *);
unsigned     list_insert_sorted(list list, void * element, int (* compare)(const void*, const void*));
void         list_sort         (list list, int (* compare)(const void*, const void*));
int          list_find         (list list, void * key, int (* compare)(const void*, const void*));
int          list_find_unsorted(list list, void * key);
void         list_reduce       (list l, void * result, void * reduction(void * a, void * b));
char **      list_to_char(list);

#define      VA_ARGS(...) , ##__VA_ARGS__
#define      list_apply(l, func, ...) do{unsigned i; for(i=0;i<l->length;i++){func(l->cell[i] VA_ARGS(__VA_ARGS__));}} while(0)
/***************************************************************************************************************/

#endif

