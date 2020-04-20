#ifndef _YTLOG_H
#define _YTLOG_H

#include <stdio.h>

#define YT_LOG_LEVEL_ERR 1
#define YT_LOG_LEVEL_WARN 2
#define YT_LOG_LEVEL_INFO 3
#define YT_LOG_LEVEL_DEBUG 4

#ifndef YT_LOG_LEVEL
#define YT_LOG_LEVEL YT_LOG_LEVEL_WARN
#endif

#define COLOR_RED "\033[31m"
#define COLOR_GREEN "\033[32m"
#define COLOR_YELLOW "\033[33m"
#define COLOR_BLUE "\033[34m"
#define COLOR_PURPLE "\033[35m"
#define COLOR_CYAN "\033[36m"
#define COLOR_CYAN_LIGHT "\033[36;1m"
#define COLOR_NO "\033[0m"

//#define NO_PRINT
#if defined(YTMSG_FULL) || defined(YTMSG_LITE)
    #include <unistd.h>
#ifndef STM32
    #define PRINTF printf
#else
	#ifdef __cplusplus
	extern "C" {
	#endif
		void stm32printf(const char *fmt, ...);
	#ifdef __cplusplus
	}
	#endif
	#define PRINTF stm32printf
#endif
    #define USLEEP(us) usleep(us)
#else
    void vkprintf(const char *fmt, ...);
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
