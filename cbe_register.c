#include "cbe_register.h"
#include "cbe_slice.h"
#include <stdio.h>
#include <stdlib.h>

int cbe_sort_intervals_by_start_point(const void *a, const void *b) {
  return (*(struct cbe_live_interval *)a).start_point <
         (*(struct cbe_live_interval *)b).start_point;
}

int cbe_sort_intervals_by_end_point(const void *a, const void *b) {
  return (*(struct cbe_live_interval *)a).end_point <
         (*(struct cbe_live_interval *)b).end_point;
}

void cbe_delete_interval(cbe_live_intervals *intervals, size_t index) {
  cbe_live_intervals new_intervals;
  slice_init(&new_intervals);
  slice_ensure_cap(&new_intervals, intervals->size);

  for (size_t i = 0; i < intervals->size; i++) {
    if (i == index)
      continue;
    slice_push(&new_intervals, intervals->items[i]);
  }

  intervals->items = new_intervals.items;
  intervals->size = new_intervals.size;
}

const char *cbe_get_register_name(enum cbe_register reg) {
  const char *names[] = {"none", "eax", "ebx", "ecx", "edx",
                         "esi",  "edi", "ebp", "esp"};
  return names[reg];
}
