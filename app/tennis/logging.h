#ifndef LOGGING_H_
#define LOGGING_H_

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

void logg(const char* prefix, const char* file, const char* function, uint32_t line, const char* format, ...);

#define LOG_LEVEL 4


#if LOG_LEVEL > 3
#define LOGTRACE(format, ...) logg("TRACE", __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#else
#define LOGTRACE(format, ...)
#endif

#if LOG_LEVEL > 2
#define LOGINFO(format, ...) logg("INFO", __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#else
#define LOGINFO(format, ...)
#endif

#if LOG_LEVEL > 1
#define LOGTEST(format, ...) logg("TEST", __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#else
#define LOGTEST(format, ...)
#endif

#if LOG_LEVEL > 0
#define LOGWARN(format, ...) logg("WARN", __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#else
#define LOGWARN(format, ...)
#endif

#if LOG_LEVEL > -1
#define LOGERROR(format, ...) logg("ERROR", __FILE__, __FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#else
#define LOGERROR(format, ...)
#endif

#ifdef __cplusplus
}
#endif

#endif // LOGGING_H_