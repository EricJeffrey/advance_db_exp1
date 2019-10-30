#if !defined(LOGGER_H)
#define LOGGER_H

#include <cstdio>
#include <cstdlib>

#define LOG_DEBUG_ON 1

#define FAIL                                                                  \
    do {                                                                      \
        fprintf(stderr, "ERROR! %s: %s: %d\n", __FILE__, __func__, __LINE__); \
        fflush(stderr);                                                       \
        exit(EXIT_FAILURE);                                                   \
    } while (0)

#define FAILARG(A)                                                                        \
    do {                                                                                  \
        fprintf(stderr, "ERROR! %s: %s: %d. MSG: %s\n", __FILE__, __func__, __LINE__, A); \
        fflush(stderr);                                                                   \
        exit(EXIT_FAILURE);                                                               \
    } while (0)

#define LOG_DEBUG(A)                               \
    do {                                           \
        if (LOG_DEBUG_ON)                          \
            fprintf(stderr, "LOG_DEBUG: %s\n", A); \
    } while (0)

#endif // LOGGER_H
