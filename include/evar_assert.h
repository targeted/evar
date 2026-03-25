#ifndef EVAR_ASSERT_H
#define EVAR_ASSERT_H

#include <evar_config.h>

/*
 * Runtime assert with device-specific crash as a result.
 * For non-debug assertions, the tasks should use `if (!ok) evar__crash(...);`
 */
#ifdef EVAR_DEBUG
#define evar_assert(cond) if (!(cond)) evar_device__crash(0xffff, "assertion failed at " __FILE__ ":" EVAR_QUOTE(__LINE__))
#else
#define evar_assert(cond)
#endif

#endif
