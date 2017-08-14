#pragma once

// Macros
#define STRING_JOIN(x, y) STRING_JOIN2(x, y)
#define STRING_JOIN2(x, y) x##y

#define log_print(category, format, ...)                   \
    {                                                      \
            char __lp_message [2048];                      \
            sprintf(__lp_message, format, __VA_ARGS__);    \
            printf("[%s]: %s\n",  category, __lp_message); \
    }


#define array_size(array)  (sizeof(array)/sizeof(array[0]))

#define DLLIMPORT __declspec(dllimport)
#define DLLEXPORT __declspec(dllexport)

#define VNAME(x) #x

// Runs code at scope exit
template <typename F>
struct _ScopeExit {
    _ScopeExit(F f) : f(f) {}
    ~_ScopeExit() { f(); }
    F f;
};

template <typename F>
_ScopeExit<F> _MakeScopeExit(F f) { return _ScopeExit<F>(f); };

#define scope_exit(code)                                                     \
    auto STRING_JOIN(_scope_exit_, __LINE__) = _MakeScopeExit([=](){code;})


#ifdef PERF_MON
    // @Temporary In the future, instead of log_printing, add the value to some array so that it is
    // displayed in a game overlay, or maybe as an overview when the game is closed.
    #define perf_monitor()                                                                       \
        double __perf_mon_begin = os_specific_get_time();                                        \
        scope_exit(                                                                              \
            double __perf_mon_end = os_specific_get_time();                                      \
            char * __perf_mon_file = strdup(__FILE__);                                           \
            char * __perf_mon_func = strdup(__FUNCTION__);                                       \
            char * __perf_mon_cursor = __perf_mon_func;                                          \
            while(*__perf_mon_cursor != ':' && *__perf_mon_cursor != '0') {                      \
                __perf_mon_cursor++;                                                             \
            }                                                                                    \
            *__perf_mon_cursor = '\0';                                                           \
            log_print("perf_monitor", "Function %s in %s took %0.6f ms to complete",             \
                      __perf_mon_func, __perf_mon_file + 7, /* +7 is there to remove "../src" */ \
                      (__perf_mon_end - __perf_mon_begin) * 1000);                               \
            free(__perf_mon_func);                                                               \
            free(__perf_mon_file);                                                               \
    )
#else
    #define perf_monitor()
#endif
