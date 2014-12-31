
#ifndef NN_DEBUG_INCLUDED
#define NN_DEBUG_INCLUDED

#define LL_VERBOSE 1
#define LL_TRACE 5
#define LL_DEBUG 10
#define LL_DETAIL 20
#define LL_INFO 30
#define LL_WARN 40
#define LL_ERROR 50
#define LL_FATAL 60


void _uv_init_debug_inner(int level);

void debug_print(int level, const char *fmt, ...);

#endif

