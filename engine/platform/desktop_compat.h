/* Force-included for the engine's desktop build.
 *
 * _GNU_SOURCE is needed before any system header: glibc hides dladdr() and Dl_info behind it,
 * while Android's bionic exposes them unconditionally. Costing the port one compile definition
 * beats editing the bridge's crash handler, which is not our code.
 */
#ifndef LONGWAYHOME_DESKTOP_COMPAT_H
#define LONGWAYHOME_DESKTOP_COMPAT_H

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1
#endif

#include <dlfcn.h>
#include <unwind.h>

#endif
