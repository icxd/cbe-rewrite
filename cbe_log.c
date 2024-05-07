#include "cbe_log.h"
#include <stdarg.h>
#include <stdio.h>

static int colors[] = {34, 32, 37, 33, 31, 35};
static const char *names[] = {"DEBUG", "INFO",  "TRACE",
                              "WARN",  "ERROR", "FATAL"};

void cbe_log(enum cbe_log_type type, const char *file, int line,
             const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  int color = colors[type];
  const char *name = names[type];

  if (type != CBE_LOG_ERROR && type != CBE_LOG_WARN)
    printf("\033[30;1m(%s:%d) ", file, line);
  printf("\033[%d;1m%s: \033[0;0m", color, name);
  vprintf(fmt, ap);
  printf("\n");

  va_end(ap);
}
