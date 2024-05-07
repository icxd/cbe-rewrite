#include "cbe.h"
#include "cbe_log.h"
#include "cbe_register.h"
#include "cbe_types.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* --------------- CONTEXT FUNCTIONS --------------- */

void cbe_context_init(struct cbe_context *context) {
  slice_init(&context->global_variables);

  slice_init(&context->functions);
  context->current_function_index = -1;

  slice_init(&context->symbol_table);

  context->register_pool = (struct cbe_register_pool){
      0,
      {CBE_REG_EAX, CBE_REG_EBX, CBE_REG_ECX, CBE_REG_EDX, CBE_REG_ESI,
       CBE_REG_EDI, CBE_REG_EBP, CBE_REG_ESP},
      8};
  slice_init(&context->live_intervals);
  slice_init(&context->active_intervals);
  context->current_stack_location = 0;
}

void cbe_context_free(struct cbe_context *context) {
  slice_free(&context->global_variables);
  slice_free(&context->symbol_table);
  slice_free(&context->live_intervals);
  slice_free(&context->active_intervals);
}

cbe_symbol_id cbe_context_find_symbol(struct cbe_context *context,
                                      const char *symbol) {
  for (size_t i = 0; i < context->symbol_table.size; i++) {
    const char *current = context->symbol_table.items[i];
    if (strcmp(symbol, current) == 0)
      return i;
  }
  return SIZE_MAX;
}

cbe_symbol_id cbe_context_add_symbol(struct cbe_context *context,
                                     const char *symbol) {
  // This makes sure that the specified symbol is undefined.
  if (cbe_context_find_symbol(context, symbol) != SIZE_MAX) {
    return SIZE_MAX;
  }
  slice_push(&context->symbol_table, symbol);
  return context->symbol_table.size - 1;
}

cbe_symbol_id cbe_context_find_or_add_symbol(struct cbe_context *context,
                                             const char *symbol) {
  cbe_symbol_id symbol_id = cbe_context_find_symbol(context, symbol);
  if (symbol_id == SIZE_MAX)
    symbol_id = cbe_context_add_symbol(context, symbol);
  return symbol_id;
}

void cbe_context_build_global_variable(struct cbe_context *context,
                                       const char *name, bool constant,
                                       struct cbe_typed_value value) {
  cbe_symbol_id symbol_id = cbe_context_add_symbol(context, name);
  if (symbol_id == SIZE_MAX)
    CBE_PRINT_ERROR("redefinition of global symbol `%s`", name);
  slice_push(&context->global_variables, (struct cbe_global_variable){
                                             symbol_id,
                                             constant,
                                             value,
                                         });
}

void cbe_context_build_function(struct cbe_context *context, const char *name) {
  CBE_ASSERT(context->current_function_index == -1);

  struct cbe_function fn;
  fn.name = name;
  slice_init(&fn.instructions);
  fn.ip = 0;
  slice_init(&fn.labels);
  // slice_init(&fn.locals);

  slice_push(&context->functions, fn);
  context->current_function_index = context->functions.size - 1;
}

void cbe_context_finish_current_function(struct cbe_context *context) {
  CBE_ASSERT(context->current_function_index != -1);
  context->current_function_index = -1;
}

size_t cbe_context_build_label(struct cbe_context *context, const char *label) {
  CBE_ASSERT(context->current_function_index != -1);
  slice_push(&context->functions.items[context->current_function_index].labels,
             label);
  return context->functions.items[context->current_function_index].labels.size -
         1;
}

static void slice_c_array(void *arr, size_t elem_size, int start, int end) {
  int size = (end - start + 1) * elem_size;

  char *src = (char *)arr + start * elem_size;
  memcpy(arr, src, size);
}

enum cbe_register cbe_context_get_register(struct cbe_context *context) {
  if (context->register_pool.registers_count == 0)
    return CBE_REG_NONE;

  enum cbe_register reg = context->register_pool.registers[0];
  slice_c_array(&context->register_pool.registers, sizeof(enum cbe_register), 1,
                CBE_ARRAY_LEN(context->register_pool.registers));
  context->register_pool.registers_count--;

  return reg;
}

void cbe_context_free_register(struct cbe_context *context,
                               enum cbe_register reg) {
  for (size_t i = 0; i < CBE_ARRAY_LEN(context->register_pool.registers); i++) {
    if (context->register_pool.registers[i] == reg)
      return;
  }

  context->register_pool.registers[context->register_pool.registers_count++] =
      reg;
}

bool cbe_context_register_pool_is_empty(struct cbe_context *context) {
  return context->register_pool.registers_count == 0;
}

void cbe_context_expire_old_intervals(struct cbe_context *context,
                                      struct cbe_live_interval *interval) {
  // qsort(context->active_intervals.items, context->active_intervals.size,
  //       sizeof(struct cbe_live_interval), cbe_sort_intervals_by_end_point);
  for (size_t i = 0; i < context->active_intervals.size; i++) {
    struct cbe_live_interval active_interval =
        context->active_intervals.items[i];
    if (active_interval.end_point >= interval->start_point)
      break;
    cbe_context_free_register(context, active_interval.symbol.reg);
    cbe_delete_interval(&context->active_intervals, i);
    break;
  }
}

