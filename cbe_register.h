#ifndef CBE_REGISTER_H
#define CBE_REGISTER_H

#include "cbe_types.h"
#include <stddef.h>

enum cbe_register {
  CBE_REG_NONE,

  CBE_REG_EAX,
  CBE_REG_EBX,
  CBE_REG_ECX,
  CBE_REG_EDX,
  CBE_REG_ESI,
  CBE_REG_EDI,
  CBE_REG_EBP,
  CBE_REG_ESP,

  CBE_REG_COUNT,
};

struct cbe_register_symbol {
  const char *name;
  enum cbe_register reg;
  int location;
};

struct cbe_live_interval {
  struct cbe_register_symbol symbol;
  int location;
  int start_point, end_point;
};
typedef slice(struct cbe_live_interval) cbe_live_intervals;

struct cbe_register_pool {
  size_t head;
  enum cbe_register registers[CBE_REG_COUNT];
  size_t registers_count;
};

int cbe_sort_intervals_by_start_point(const void *, const void *);
int cbe_sort_intervals_by_end_point(const void *, const void *);

void cbe_delete_interval(cbe_live_intervals *, size_t);

const char *cbe_get_register_name(enum cbe_register);

#endif // CBE_REGISTER_H
