#ifndef ASSERT_H
#define ASSERT_H
#include "../emu/emu_config.h"
#ifdef EMU 
#include <assert.h>
#else
#define ASSERT
#ifdef ASSERT
#define __static_assert(con, id) static int assert_ ## id [2 * !!(con) - 1] __attribute__((unused));
#define _static_assert(con, id) __static_assert(con, id) //expanding any macro that could be in `id` argument
#define static_assert(con) _static_assert(con, __LINE__)
#endif
#endif
#endif
