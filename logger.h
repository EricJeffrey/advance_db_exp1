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

void LOG_DEBUG(const char *str) {
    if (LOG_DEBUG_ON)
        fprintf(stderr, "%s\n", str);
}
void LOG_DEBUG(const char *str, int arg) {
    if (LOG_DEBUG_ON)
        fprintf(stderr, "%s: %d\n", str, arg);
}

#endif // LOGGER_H
