/* Desktop stand-in for <android/log.h>.
 *
 * The SP bridge and the integration layer log through __android_log_print/__android_log_vprint.
 * On Android those land in logcat; here they land on stderr with the same signature and the same
 * priority constants, so the port's own logging code is unchanged. This is the whole of the
 * Android platform layer that has to be replaced for the engine to run on a desktop.
 */
#ifndef LONGWAYHOME_ANDROID_LOG_H
#define LONGWAYHOME_ANDROID_LOG_H

#include <stdarg.h>
#include <stdio.h>

typedef enum android_LogPriority {
  ANDROID_LOG_UNKNOWN = 0,
  ANDROID_LOG_DEFAULT,
  ANDROID_LOG_VERBOSE,
  ANDROID_LOG_DEBUG,
  ANDROID_LOG_INFO,
  ANDROID_LOG_WARN,
  ANDROID_LOG_ERROR,
  ANDROID_LOG_FATAL,
  ANDROID_LOG_SILENT
} android_LogPriority;

/* Names the Android runtime provides as macros in some NDK versions. */
#ifndef ANDROID_LOG_TAGS
#define ANDROID_LOG_TAGS ""
#endif

static inline int __android_log_print(int prio, const char *tag, const char *fmt, ...)
{
  static const char *names[] = { "?", "?", "V", "D", "I", "W", "E", "F", "S" };
  const char *level = (prio >= 0 && prio <= ANDROID_LOG_SILENT) ? names[prio] : "?";
  va_list ap;
  int n = fprintf(stderr, "[%s] %s: ", level, tag ? tag : "-");
  va_start(ap, fmt);
  n += vfprintf(stderr, fmt, ap);
  va_end(ap);
  fflush(stderr);
  return n;
}

static inline int __android_log_vprint(int prio, const char *tag, const char *fmt, va_list ap)
{
  (void)prio;
  int n = fprintf(stderr, "%s: ", tag ? tag : "-");
  n += vfprintf(stderr, fmt, ap);
  fflush(stderr);
  return n;
}

static inline int __android_log_write(int prio, const char *tag, const char *text)
{
  return __android_log_print(prio, tag, "%s", text);
}

#endif /* LONGWAYHOME_ANDROID_LOG_H */
