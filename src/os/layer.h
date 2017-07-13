#ifdef WINDOWS
#define PLATFORM win32
#include "os/win32/core.h"
#endif

//Name generating macros
#define MAKE_FN_NAME(x, y) x##_##y
#define GENERATE_FUNC_NAME(platform, name) MAKE_FN_NAME(platform, name)

#define os_specific_create_window 		 GENERATE_FUNC_NAME(PLATFORM,create_window)
#define os_specific_update_keyboard 	 GENERATE_FUNC_NAME(PLATFORM,update_keyboard)
#define os_specific_update_window_events GENERATE_FUNC_NAME(PLATFORM,update_window_events)
#define os_specific_init_clock 			 GENERATE_FUNC_NAME(PLATFORM,init_clock)
#define os_specific_get_time 			 GENERATE_FUNC_NAME(PLATFORM,get_time)
#define os_specific_sleep 				 GENERATE_FUNC_NAME(PLATFORM,sleep)