#pragma once

// Macros
#define STRING_JOIN2(x, y) x##y


#define log_print(category, format, ...)              \
    {                                                 \
            char message [2048];                      \
            sprintf(message, format, __VA_ARGS__);    \
            printf("[%s]: %s\n",  category, message); \
    }                                                 \


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

#define scope_exit(code) \
    auto STRING_JOIN2(_scope_exit_, __LINE__) = _MakeScopeExit([=](){code;})

