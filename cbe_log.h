#ifndef CBE_LOG_H
#define CBE_LOG_H

enum cbe_log_type {
  CBE_LOG_DEBUG,
  CBE_LOG_INFO,
  CBE_LOG_TRACE,
  CBE_LOG_WARN,
  CBE_LOG_ERROR,
  CBE_LOG_FATAL,
};

#define CBE_DEBUG(...) cbe_log(CBE_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define CBE_INFO(...) cbe_log(CBE_LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define CBE_TRACE(...) cbe_log(CBE_LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
#define CBE_WARN(...) cbe_log(CBE_LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define CBE_ERROR(...) cbe_log(CBE_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define CBE_FATAL(...) cbe_log(CBE_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

void cbe_log(enum cbe_log_type, const char *, int, const char *, ...);

#endif // CBE_LOG_H
