#pragma once

// Macros
#define log_print(category, format, ...)                            \
    {                                                               \
            char message [2048];                                    \
            sprintf(message, format, __VA_ARGS__);                  \
            printf("[%s]: %s\n", category, message, __VA_ARGS__);   \
    }                                                               \


#define array_size(array)  (sizeof(array)/sizeof(array[0]))

#define DLLIMPORT __declspec(dllimport)
#define DLLEXPORT __declspec(dllexport)

#define VNAME(x) #x

