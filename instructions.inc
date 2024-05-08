//
// INST(
//   uppercase_name,
//   name,
//   expects_temporary,
//   ...
// )
//
INST(ADD, add, true, { struct cbe_typed_value left, right; })
INST(SUB, sub, true, { struct cbe_typed_value left, right; })
INST(MUL, mul, true, { struct cbe_typed_value left, right; })
INST(DIV, div, true, { struct cbe_typed_value left, right; })
INST(MOD, mod, true, { struct cbe_typed_value left, right; })
INST(REM, rem, true, { struct cbe_typed_value left, right; })

