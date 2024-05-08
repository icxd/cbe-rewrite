#include "cbe.h"
#include "cbe_log.h"
#include "cbe_register.h"

int main(void) {
  struct cbe_context context;
  cbe_context_init(&context);

  cbe_context_build_global_variable(
      &context, "my_var1", false,
      cbe_build_typed_value(cbe_build_type_int(32),
                            cbe_build_value_integer(123)));
  // cbe_context_build_global_variable(
  //     &context, "my_var2", false,
  //     cbe_build_typed_value(cbe_build_type_unsigned_int(32),
  //                           cbe_build_value_float(123.456)));
  cbe_context_build_global_variable(
      &context, "my_var3", false,
      cbe_build_typed_value(cbe_build_type_unsigned_int(32),
                            cbe_build_value_string("this is a test")));
  cbe_context_build_global_variable(
      &context, "my_var4", false,
      cbe_build_typed_value(cbe_build_type_unsigned_int(32),
                            cbe_build_value_character('a')));
  cbe_context_build_global_variable(
      &context, "my_var5", false,
      cbe_build_typed_value(cbe_build_type_int(32),
                            cbe_build_value_global(&context, "my_var1")));

  cbe_context_build_function(&context, "main");

  cbe_context_build_label(&context, "entry");

  cbe_context_build_inst_add(
      &context,
      cbe_build_typed_value(cbe_build_type_int(32),
                            cbe_build_value_integer(50)),
      cbe_build_typed_value(cbe_build_type_int(32),
                            cbe_build_value_integer(100)));

  cbe_context_finish_current_function(&context);

  struct cbe_module module;
  cbe_module_init(&module, &context);

  cbe_module_generate(&module);
  cbe_module_output_to_file(&module, stdout);

  {
    //     slice_push(&context.live_intervals,
    //                (struct cbe_live_interval){
    //                    .symbol = {.name = "a", .reg = CBE_REG_NONE, .location
    //                    = -1}, .start_point = 1, .end_point = 4,
    //                });
    //     slice_push(&context.live_intervals,
    //                (struct cbe_live_interval){
    //                    .symbol = {.name = "b", .reg = CBE_REG_NONE, .location
    //                    = -1}, .start_point = 2, .end_point = 6,
    //                });
    //     slice_push(&context.live_intervals,
    //                (struct cbe_live_interval){
    //                    .symbol = {.name = "c", .reg = CBE_REG_NONE, .location
    //                    = -1}, .start_point = 3, .end_point = 10,
    //                });
    //     slice_push(&context.live_intervals,
    //                (struct cbe_live_interval){
    //                    .symbol = {.name = "d", .reg = CBE_REG_NONE, .location
    //                    = -1}, .start_point = 5, .end_point = 9,
    //                });
    //     slice_push(&context.live_intervals,
    //                (struct cbe_live_interval){
    //                    .symbol = {.name = "e", .reg = CBE_REG_NONE, .location
    //                    = -1}, .start_point = 7, .end_point = 8,
    //                });

    for (size_t i = 0; i < context.live_intervals.size; i++) {
      struct cbe_live_interval interval = context.live_intervals.items[i];
      CBE_DEBUG(
          "(%p) Symbol: %s | Location: %d | Start point: %d | End point: %d",
          (void *)&interval, interval.symbol.name, interval.symbol.location,
          interval.start_point, interval.end_point);

      cbe_context_expire_old_intervals(&context,
                                       &context.live_intervals.items[i]);

      if (cbe_context_register_pool_is_empty(&context)) {
        cbe_context_spill_at_interval(&context,
                                      &context.live_intervals.items[i]);
      } else {
        enum cbe_register reg = cbe_context_get_register(&context);
        CBE_DEBUG("ACTION: Allocate register %s (%d) to interval (%p)",
                  cbe_get_register_name(reg), reg, (void *)&interval);
        context.live_intervals.items[i].symbol.reg = reg;
        slice_push(&context.active_intervals, context.live_intervals.items[i]);
      }
    }

    for (size_t i = 0; i < context.live_intervals.size; i++) {
      struct cbe_live_interval interval = context.live_intervals.items[i];
      CBE_INFO("cbe_register_symbol{name: %s, register: %s, location: %d}",
               interval.symbol.name, cbe_get_register_name(interval.symbol.reg),
               interval.symbol.location);
    }
  }

  cbe_module_free(&module);
  cbe_context_free(&context);

  return 0;
}
