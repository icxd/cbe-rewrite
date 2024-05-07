#ifndef CBE_H
#define CBE_H

#include "cbe_register.h"
#include "cbe_types.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>

typedef size_t cbe_symbol_id;
typedef slice(const char *) cbe_symbol_table;

enum cbe_value_tag {
  CBE_VALUE_INTEGER,
  CBE_VALUE_FLOATING,
  CBE_VALUE_STRING,
  CBE_VALUE_CHARACTER,
  CBE_VALUE_LOCAL,
  CBE_VALUE_GLOBAL,
};
struct cbe_value {
  enum cbe_value_tag tag;
  union {
    int64_t integer;
    double floating;
    const char *string;
    char character;
    cbe_symbol_id global, local;
  };
};

typedef size_t cbe_type_id;
enum cbe_type_tag {
  CBE_TYPE_INT,
};
struct cbe_type {
  enum cbe_type_tag tag;
  union {
    struct { /* i/u<size> */
      uint size;
      bool unsigned_;
    } integer;
  };
};

struct cbe_typed_value {
  struct cbe_type type;
  struct cbe_value value;
};

enum cbe_instruction_tag {
  CBE_INST_ADD,
};
struct cbe_instruction {
  enum cbe_instruction_tag tag;
  union {
    struct {
      struct cbe_typed_value left, right;
    } add;
  };
};

struct cbe_function {
  const char *name;

  slice(struct cbe_instruction) instructions;
  size_t ip;

  slice(const char *) labels;
  // slice(cbe_symbol_id) locals;
};

struct cbe_global_variable {
  cbe_symbol_id symbol_id;
  bool constant;
  struct cbe_typed_value value;
};

struct cbe_context {
  slice(struct cbe_global_variable) global_variables;

  slice(struct cbe_function) functions;
  // -1 means no function, meaning the global scope.
  int current_function_index;

  cbe_symbol_table symbol_table;

  struct cbe_register_pool register_pool;
  cbe_live_intervals live_intervals, active_intervals;
  int current_stack_location;
};

struct cbe_module {
  struct cbe_context *context;
  char *text, *data, *rodata, *bss;
};

/* --------------- CONTEXT FUNCTIONS --------------- */

void cbe_context_init(struct cbe_context *);
void cbe_context_free(struct cbe_context *);

// Returns `SIZE_MAX` if symbol wasn't found, otherwise returns the symbol id.
cbe_symbol_id cbe_context_find_symbol(struct cbe_context *, const char *);
// Returns the added symbol's id if it isn't already defines. Otherwise returns
// `SIZE_MAX`
cbe_symbol_id cbe_context_add_symbol(struct cbe_context *, const char *);
cbe_symbol_id cbe_context_find_or_add_symbol(struct cbe_context *,
                                             const char *);

void cbe_context_build_global_variable(struct cbe_context *, const char *, bool,
                                       struct cbe_typed_value);

void cbe_context_build_function(struct cbe_context *, const char *);
void cbe_context_finish_current_function(struct cbe_context *);

size_t cbe_context_build_label(struct cbe_context *, const char *);

enum cbe_register cbe_context_get_register(struct cbe_context *);
void cbe_context_free_register(struct cbe_context *, enum cbe_register);
bool cbe_context_register_pool_is_empty(struct cbe_context *);

void cbe_context_expire_old_intervals(struct cbe_context *,
                                      struct cbe_live_interval *);
void cbe_context_spill_at_interval(struct cbe_context *,
                                   struct cbe_live_interval *);

/* --------------- MODULE FUNCTIONS --------------- */

void cbe_module_init(struct cbe_module *, struct cbe_context *);
void cbe_module_free(struct cbe_module *);

void cbe_module_generate(struct cbe_module *);

void cbe_module_generate_global_variable(struct cbe_module *,
                                         struct cbe_global_variable);

char *cbe_module_generate_typed_value(struct cbe_module *,
                                      struct cbe_typed_value);

char *cbe_module_generate_value(struct cbe_module *, struct cbe_value);
char *cbe_module_generate_type(struct cbe_module *, struct cbe_type);

void cbe_module_output_to_file(struct cbe_module *, FILE *);

/* --------------- GENERAL FUNCTIONS --------------- */

struct cbe_typed_value cbe_build_typed_value(struct cbe_type, struct cbe_value);

struct cbe_type cbe_build_type_int(uint);
struct cbe_type cbe_build_type_unsigned_int(uint);

struct cbe_value cbe_build_value_integer(int64_t);
struct cbe_value cbe_build_value_float(double);
struct cbe_value cbe_build_value_string(const char *);
struct cbe_value cbe_build_value_character(char);
struct cbe_value cbe_build_value_local(struct cbe_context *, const char *);
struct cbe_value cbe_build_value_global(struct cbe_context *, const char *);

#endif // CBE_H
