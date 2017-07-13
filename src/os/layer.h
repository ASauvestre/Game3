#ifdef WINDOWS
#define PLATFORM win32
#include "os/win32/core.h"
#endif

//Name generating macros
#define MAKE_FN_NAME(x, y) x##_##y
#define GENERATE_FUNC_NAME(platform, name) MAKE_FN_NAME(platform, name)

// General
#define os_specific_create_window        GENERATE_FUNC_NAME(PLATFORM, create_window)
#define os_specific_update_keyboard      GENERATE_FUNC_NAME(PLATFORM, update_keyboard)
#define os_specific_update_window_events GENERATE_FUNC_NAME(PLATFORM, update_window_events)

// Time
#define os_specific_init_clock           GENERATE_FUNC_NAME(PLATFORM, init_clock)
#define os_specific_get_time             GENERATE_FUNC_NAME(PLATFORM, get_time)
#define os_specific_sleep                GENERATE_FUNC_NAME(PLATFORM, sleep)

// DLL
#define os_specific_load_dll             GENERATE_FUNC_NAME(PLATFORM, load_dll)
#define os_specific_get_address_from_dll GENERATE_FUNC_NAME(PLATFORM, get_address_from_dll)