#ifndef CBE_TYPES_H
#define CBE_TYPES_H

#define CBE_ARRAY_LEN(a) (sizeof(a) / sizeof(*a))

#define CBE_PRINT_ERROR(fmt, ...)                                              \
  do {                                                                         \
    CBE_ERROR(fmt, __VA_ARGS__);                                               \
    exit(1);                                                                   \
  } while (0)

/* --------------- SLICES --------------- */

#define slice(T)                                                               \
  struct {                                                                     \
    T *items;                                                                  \
    size_t cap, size;                                                          \
  }

#define slice_init(s)                                                          \
  do {                                                                         \
    (s)->items = (__typeof__(*(s)->items) *)malloc(sizeof(*(s)->items) * 256); \
    (s)->cap = 256;                                                            \
    (s)->size = 0;                                                             \
  } while (0)

#define slice_free(s)                                                          \
  do {                                                                         \
    free((s)->items);                                                          \
  } while (0)

#define slice_push(s, ...)                                                     \
  do {                                                                         \
    if ((s)->size >= (s)->cap) {                                               \
      (s)->cap *= 2;                                                           \
      (s)->items = (__typeof__(*(s)->items) *)realloc(                         \
          (__typeof__(*(s)->items) *)(s)->items, sizeof(*(s)->items) * 256);   \
    }                                                                          \
    (s)->items[(s)->size++] = (__VA_ARGS__);                                   \
  } while (0)

// If `_end` is -1, then it will set the end to the current size of the slice,
// meaning that you can slice of the start of the slice with a bit nicer syntax.
//
// ## Example
// ```c
// // This will cut off the first element in `my_slice`.
// slice_sub(&my_slice, 1, -1);
// ```
#define slice_sub(s, _start, _end)                                             \
  do {                                                                         \
    size_t end = (_end) == -1 ? (s)->size : (_end);                            \
    size_t size = (end - _start + 1);                                          \
    char *src = (char *)(s)->items + _start * sizeof(*(s)->items);             \
    memcpy((s)->items, src, size * sizeof(*(s)->items));                       \
    (s)->size = size;                                                          \
  } while (0)

#define slice_ensure_cap(s, c)                                                 \
  do {                                                                         \
    (s)->cap = (c);                                                            \
  } while (0)

#endif // CBE_TYPES_H