void cbe_context_spill_at_interval(struct cbe_context *context,
                                   struct cbe_live_interval *interval) {
  // qsort(context->active_intervals.items, context->active_intervals.size,
  //       sizeof(struct cbe_live_interval), cbe_sort_intervals_by_end_point);
  struct cbe_live_interval spill =
      context->active_intervals.items[context->active_intervals.size - 1];
  if (spill.end_point > interval->end_point) {
    CBE_DEBUG("ACTION: Spill interval (%p)", (void *)&spill);
    CBE_DEBUG("ACTION: Allocate register %s (%d) to interval (%p)",
              cbe_get_register_name(spill.symbol.reg), spill.symbol.reg,
              (void *)interval);
    interval->symbol.reg = spill.symbol.reg;
    spill.symbol.location = context->current_stack_location;
    cbe_delete_interval(&context->active_intervals,
                        context->active_intervals.size - 1);
  } else {
    CBE_DEBUG("ACTION: Spill interval (%p)", (void *)interval);
    interval->symbol.location = context->current_stack_location;
  }
  context->current_stack_location += 4;
}

/* --------------- MODULE FUNCTIONS --------------- */

void cbe_module_init(struct cbe_module *module, struct cbe_context *context) {
  module->context = context;
  // I think ~64MiB is enough for each one, might change up some of the sizes
  // later though.
  module->text = (char *)malloc(64 * 1024);
  module->data = (char *)malloc(64 * 1024);
  module->rodata = (char *)malloc(64 * 1024);
  module->bss = (char *)malloc(64 * 1024);
}

void cbe_module_free(struct cbe_module *module) {
  free((void *)module->text);
  free((void *)module->data);
  free((void *)module->rodata);
  free((void *)module->bss);
}

void cbe_module_generate(struct cbe_module *module) {
  for (size_t i = 0; i < module->context->global_variables.size; i++) {
    cbe_module_generate_global_variable(
        module, module->context->global_variables.items[i]);
  }
}

void cbe_module_generate_global_variable(struct cbe_module *module,
                                         struct cbe_global_variable variable) {
  char *value = cbe_module_generate_typed_value(module, variable.value);
  sprintf(variable.constant ? module->rodata : module->data,
          "%sglobal__%ld: dw %s\n",
          variable.constant ? module->rodata : module->data, variable.symbol_id,
          value);
  free(value);
}

char *cbe_module_generate_typed_value(struct cbe_module *module,
                                      struct cbe_typed_value typed_value) {
  return cbe_module_generate_value(module, typed_value.value);
}

char *cbe_module_generate_value(struct cbe_module *module,
                                struct cbe_value value) {
  (void)module;
  char *buffer = (char *)malloc(256); // should be enough for now
  switch (value.tag) {
  case CBE_VALUE_INTEGER:
    sprintf(buffer, "%ld", value.integer);
    break;

  case CBE_VALUE_FLOATING:
    CBE_PRINT_ERROR("floating point values are not supported yet.");

  case CBE_VALUE_STRING:
    sprintf(buffer, "");
    for (size_t i = 0; i < strlen(value.string); i++)
      sprintf(buffer, "%s0x%02x, ", buffer, value.string[i]);
    sprintf(buffer, "%s0x00", buffer);
    break;

  case CBE_VALUE_CHARACTER:
    sprintf(buffer, "0x%02x", value.character);
    break;

  case CBE_VALUE_GLOBAL:
    sprintf(buffer, "global__%ld", value.global);
    break;

  default:
    CBE_PRINT_ERROR("not implemented");
  }
  return buffer;
}

char *cbe_module_generate_type(struct cbe_module *module,
                               struct cbe_type type) {
  (void)module;
  (void)type;
  CBE_WARN("`cbe_module_generate_type` is currently unused.");
  return NULL;
}

void cbe_module_output_to_file(struct cbe_module *module, FILE *fp) {
  fprintf(fp, "section .text\n");
  fprintf(fp, "%s\n", module->text);

  fprintf(fp, "section .data\n");
  fprintf(fp, "%s\n", module->data);

  fprintf(fp, "section .rodata\n");
  fprintf(fp, "%s\n", module->rodata);

  fprintf(fp, "section .bss\n");
  fprintf(fp, "%s\n", module->bss);
}

/* --------------- GENERAL FUNCTIONS --------------- */

struct cbe_typed_value cbe_build_typed_value(struct cbe_type type,
                                             struct cbe_value value) {
  return (struct cbe_typed_value){type, value};
}

struct cbe_type cbe_build_type_int(uint size) {
  return (struct cbe_type){.tag = CBE_TYPE_INT, .integer = {size, false}};
}

struct cbe_type cbe_build_type_unsigned_int(uint size) {
  return (struct cbe_type){.tag = CBE_TYPE_INT, .integer = {size, true}};
}

struct cbe_value cbe_build_value_integer(int64_t value) {
  return (struct cbe_value){.tag = CBE_VALUE_INTEGER, .integer = value};
}

struct cbe_value cbe_build_value_float(double value) {
  return (struct cbe_value){.tag = CBE_VALUE_FLOATING, .floating = value};
}

struct cbe_value cbe_build_value_string(const char *value) {
  return (struct cbe_value){.tag = CBE_VALUE_STRING, .string = value};
}

struct cbe_value cbe_build_value_character(char value) {
  return (struct cbe_value){.tag = CBE_VALUE_CHARACTER, .character = value};
}

struct cbe_value cbe_build_value_local(struct cbe_context *context,
                                       const char *value) {
  return (struct cbe_value){
      .tag = CBE_VALUE_LOCAL,
      .local = cbe_context_find_or_add_symbol(context, value),
  };
}

struct cbe_value cbe_build_value_global(struct cbe_context *context,
                                        const char *value) {
  return (struct cbe_value){
      .tag = CBE_VALUE_GLOBAL,
      .global = cbe_context_find_or_add_symbol(context, value),
  };
}
