#ifndef _YTLOG_H
#define _YTLOG_H

#include <stdio.h>
#include <unistd.h>

#define YT_LOG_LEVEL_ERR 1
#define YT_LOG_LEVEL_WARN 2
#define YT_LOG_LEVEL_INFO 3
#define YT_LOG_LEVEL_DEBUG 4

#ifndef YT_LOG_LEVEL
#define YT_LOG_LEVEL YT_LOG_LEVEL_WARN
#endif

#if defined(YTMSG_FULL) || defined(YTMSG_LITE)
    #define PRINTF printf
    #define USLEEP(us) usleep(us)
#else
    void vkprintf(char *fmt, ...);
    #define PRINTF vkprintf
    #define USLEEP(x)
#endif

#ifdef NO_PRINT
#define LOG_E(format, args...)
#define LOG_W(format, args...)
#define LOG_I(format, args...)
#define LOG_D(format, args...)
#else  // else of NO_PRINT
#define LOG_E(format, args...) do \
{ \
    if(YT_LOG_LEVEL >= YT_LOG_LEVEL_ERR) { \
            PRINTF(format, ##args);  \
        USLEEP(1000); \
    } \
} while (0)

#define LOG_W(format, args...) do \
{ \
    if(YT_LOG_LEVEL >= YT_LOG_LEVEL_WARN) { \
        PRINTF(format, ##args); \
    } \
} while (0)

#define LOG_I(format, args...) do \
{ \
    if(YT_LOG_LEVEL >= YT_LOG_LEVEL_INFO) { \
        PRINTF(format, ##args); \
    } \
} while (0)

#define LOG_D(format, args...) do \
{ \
    if(YT_LOG_LEVEL >= YT_LOG_LEVEL_DEBUG) { \
        PRINTF(format, ##args); \
    } \
} while(0)
#endif // end of NO_PRINT

#endif
